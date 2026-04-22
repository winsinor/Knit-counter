#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/dialog_ex.h>
#include <storage/storage.h>
#include <flipper_format/flipper_format.h>

#include "project.h"
#include "views/list_view.h"
#include "views/main_view.h"
#include "views/settings_view.h"

/* ── View IDs ─────────────────────────────────────────────────────────── */
typedef enum {
    KnitViewProjectList = 0,
    KnitViewMain,
    KnitViewSettings,
    KnitViewTextInput,
    KnitViewDialog,
} KnitViewId;

/* ── Text-input modes ─────────────────────────────────────────────────── */
typedef enum {
    KnitTextInputNewProject = 0,
    KnitTextInputRename,
    KnitTextInputSubLimit,
} KnitTextInputMode;

/* ── Dialog modes ─────────────────────────────────────────────────────── */
typedef enum {
    KnitDialogResetSub = 0,
    KnitDialogResetAll,
    KnitDialogDelete,
} KnitDialogMode;

/* ── Custom events sent between views and the dispatcher ──────────────── */
typedef enum {
    KnitEventNewProject = 0,
    KnitEventProjectSelected,
    KnitEventBackToList,
    KnitEventOpenSettings,
    KnitEventSettingsRename,
    KnitEventSettingsSubLimit,
    KnitEventSettingsResetSub,
    KnitEventSettingsResetAll,
    KnitEventSettingsDelete,
    KnitEventSettingsBack,
    KnitEventTextInputDone,
    KnitEventDialogConfirm,
    KnitEventDialogCancel,
} KnitEvent;

/* ── App struct ───────────────────────────────────────────────────────── */
struct KnitCountApp {
    ViewDispatcher* view_dispatcher;
    Gui*            gui;
    Storage*        storage;

    /* Widgets */
    Submenu*   project_list_submenu;
    View*      main_view;
    Submenu*   settings_submenu;
    TextInput* text_input;
    DialogEx*  dialog;

    /* Current project */
    KnitProject current_project;
    bool        has_project;

    /* Project directory cache */
    char     project_names[KNITCOUNT_MAX_PROJECTS][KNITCOUNT_MAX_NAME + 1];
    uint32_t project_count;
    uint32_t selected_project_index; /* index into project_names[] */

    /* Text-input state */
    KnitTextInputMode text_input_mode;
    char              text_buffer[KNITCOUNT_MAX_NAME + 1];

    /* Dialog state */
    KnitDialogMode dialog_mode;

    /* Tracks active view so navigation_callback doesn't need SDK query */
    KnitViewId current_view;
};

typedef struct KnitCountApp KnitCountApp;

/* Entry point */
int32_t knitcount_app(void* p);
