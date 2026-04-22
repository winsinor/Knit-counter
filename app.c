#include "app.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <furi.h>

/* ══════════════════════════════════════════════════════════════════════
   Forward declarations for static helpers
   ══════════════════════════════════════════════════════════════════════ */
static void knitcount_open_project(KnitCountApp* app, uint32_t index);
static void knitcount_save_current(KnitCountApp* app);
static void knitcount_go_list(KnitCountApp* app);
static void knitcount_open_settings(KnitCountApp* app);
static void knitcount_show_dialog(KnitCountApp* app, KnitDialogMode mode);
static void knitcount_show_text_input(KnitCountApp* app, KnitTextInputMode mode);

/* Wrapper that keeps current_view in sync */
static void knitcount_switch_to_view(KnitCountApp* app, KnitViewId id) {
    app->current_view = id;
    view_dispatcher_switch_to_view(app->view_dispatcher, (uint32_t)id);
}

/* ══════════════════════════════════════════════════════════════════════
   Text-input callback
   ══════════════════════════════════════════════════════════════════════ */

static void text_input_result_cb(void* context) {
    KnitCountApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, KnitEventTextInputDone);
}

/* ══════════════════════════════════════════════════════════════════════
   Dialog callback
   ══════════════════════════════════════════════════════════════════════ */

static void dialog_result_cb(DialogExResult result, void* context) {
    KnitCountApp* app = context;
    if(result == DialogExResultRight) {
        view_dispatcher_send_custom_event(app->view_dispatcher, KnitEventDialogConfirm);
    } else {
        view_dispatcher_send_custom_event(app->view_dispatcher, KnitEventDialogCancel);
    }
}

/* ══════════════════════════════════════════════════════════════════════
   Navigation (Back button falls through to here)
   ══════════════════════════════════════════════════════════════════════ */

static bool navigation_callback(void* context) {
    KnitCountApp* app = context;
    /* Only called when a view doesn't consume Back.
       Submenus pass Back up; our main view consumes it itself.
       So this fires from: project list, settings, text input, dialog. */

    switch(app->current_view) {
    case KnitViewProjectList:
        view_dispatcher_stop(app->view_dispatcher);
        return false;

    case KnitViewSettings:
        knitcount_switch_to_view(app, KnitViewMain);
        return true;

    case KnitViewTextInput:
        switch(app->text_input_mode) {
        case KnitTextInputNewProject:
            knitcount_switch_to_view(app, KnitViewProjectList);
            break;
        case KnitTextInputRename:
        case KnitTextInputSubLimit:
            knitcount_switch_to_view(app, KnitViewSettings);
            break;
        }
        return true;

    case KnitViewDialog:
        knitcount_switch_to_view(app, KnitViewSettings);
        return true;

    default:
        view_dispatcher_stop(app->view_dispatcher);
        return false;
    }
}

/* ══════════════════════════════════════════════════════════════════════
   Custom event dispatcher
   ══════════════════════════════════════════════════════════════════════ */

