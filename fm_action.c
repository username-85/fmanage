#include "fm_action.h"
#include "fm_dir.h"
#include "fm_ui.h"
#include "rc.h"

#include <libgen.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int open_dir(void);
static int exec_file(void);

int open_file(void) {
	int ret = SUCCESS;

	struct fm_file *fmfile = get_afile();
	if (fmfile->is_dir) {
		return open_dir();
	}
	else if (fmfile->is_exec) {
		return exec_file();
	}

	return ret;
}

int open_dir(void) {
	int ret = SUCCESS;

	struct fm_pane *pane = get_apane();
	struct fm_file *fmfile = get_afile();

	if (fmfile->is_dir) {
		char *fpath = get_afile_fpath();

		if (access(fpath, R_OK) == 0) {
			destroy_fm_dir(pane->dir);
			pane->dir = NULL;
			pane->dir = create_fm_dir(fpath);
			pane->cur=0;
			pane->top=0;
		}
		else {
			ret = ERROR;
		}

		free(fpath); 
	}
	else {
		ret = ERROR;
	}

	return ret;
}

int exec_file(void) {
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
	}
	else {  // parent
        	int status;
        	if (wait(&status) == -1) {
        		ret = ERROR;
        	}
	}

	free(fname);
	resetty();
	refresh();

	return ret;
}

