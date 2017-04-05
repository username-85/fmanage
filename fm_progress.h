#ifndef FM_PROGRESS_H
#define FM_PROGRESS_H

#include <stdio.h>

struct fm_progress {
        size_t total_bytes;
        size_t bytes;
        size_t total_files;
        size_t files;
};

int fm_progress_init(void);
int fm_progress_end(void);

void fm_progress_inc_bytes(size_t inc);
void fm_progress_inc_files(void);

size_t fm_progress_get_bytes();
size_t fm_progress_get_total_bytes();

#endif
