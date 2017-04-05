#define _XOPEN_SOURCE 1			/* Required under GLIBC for nftw() */
#define _XOPEN_SOURCE_EXTENDED 1	/* Same */

#include "fm_progress.h"

#include "rc.h"
#include "fm_dir.h"
#include "fm_ui.h"

#include <fcntl.h>
#include <ftw.h>	/* gets <sys/types.h> and <sys/stat.h>*/
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

static struct fm_progress *progress;

static int fm_progress_init_file(char const *path);
static int fm_progress_init_dir(char const *path);
static int fm_progress_init_dir_step(const char *file, const struct stat *sb,
                                     int flag, struct FTW *s);

int fm_progress_init(void)
{
	fm_progress_end();

	int fd = open("/dev/zero", O_RDWR);
	if (fd == -1) 
		return ERROR;

	progress = mmap(NULL, sizeof(struct fm_progress), 
	                PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (progress == MAP_FAILED)
		return ERROR;

	struct fm_file *afile = get_fm_afile();
	if (!afile) 
		return ERROR;

	char *fpath = get_afile_fpath();
	if (!fpath) 
		return ERROR;

	int ret = SUCCESS;	
	if (afile->is_dir)
		ret = fm_progress_init_dir(fpath);
	else 
		ret = fm_progress_init_file(fpath);

	free(fpath);
	return ret;
}

int fm_progress_end(void)
{
	if (progress) {
        	if (munmap(progress, sizeof(struct fm_progress)) == -1)
        		return ERROR;

		progress = NULL;
	}
	return SUCCESS;
}

void fm_progress_inc_bytes(size_t inc)
{
	if (progress)
		progress->bytes += inc;
}

void fm_progress_inc_files(void)
{
	if (progress)
		progress->files++;
}

size_t fm_progress_get_bytes(void)
{
	return (progress) ? progress->bytes : 0;
}

size_t fm_progress_get_total_bytes(void)
{
	return (progress) ? progress->total_bytes : 0;
}

static int fm_progress_init_file(char const * path)
{
	struct stat st;
	if (stat(path, &st) != -1) {
		progress->total_files += 1;
		progress->total_bytes += st.st_size;
		return SUCCESS;
	}
	return ERROR;
}

static int fm_progress_init_dir(char const * name)
{
	int flags = 0;
	int nfds = getdtablesize() / 2;

	if (nftw(name, fm_progress_init_dir_step, nfds, flags) != 0)
		return ERROR;

	return SUCCESS;
}

static int fm_progress_init_dir_step(char const *file, const struct stat *sb,
                              int flag, struct FTW *s)
{
	(void)*sb;
	(void)s;

	if (flag == FTW_F)
		return fm_progress_init_file(file);
	
	return SUCCESS;
}

