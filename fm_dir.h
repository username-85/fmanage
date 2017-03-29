#ifndef FM_DIR_H
#define FM_DIR_H

#include <stdio.h>
#include <stdbool.h>

struct fm_dir {
	char *path;
	int files_num;
	struct fm_file *files;
};

struct fm_file {
	char *name;
	bool is_dir;
	bool is_exec;
	bool is_reg;
	bool is_slink;
	int size;
};

void destroy_fm_dir(struct fm_dir *dir);
struct fm_dir* create_fm_dir(char const *path);

#endif
