#include "../app.h"

#include <gui/canvas.h>
#include <gui/elements.h>
#include <input/input.h>
#include <string.h>
#include <stdio.h>

/* ── Draw ─────────────────────────────────────────────────────────────── */

static void main_view_draw_cb(Canvas* canvas, void* model_ptr) {
    KnitMainViewModel* m = model_ptr;
    char buf[32];

    canvas_clear(canvas);

    /* ── Title bar ─────────────────────────────────── */
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_box(canvas, 0, 0, 128, 13);
    canvas_set_color(canvas, ColorWhite);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignBottom, "KnitCount");
    canvas_set_color(canvas, ColorBlack);

    /* ── Project name ──────────────────────────────── */
    canvas_set_font(canvas, FontSecondary);
    /* Truncate display to ~21 chars to fit 128px width */
    char name_buf[22];
    strncpy(name_buf, m->name, 21);
    name_buf[21] = '\0';
    canvas_draw_str_aligned(canvas, 64, 24, AlignCenter, AlignBottom, name_buf);

    canvas_draw_line(canvas, 0, 26, 127, 26);

    /* ── Counters ───────────────────────────────────── */
    canvas_set_font(canvas, FontPrimary);

    snprintf(buf, sizeof(buf), "Row:    %lu", (unsigned long)m->main_count);
    canvas_draw_str(canvas, 4, 40, buf);

    snprintf(
        buf,
        sizeof(buf),
        "Repeat: %lu / %lu",
        (unsigned long)m->sub_count,
        (unsigned long)m->sub_limit);
    canvas_draw_str(canvas, 4, 54, buf);

    /* ── Bottom hints ──────────────────────────────── */
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 0, 63, "< undo");
    canvas_draw_str_aligned(canvas, 127, 63, AlignRight, AlignBottom, "next >");
}

/* ── Input ────────────────────────────────────────────────────────────── */

static bool main_view_input_cb(InputEvent* event, void* context) {
    KnitCountApp* app = context;

    /* Long-press OK → settings */
    if(event->type == InputTypeLong && event->key == InputKeyOk) {
        view_dispatcher_send_custom_event(app->view_dispatcher, KnitEventOpenSettings);
        return true;
    }

    /* Short press */
    if(event->type == InputTypeShort) {
        switch(event->key) {
        case InputKeyRight:
        case InputKeyOk:
            knitproject_increment_sub(&app->current_project);
            knitcount_main_view_update(app);
            return true;
        case InputKeyLeft:
            knitproject_decrement_sub(&app->current_project);
            knitcount_main_view_update(app);
            return true;
        case InputKeyBack:
            view_dispatcher_send_custom_event(app->view_dispatcher, KnitEventBackToList);
            return true;
        default:
            break;
        }
    }

    /* Repeat (held arrow keys) */
    if(event->type == InputTypeRepeat) {
        if(event->key == InputKeyRight) {
            knitproject_increment_sub(&app->current_project);
            knitcount_main_view_update(app);
            return true;
        } else if(event->key == InputKeyLeft) {
            knitproject_decrement_sub(&app->current_project);
            knitcount_main_view_update(app);
            return true;
        }
    }

    return false;
}

/* ── Public API ───────────────────────────────────────────────────────── */

void knitcount_main_view_setup(KnitCountApp* app) {
    view_allocate_model(app->main_view, ViewModelTypeLockFree, sizeof(KnitMainViewModel));
    view_set_draw_callback(app->main_view, main_view_draw_cb);
    view_set_input_callback(app->main_view, main_view_input_cb);
    view_set_context(app->main_view, app);
}

void knitcount_main_view_update(KnitCountApp* app) {
    with_view_model(
        app->main_view,
        KnitMainViewModel* m,
        {
            strncpy(m->name, app->current_project.name, KNITCOUNT_MAX_NAME);
            m->name[KNITCOUNT_MAX_NAME] = '\0';
            m->main_count = app->current_project.main_count;
            m->sub_count  = app->current_project.sub_count;
            m->sub_limit  = app->current_project.sub_limit;
        },
        true);
}
