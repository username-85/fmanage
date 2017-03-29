#include "fm_dir.h"
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

static size_t count_files(char const *path);
static int file_cmp(const void *a, const void *b);
static void destroy_fm_files(struct fm_dir *dir);

size_t count_files(char const *path) {
	DIR *dir = opendir(path);
	if (!dir) {
		return 0;
	}

	size_t ret = 0;
	struct dirent *entry;
	while ( (entry = readdir(dir)) ) {
		if (!strcmp(entry->d_name, ".")) { 
			continue;
		}
		ret++;
	}
	closedir(dir);
	return ret;
}

int file_cmp(void const *a, void const *b) {
	struct fm_file *filea = (struct fm_file *)a;
	struct fm_file *fileb = (struct fm_file *)b;

	int rv = -(filea->is_dir - fileb->is_dir);
	if (rv == 0) rv = strcmp(filea->name, fileb->name);
	return rv;
}

void destroy_fm_dir(struct fm_dir *dir) {
	if (!dir) {
		return;
	}
	destroy_fm_files(dir);
	free(dir->path);
	free(dir);
}

void destroy_fm_files(struct fm_dir *dir) {
	if (!dir) {
		return;
	}

	if (dir->files) {
		for (int i = 0; i < dir->files_num; i++) {
			if (dir->files[i].name) {
				free(dir->files[i].name);
			}
		}
		free(dir->files);
	}
}

struct fm_dir* create_fm_dir(char const *path) {
	struct fm_dir *fmdir = calloc(1, sizeof(struct fm_dir));
	if (!fmdir) {
		goto error;
	}

	int files_num = count_files(path);
	struct fm_file *fmfiles = calloc(files_num, sizeof(struct fm_file));
	if (!fmfiles) {
		goto error;
	}
	fmdir->files = fmfiles;
	fmdir->files_num = files_num;

	fmdir->path = strdup(path);
	if (!fmdir->path) {
		goto error;
	}

	DIR *dir = opendir(path);
	struct dirent *entry;
	int i = 0;
	while( (entry = readdir(dir)) && i < files_num ) {

		if (!strcmp(entry->d_name, ".")) { 
			continue;
		}

		fmfiles[i].name = strdup(entry->d_name);
		if (!fmfiles[i].name) {
			goto error;
		}

		struct stat st = {0};
		// last parameter can be 0 or AT_SYMLINK_NOFOLLOW, 
		// which makes it act like stat() or lstat() respectively
                fstatat( dirfd(dir), entry->d_name, &st, 0); 

		fmfiles[i].size = st.st_size; 

		if (st.st_mode) {
			if(S_ISDIR(st.st_mode)) {
				fmfiles[i].is_dir = true;
			}
			else if(st.st_mode & S_IXUSR) {
				fmfiles[i].is_exec = true;
			}

			if(S_ISREG(st.st_mode)) {
				fmfiles[i].is_reg = true;
			}

			if(S_ISLNK(st.st_mode)) {
				fmfiles[i].is_slink = true;
			}
		}
		
 	 	i++; 
	}
	closedir(dir);

	qsort(fmdir->files, fmdir->files_num, 
	      sizeof(struct fm_file), file_cmp);

	return fmdir;

error:
	destroy_fm_dir(fmdir);
	return NULL;
}

