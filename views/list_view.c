#include "../app.h"

#include <string.h>
#include <stdio.h>

/* ── Submenu callback ─────────────────────────────────────────────────── */

static void list_item_callback(void* context, uint32_t index) {
    KnitCountApp* app = context;

    if(index == 0) {
        /* "New Project" */
        app->text_input_mode = KnitTextInputNewProject;
        app->text_buffer[0]  = '\0';
        view_dispatcher_send_custom_event(app->view_dispatcher, KnitEventNewProject);
    } else {
        app->selected_project_index = index - 1; /* subtract the "New Project" slot */
        view_dispatcher_send_custom_event(app->view_dispatcher, KnitEventProjectSelected);
    }
}

/* ── Public API ───────────────────────────────────────────────────────── */

void knitcount_list_view_setup(KnitCountApp* app) {
    submenu_set_header(app->project_list_submenu, "KnitCount");
    submenu_set_selected_item(app->project_list_submenu, 0);
}

void knitcount_list_view_populate(KnitCountApp* app) {
    submenu_reset(app->project_list_submenu);

    submenu_add_item(
        app->project_list_submenu,
        "New Project",
        0,
        list_item_callback,
        app);

    for(uint32_t i = 0; i < app->project_count; i++) {
        submenu_add_item(
            app->project_list_submenu,
            app->project_names[i],
            i + 1,
            list_item_callback,
            app);
    }
}
