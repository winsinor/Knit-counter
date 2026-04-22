#pragma once

#include <gui/modules/submenu.h>
#include "../project.h"

/* Forward declaration avoids circular include with app.h */
typedef struct KnitCountApp KnitCountApp;

void knitcount_list_view_setup(KnitCountApp* app);
void knitcount_list_view_populate(KnitCountApp* app);
