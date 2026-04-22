#include "../app.h"

#include <string.h>
#include <stdio.h>

/* ── Item indices (stable, not the submenu's uint32_t index) ──────────── */
typedef enum {
    SettingsItemRename    = 0,
    SettingsItemSubLimit  = 1,
    SettingsItemResetSub  = 2,
    SettingsItemResetAll  = 3,
    SettingsItemDelete    = 4,
} SettingsItem;

/* ── Submenu callback ─────────────────────────────────────────────────── */

static void settings_item_callback(void* context, uint32_t index) {
    KnitCountApp* app = context;

    switch((SettingsItem)index) {
    case SettingsItemRename:
        view_dispatcher_send_custom_event(app->view_dispatcher, KnitEventSettingsRename);
        break;
    case SettingsItemSubLimit:
        view_dispatcher_send_custom_event(app->view_dispatcher, KnitEventSettingsSubLimit);
        break;
    case SettingsItemResetSub:
        view_dispatcher_send_custom_event(app->view_dispatcher, KnitEventSettingsResetSub);
        break;
    case SettingsItemResetAll:
        view_dispatcher_send_custom_event(app->view_dispatcher, KnitEventSettingsResetAll);
        break;
    case SettingsItemDelete:
        view_dispatcher_send_custom_event(app->view_dispatcher, KnitEventSettingsDelete);
        break;
    }
}

/* ── Public API ───────────────────────────────────────────────────────── */

void knitcount_settings_view_setup(KnitCountApp* app) {
    submenu_set_header(app->settings_submenu, "Settings");
}

void knitcount_settings_view_populate(KnitCountApp* app) {
    submenu_reset(app->settings_submenu);

    submenu_add_item(
        app->settings_submenu,
        "Rename Project",
        SettingsItemRename,
        settings_item_callback,
        app);

    /* Show current sub-limit in the menu label */
    char limit_label[40];
    snprintf(
        limit_label,
        sizeof(limit_label),
        "Set Repeat Limit: %lu",
        (unsigned long)app->current_project.sub_limit);
    submenu_add_item(
        app->settings_submenu,
        limit_label,
        SettingsItemSubLimit,
        settings_item_callback,
        app);

    submenu_add_item(
        app->settings_submenu,
        "Reset Repeat Counter",
        SettingsItemResetSub,
        settings_item_callback,
        app);

    submenu_add_item(
        app->settings_submenu,
        "Reset All Counters",
        SettingsItemResetAll,
        settings_item_callback,
        app);

    submenu_add_item(
        app->settings_submenu,
        "Delete Project",
        SettingsItemDelete,
        settings_item_callback,
        app);
}
