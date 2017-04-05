#define _XOPEN_SOURCE 1			/* Required under GLIBC for nftw() */
#define _XOPEN_SOURCE_EXTENDED 1	/* Same */

#include "fm_action.h" 

#include "fm_dir.h"
#include "fm_progress.h"
#include "fm_ui.h"
#include "fm_util.h"
#include "rc.h"

#include <libgen.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>	/* gets <sys/types.h> and <sys/stat.h> for us */
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int open_dir(void);
static int exec_file(void);
static int cp_dir_step(const char *file, const struct stat *sb, int flag, 
                struct FTW *s);

static int cp_file(char const *src, char const *dest);
static int cp_dir(char const *from);
static int cp_dir_step(char const  *file, const struct stat *sb, 
                       int flag, struct FTW *s);

int fm_open(void)
{
	int ret = SUCCESS;

	struct fm_file *fmfile = get_fm_afile();
	if (fmfile->is_dir) {
		return open_dir();
	} else if (fmfile->is_exec) {
		return exec_file();
	}

	return ret;
}

void fm_copy(void)
{
	struct fm_file *afile = get_fm_afile();
	char *name = afile->name;

	char const *dest_dir = get_fm_dir_fpath();
	char *newpath = malloc(strlen(dest_dir) + 1 + strlen(name) + 1);
	sprintf(newpath, "%s/%s", dest_dir, name);

	if (file_exist(newpath)) {
		free(newpath);
		msg_win("File exists, couldn't copy.");
		return;
	}

	fm_progress_init(); 

	pid_t pid = fork();

	switch (pid) {
	case -1: // fork error
		msg_win("fork() error.");
		break;
	case 0: // child
		if (afile->is_dir) {
			cp_dir(name);
		} else {
			cp_file(name, newpath);
			//free(newpath);
		}
		exit(EXIT_SUCCESS);
		break;
	default: ;// parent
		bool waiting_child = true;
		while (waiting_child) {
			int status;
			if (waitpid(pid, &status, WNOHANG) == 0) {
				display_progress(); 
			} else {
				display_progress_end();
				waiting_child = false;
			}
		}
	}

	free(newpath);
	fm_progress_end(); 

	struct fm_pane *dest_pane = get_fm_pane();
	if (dest_pane) {
		dest_pane->dir = reload_fm_dir(dest_pane->dir);
		if (!dest_pane->dir)
			msg_win("Error!. Could not reaload directory");
	}
}

static int open_dir(void)
{
	int ret = SUCCESS;

	struct fm_pane *pane = get_fm_apane();
	struct fm_file *fmfile = get_fm_afile();

	if (!pane || !fmfile || !fmfile->is_dir) 
		return ERROR;

	char *fpath = get_afile_fpath();
	if (fpath && (access(fpath, R_OK) == 0)) {
		struct fm_dir *new_dir = create_fm_dir(fpath);

		if (new_dir) {
			if (strcmp(fmfile->name, "..") == 0) {
				pane->cur = pane->prev_cur;
				pane->top = pane->prev_top;
			} else { 
				pane->prev_cur = pane->cur;
				pane->prev_top = pane->top;
				pane->cur = 0;
				pane->top = 0;
			}

			destroy_fm_dir(pane->dir);
			pane->dir = new_dir;
		} else {
			ret = ERROR;
		}
	} else {
		ret = ERROR;
	}

	free(fpath);
	return ret;
}

static int exec_file(void)
{
	int ret = SUCCESS;
	savetty();
	endwin();

	char *fname = get_afile_fpath();

	pid_t pid = fork();
	if (pid == -1) {
        	perror("fork");
		ret = ERROR;
	}

	if (pid == 0) { // child 
        	int ret = execl(fname, basename(fname), (char *)NULL);
        	if (ret == -1) {
                	perror("execl");
                	exit(EXIT_FAILURE);
        	}
	} else {  // parent
        	int status = 0;
        	if (wait(&status) == -1)
        		ret = ERROR;
	}

	free(fname);
	resetty();
	refresh();

	return ret;
}


static int cp_file(char const *src, char const *dest)
{
	int saved_errno = errno;

	int fd_src = open(src, O_RDONLY);
	if (fd_src < 0)
		return ERROR;

	struct stat fst = {0};
	fstat(fd_src, &fst);

	int fd_dest = open(dest, O_WRONLY | O_CREAT | O_EXCL, fst.st_mode);
	if (fd_dest < 0)
		goto error;

	ssize_t nread;
	char buf[BUFSIZ];
	while (nread = read(fd_src, buf, sizeof buf), nread > 0) {
		char *out_ptr = buf;
		ssize_t nwritten;

		do {
			nwritten = write(fd_dest, out_ptr, nread);

			if (nwritten >= 0) {
				fm_progress_inc_bytes(nwritten);
				nread -= nwritten;
				out_ptr += nwritten;
			} else if (errno != EINTR) {
				goto error;
			}

		} while (nread > 0);
	}

	if (nread == 0) {
		if (close(fd_dest) < 0) {
			fd_dest = -1;
			goto error;
		}
		close(fd_src);
		fm_progress_inc_files();

		return SUCCESS;
	}

error:
	close(fd_src);
	if (fd_dest >= 0)
		close(fd_dest);

	errno = saved_errno;
	return ERROR;
}

static int cp_dir(char const *from)
{
	//int flags = FTW_PHYS;
	int flags = 0;
	int errors = 0;
	int nfds = getdtablesize() / 2;

	if (nftw(from, cp_dir_step, nfds, flags) != 0)
		errors++;

	return (errors != 0);
}

static int cp_dir_step(char const  *file, const struct stat *sb, 
                       int flag, struct FTW *s)
{
	(void)*sb;
	(void)s;

	int ret = SUCCESS;
	char const *name = file;
	char const *dest_dir = get_fm_dir_fpath();

	char *newpath = malloc(strlen(dest_dir) + 1 + strlen(name) + 1);
	sprintf(newpath, "%s/%s", dest_dir, name);

	switch (flag) {
	case FTW_F:
		cp_file(name, newpath);
		break;
	case FTW_D:
		mkdir(newpath, sb->st_mode);
		break;
	//case FTW_SL:
	//symlink
	//	break;
	case FTW_DNR:
		//unreadable directory
		break;
	case FTW_NS:
		//stat failed
		break;
	case FTW_DP:
	case FTW_SLN:
		//FTW_DP or FTW_SLN: can't happen if FTW_PHYS set
		ret = ERROR;
		break;
	default:
		//unknown flag 
		ret = ERROR;
		break;
	}

	free(newpath);
	return ret;
}