static bool custom_event_callback(void* context, uint32_t event) {
    KnitCountApp* app = context;

    switch((KnitEvent)event) {

    /* ── Project list ─────────────────────────────────────────────── */
    case KnitEventNewProject:
        knitcount_show_text_input(app, KnitTextInputNewProject);
        break;

    case KnitEventProjectSelected:
        knitcount_open_project(app, app->selected_project_index);
        break;

    case KnitEventBackToList:
        knitcount_save_current(app);
        knitcount_go_list(app);
        break;

    /* ── Main counter ─────────────────────────────────────────────── */
    case KnitEventOpenSettings:
        knitcount_save_current(app);
        knitcount_open_settings(app);
        break;

    /* ── Settings ─────────────────────────────────────────────────── */
    case KnitEventSettingsRename:
        knitcount_show_text_input(app, KnitTextInputRename);
        break;

    case KnitEventSettingsSubLimit:
        knitcount_show_text_input(app, KnitTextInputSubLimit);
        break;

    case KnitEventSettingsResetSub:
        knitcount_show_dialog(app, KnitDialogResetSub);
        break;

    case KnitEventSettingsResetAll:
        knitcount_show_dialog(app, KnitDialogResetAll);
        break;

    case KnitEventSettingsDelete:
        knitcount_show_dialog(app, KnitDialogDelete);
        break;

    case KnitEventSettingsBack:
        knitcount_switch_to_view(app, KnitViewMain);
        break;

    /* ── Text input done ──────────────────────────────────────────── */
    case KnitEventTextInputDone: {
        switch(app->text_input_mode) {

        case KnitTextInputNewProject: {
            if(app->text_buffer[0] == '\0') {
                /* Empty name: stay on text input */
                break;
            }
            KnitProject p;
            knitproject_init(&p, app->text_buffer);
            knitproject_save(&p, app->storage);

            /* Refresh project list and open the new project */
            app->project_count = knitproject_list(
                app->storage,
                app->project_names,
                KNITCOUNT_MAX_PROJECTS);
            knitcount_list_view_populate(app);

            app->current_project = p;
            app->has_project     = true;
            knitcount_main_view_update(app);
            knitcount_switch_to_view(app, KnitViewMain);
            break;
        }

        case KnitTextInputRename: {
            if(app->text_buffer[0] == '\0') break;
            knitproject_rename(&app->current_project, app->storage, app->text_buffer);
            /* Refresh list cache */
            app->project_count = knitproject_list(
                app->storage,
                app->project_names,
                KNITCOUNT_MAX_PROJECTS);
            knitcount_list_view_populate(app);
            knitcount_main_view_update(app);
            knitcount_open_settings(app);
            break;
        }

        case KnitTextInputSubLimit: {
            long val = strtol(app->text_buffer, NULL, 10);
            if(val >= KNITCOUNT_SUB_LIMIT_MIN) {
                app->current_project.sub_limit = (uint32_t)val;
                /* Clamp sub_count to new limit */
                if(app->current_project.sub_count >= app->current_project.sub_limit) {
                    app->current_project.sub_count = 0;
                }
                knitproject_save(&app->current_project, app->storage);
                knitcount_main_view_update(app);
            }
            knitcount_open_settings(app);
            break;
        }
        }
        break;
    }

    /* ── Dialog result ────────────────────────────────────────────── */
    case KnitEventDialogConfirm:
        switch(app->dialog_mode) {
        case KnitDialogResetSub:
            app->current_project.sub_count = 0;
            knitproject_save(&app->current_project, app->storage);
            knitcount_main_view_update(app);
            knitcount_switch_to_view(app, KnitViewMain);
            break;

        case KnitDialogResetAll:
            app->current_project.sub_count  = 0;
            app->current_project.main_count = 0;
            knitproject_save(&app->current_project, app->storage);
            knitcount_main_view_update(app);
            knitcount_switch_to_view(app, KnitViewMain);
            break;

        case KnitDialogDelete:
            knitproject_delete(app->storage, app->current_project.name);
            app->has_project   = false;
            app->project_count = knitproject_list(
                app->storage,
                app->project_names,
                KNITCOUNT_MAX_PROJECTS);
            knitcount_list_view_populate(app);
            knitcount_switch_to_view(app, KnitViewProjectList);
            break;
        }
        break;

    case KnitEventDialogCancel:
        knitcount_switch_to_view(app, KnitViewSettings);
        break;

    default:
        break;
    }

    return true;
}

/* ══════════════════════════════════════════════════════════════════════
   Helper: open a project by index in project_names[]
   ══════════════════════════════════════════════════════════════════════ */

static void knitcount_open_project(KnitCountApp* app, uint32_t index) {
    furi_assert(index < app->project_count);
    KnitProject p;
    memset(&p, 0, sizeof(p));
    if(knitproject_load(&p, app->storage, app->project_names[index])) {
        app->current_project = p;
        app->has_project     = true;
        knitcount_main_view_update(app);
        knitcount_switch_to_view(app, KnitViewMain);
    }
    /* If load fails, stay on list */
}

static void knitcount_save_current(KnitCountApp* app) {
    if(app->has_project) {
        knitproject_save(&app->current_project, app->storage);
    }
}

static void knitcount_go_list(KnitCountApp* app) {
    app->project_count = knitproject_list(
        app->storage,
        app->project_names,
        KNITCOUNT_MAX_PROJECTS);
    knitcount_list_view_populate(app);
    knitcount_switch_to_view(app, KnitViewProjectList);
}

static void knitcount_open_settings(KnitCountApp* app) {
    knitcount_settings_view_populate(app);
    knitcount_switch_to_view(app, KnitViewSettings);
}

/* ── Text input helper ────────────────────────────────────────────────── */

static void knitcount_show_text_input(KnitCountApp* app, KnitTextInputMode mode) {
    app->text_input_mode = mode;

    switch(mode) {
    case KnitTextInputNewProject:
        text_input_set_header_text(app->text_input, "Project Name");
        app->text_buffer[0] = '\0';
        text_input_set_result_callback(
            app->text_input,
            text_input_result_cb,
            app,
            app->text_buffer,
            sizeof(app->text_buffer),
            false);
        break;

    case KnitTextInputRename:
        text_input_set_header_text(app->text_input, "Rename Project");
        strncpy(app->text_buffer, app->current_project.name, sizeof(app->text_buffer) - 1);
        app->text_buffer[sizeof(app->text_buffer) - 1] = '\0';
        text_input_set_result_callback(
            app->text_input,
            text_input_result_cb,
            app,
            app->text_buffer,
            sizeof(app->text_buffer),
            false);
        break;

    case KnitTextInputSubLimit:
        text_input_set_header_text(app->text_input, "Repeat Limit (min 2)");
        snprintf(
            app->text_buffer,
            sizeof(app->text_buffer),
            "%lu",
            (unsigned long)app->current_project.sub_limit);
        text_input_set_result_callback(
            app->text_input,
            text_input_result_cb,
            app,
            app->text_buffer,
            sizeof(app->text_buffer),
            true);
        break;
    }

    knitcount_switch_to_view(app, KnitViewTextInput);
}

