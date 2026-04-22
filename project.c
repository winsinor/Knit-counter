#include "project.h"

#include <string.h>
#include <stdio.h>
#include <flipper_format/flipper_format.h>
#include <furi.h>

/* ── helpers ──────────────────────────────────────────────────────────── */

static void sanitize_name(char* dst, size_t dst_size, const char* src) {
    size_t i;
    for(i = 0; i < dst_size - 1 && src[i]; i++) {
        char c = src[i];
        if(c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
           c == '"' || c == '<'  || c == '>' || c == '|') {
            dst[i] = '_';
        } else {
            dst[i] = c;
        }
    }
    dst[i] = '\0';
}

void knitproject_get_path(char* buf, size_t size, const char* name) {
    char safe[KNITCOUNT_MAX_NAME + 1];
    sanitize_name(safe, sizeof(safe), name);
    snprintf(buf, size, "%s/%s%s", KNITCOUNT_DIR, safe, KNITCOUNT_EXT);
}

/* ── init ─────────────────────────────────────────────────────────────── */

void knitproject_init(KnitProject* p, const char* name) {
    memset(p, 0, sizeof(*p));
    strncpy(p->name, name, KNITCOUNT_MAX_NAME);
    p->sub_limit = KNITCOUNT_SUB_LIMIT_DEFAULT;
}

/* ── counter logic ────────────────────────────────────────────────────── */

void knitproject_increment_sub(KnitProject* p) {
    p->sub_count++;
    if(p->sub_count >= p->sub_limit) {
        p->sub_count = 0;
        p->main_count++;
    }
}

void knitproject_decrement_sub(KnitProject* p) {
    if(p->sub_count > 0) {
        p->sub_count--;
    }
}

/* ── file I/O ─────────────────────────────────────────────────────────── */

bool knitproject_save(const KnitProject* p, Storage* storage) {
    storage_common_mkdir(storage, KNITCOUNT_DIR);

    char path[256];
    knitproject_get_path(path, sizeof(path), p->name);

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    bool ok = false;

    do {
        if(!flipper_format_file_open_always(ff, path)) break;
        if(!flipper_format_write_header_cstr(ff, KNITCOUNT_FILETYPE, KNITCOUNT_VERSION)) break;
        if(!flipper_format_write_string_cstr(ff, "Name", p->name)) break;
        if(!flipper_format_write_uint32(ff, "MainCount", &p->main_count, 1)) break;
        if(!flipper_format_write_uint32(ff, "SubCount",  &p->sub_count,  1)) break;
        if(!flipper_format_write_uint32(ff, "SubLimit",  &p->sub_limit,  1)) break;
        ok = true;
    } while(false);

    flipper_format_file_close(ff);
    flipper_format_free(ff);
    return ok;
}

bool knitproject_load(KnitProject* p, Storage* storage, const char* name) {
    char path[256];
    knitproject_get_path(path, sizeof(path), name);

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    FuriString*   str = furi_string_alloc();
    bool ok = false;

    do {
        if(!flipper_format_file_open_existing(ff, path)) break;

        uint32_t version = 0;
        if(!flipper_format_read_header(ff, str, &version)) break;

        if(!flipper_format_read_string(ff, "Name", str)) break;
        strncpy(p->name, furi_string_get_cstr(str), KNITCOUNT_MAX_NAME);

        if(!flipper_format_read_uint32(ff, "MainCount", &p->main_count, 1)) break;
        if(!flipper_format_read_uint32(ff, "SubCount",  &p->sub_count,  1)) break;
        if(!flipper_format_read_uint32(ff, "SubLimit",  &p->sub_limit,  1)) break;

        if(p->sub_limit < KNITCOUNT_SUB_LIMIT_MIN)
            p->sub_limit = KNITCOUNT_SUB_LIMIT_MIN;

        ok = true;
    } while(false);

    furi_string_free(str);
    flipper_format_file_close(ff);
    flipper_format_free(ff);
    return ok;
}

bool knitproject_delete(Storage* storage, const char* name) {
    char path[256];
    knitproject_get_path(path, sizeof(path), name);
    return storage_simply_remove(storage, path);
}

bool knitproject_rename(KnitProject* p, Storage* storage, const char* new_name) {
    char old_path[256];
    knitproject_get_path(old_path, sizeof(old_path), p->name);

    strncpy(p->name, new_name, KNITCOUNT_MAX_NAME);
    p->name[KNITCOUNT_MAX_NAME] = '\0';

    char new_path[256];
    knitproject_get_path(new_path, sizeof(new_path), p->name);

    if(!knitproject_save(p, storage)) return false;

    if(strcmp(old_path, new_path) != 0) {
        storage_simply_remove(storage, old_path);
    }
    return true;
}

uint32_t knitproject_list(
    Storage* storage,
    char     names[][KNITCOUNT_MAX_NAME + 1],
    uint32_t max) {

    uint32_t count = 0;
    File*    dir   = storage_file_alloc(storage);

    if(!storage_dir_open(dir, KNITCOUNT_DIR)) {
        storage_file_free(dir);
        return 0;
    }

    FileInfo fi;
    char     fname[256];

    while(count < max && storage_dir_read(dir, &fi, fname, sizeof(fname))) {
        if(file_info_is_dir(&fi)) continue;

        size_t len = strlen(fname);
        size_t ext_len = strlen(KNITCOUNT_EXT);
        if(len <= ext_len) continue;
        if(strcmp(fname + len - ext_len, KNITCOUNT_EXT) != 0) continue;

        /* Strip extension to get the stem used as key */
        char stem[KNITCOUNT_MAX_NAME + 1];
        size_t stem_len = len - ext_len;
        if(stem_len > KNITCOUNT_MAX_NAME) stem_len = KNITCOUNT_MAX_NAME;
        strncpy(stem, fname, stem_len);
        stem[stem_len] = '\0';

        /* Load the display name from the file */
        KnitProject tmp;
        memset(&tmp, 0, sizeof(tmp));
        if(knitproject_load(&tmp, storage, stem)) {
            strncpy(names[count], tmp.name, KNITCOUNT_MAX_NAME);
            names[count][KNITCOUNT_MAX_NAME] = '\0';
        } else {
            strncpy(names[count], stem, KNITCOUNT_MAX_NAME);
            names[count][KNITCOUNT_MAX_NAME] = '\0';
        }
        count++;
    }

    storage_dir_close(dir);
    storage_file_free(dir);
    return count;
}
