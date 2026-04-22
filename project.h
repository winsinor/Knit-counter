#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <storage/storage.h>

#define KNITCOUNT_DIR       "/ext/knitcount"
#define KNITCOUNT_EXT       ".cfg"
#define KNITCOUNT_FILETYPE  "KnitCount Project"
#define KNITCOUNT_VERSION   1
#define KNITCOUNT_MAX_NAME  63
#define KNITCOUNT_MAX_PROJECTS 20
#define KNITCOUNT_SUB_LIMIT_DEFAULT 10
#define KNITCOUNT_SUB_LIMIT_MIN     2

typedef struct {
    char     name[KNITCOUNT_MAX_NAME + 1];
    uint32_t main_count;
    uint32_t sub_count;
    uint32_t sub_limit;
} KnitProject;

void     knitproject_init(KnitProject* p, const char* name);
bool     knitproject_save(const KnitProject* p, Storage* storage);
bool     knitproject_load(KnitProject* p, Storage* storage, const char* name);
bool     knitproject_delete(Storage* storage, const char* name);
bool     knitproject_rename(KnitProject* p, Storage* storage, const char* new_name);
uint32_t knitproject_list(Storage* storage, char names[][KNITCOUNT_MAX_NAME + 1], uint32_t max);
void     knitproject_get_path(char* buf, size_t size, const char* name);

/* Counter logic (pure, no I/O) */
void knitproject_increment_sub(KnitProject* p);
void knitproject_decrement_sub(KnitProject* p);
