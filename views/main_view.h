#pragma once

#include <gui/view.h>
#include "../project.h"

typedef struct KnitCountApp KnitCountApp;

typedef struct {
    char     name[KNITCOUNT_MAX_NAME + 1];
    uint32_t main_count;
    uint32_t sub_count;
    uint32_t sub_limit;
} KnitMainViewModel;

void knitcount_main_view_setup(KnitCountApp* app);
void knitcount_main_view_update(KnitCountApp* app);