/* ── Dialog helper ────────────────────────────────────────────────────── */

static void knitcount_show_dialog(KnitCountApp* app, KnitDialogMode mode) {
    app->dialog_mode = mode;

    dialog_ex_reset(app->dialog);
    dialog_ex_set_result_callback(app->dialog, dialog_result_cb, app);

    switch(mode) {
    case KnitDialogResetSub:
        dialog_ex_set_header(app->dialog, "Reset Repeat?", 64, 10, AlignCenter, AlignBottom);
        dialog_ex_set_text(
            app->dialog,
            "Repeat counter\nwill be set to 0.",
            64, 32, AlignCenter, AlignCenter);
        break;

    case KnitDialogResetAll:
        dialog_ex_set_header(app->dialog, "Reset All?", 64, 10, AlignCenter, AlignBottom);
        dialog_ex_set_text(
            app->dialog,
            "Both counters\nwill be set to 0.",
            64, 32, AlignCenter, AlignCenter);
        break;

    case KnitDialogDelete:
        dialog_ex_set_header(app->dialog, "Delete Project?", 64, 10, AlignCenter, AlignBottom);
        dialog_ex_set_text(
            app->dialog,
            "This cannot\nbe undone.",
            64, 32, AlignCenter, AlignCenter);
        break;
    }

    dialog_ex_set_left_button_text(app->dialog, "Cancel");
    dialog_ex_set_right_button_text(app->dialog, "OK");
    knitcount_switch_to_view(app, KnitViewDialog);
}

/* ══════════════════════════════════════════════════════════════════════
   App lifecycle
   ══════════════════════════════════════════════════════════════════════ */

static KnitCountApp* knitcount_app_alloc(void) {
    KnitCountApp* app = malloc(sizeof(KnitCountApp));
    memset(app, 0, sizeof(*app));

    /* Storage */
    app->storage = furi_record_open(RECORD_STORAGE);
    storage_common_mkdir(app->storage, KNITCOUNT_DIR);

    /* GUI */
    app->gui = furi_record_open(RECORD_GUI);

    /* ViewDispatcher */
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, navigation_callback);

    /* ── Widgets ─────────────────────────────────────── */

    /* Project list (Submenu) */
    app->project_list_submenu = submenu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        KnitViewProjectList,
        submenu_get_view(app->project_list_submenu));

    /* Main counter (custom View) */
    app->main_view = view_alloc();
    knitcount_main_view_setup(app);
    view_dispatcher_add_view(app->view_dispatcher, KnitViewMain, app->main_view);

    /* Settings (Submenu) */
    app->settings_submenu = submenu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        KnitViewSettings,
        submenu_get_view(app->settings_submenu));

    /* Text input */
    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        KnitViewTextInput,
        text_input_get_view(app->text_input));

    /* Dialog */
    app->dialog = dialog_ex_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        KnitViewDialog,
        dialog_ex_get_view(app->dialog));

    /* ── Static setup ────────────────────────────────── */
    knitcount_list_view_setup(app);
    knitcount_settings_view_setup(app);

    /* Attach to GUI */
    view_dispatcher_attach_to_gui(
        app->view_dispatcher,
        app->gui,
        ViewDispatcherTypeFullscreen);

    return app;
}

static void knitcount_app_free(KnitCountApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, KnitViewProjectList);
    view_dispatcher_remove_view(app->view_dispatcher, KnitViewMain);
    view_dispatcher_remove_view(app->view_dispatcher, KnitViewSettings);
    view_dispatcher_remove_view(app->view_dispatcher, KnitViewTextInput);
    view_dispatcher_remove_view(app->view_dispatcher, KnitViewDialog);

    submenu_free(app->project_list_submenu);
    view_free(app->main_view);
    submenu_free(app->settings_submenu);
    text_input_free(app->text_input);
    dialog_ex_free(app->dialog);

    view_dispatcher_free(app->view_dispatcher);

    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_STORAGE);

    free(app);
}

/* ══════════════════════════════════════════════════════════════════════
   Entry point
   ══════════════════════════════════════════════════════════════════════ */

int32_t knitcount_app(void* p) {
    UNUSED(p);

    KnitCountApp* app = knitcount_app_alloc();

    /* Scan SD card for existing projects */
    app->project_count = knitproject_list(
        app->storage,
        app->project_names,
        KNITCOUNT_MAX_PROJECTS);
    knitcount_list_view_populate(app);

    /* Start on the project list */
    knitcount_switch_to_view(app, KnitViewProjectList);

    /* Run until Back on project list */
    view_dispatcher_run(app->view_dispatcher);

    /* Auto-save on exit */
    knitcount_save_current(app);

    knitcount_app_free(app);
    return 0;
}
