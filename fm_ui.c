#include "fm_ui.h"
#include "fm_dir.h"
#include "rc.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h> 
#include <string.h>
#include <libgen.h>

#define P_MINY(pane) (getbegy(pane->win) + 1)
#define P_MAXY(pane) (getmaxy(pane->win) - 2)
#define P_MINX(pane) (1)
#define P_MAXX(pane) (getmaxx(pane->win) - 2)
#define P_ROWLEN(pane) (P_MAXX(pane) - P_MINX(pane))
#define P_HEADERLEN(pane) (P_MAXX(pane) - P_MINX(pane) - 1)
#define P_HEIGHT(pane) (P_MAXY(pane) - P_MINY(pane))

#define INFOSTR "F10-Quit"

static struct fm_pane *lpane;
static struct fm_pane *rpane;
static WINDOW *info_win;

static struct fm_pane* create_fm_pane(void); 
static void destroy_fm_pane(struct fm_pane *);
static void display_fm_pane(struct fm_pane * pane);

static WINDOW * create_lpane_win();
static WINDOW * create_rpane_win();
static WINDOW * create_info_win();

static inline struct fm_pane * get_apane(void);

struct fm_pane* create_fm_pane(void) {
	struct fm_pane *pane = calloc(1, sizeof(struct fm_pane));
	if (!pane) {
		goto error;
	}
	
	char cwd[PATH_MAX];
	if (!getcwd(cwd, PATH_MAX)) {
		goto error;
	}

	struct fm_dir *dir = create_fm_dir(cwd);
	if (!dir) {
		goto error;
	}

	pane->dir = dir;
	return pane;
error:
	free(pane);
	return NULL;
}

void destroy_fm_pane(struct fm_pane * pane) {
	destroy_fm_dir(pane->dir);
	free(pane);
}

void switch_pane(void) {
	if (rpane->active) {
		rpane->active = false;
		lpane->active = true;
	}
	else {
		rpane->active = true;
		lpane->active = false;
	}
}

void mv_cur_down(void) {
	struct fm_pane *pane = get_apane();
	int new_cur = pane->cur + 1;
	if (new_cur >= pane->dir->files_num) {
		return;
	}
	pane->cur = new_cur;

	if (new_cur >= pane->top + P_MAXY(pane)) {
		pane->top++;
	}

}

void mv_cur_up(void) {
	struct fm_pane *pane = get_apane();
	if (pane->cur == 0) {
		return;
	}
	pane->cur--; 

	if (pane->cur < pane->top) {
		pane->top = pane->cur;
	}

}

int open_file(void) {
	int ret = 0;

	struct fm_pane *pane = get_apane();
	struct fm_file *fmfile = pane->dir->files + pane->cur;

	if (fmfile->is_dir) {
		char *newpath = NULL;
		char *fname   = fmfile->name;
		char *dir     = pane->dir->path;

		if (strcmp(fname, "..") == 0) {
			newpath = strdup(dir);
			dirname(newpath);
		}
		else {
			newpath = malloc(strlen(dir) + strlen("/") 
		                         + strlen(fname) + 1 );
		        if (strcmp(dir, "/") == 0) {
				sprintf(newpath, "%s%s", dir, fname);
			}
			else {
				sprintf(newpath, "%s%s%s", dir, "/", fname);
			}
		}

		if (access(newpath, R_OK) == 0) {
			destroy_fm_dir(pane->dir);
			pane->dir = NULL;
			pane->dir = create_fm_dir(newpath);
			pane->cur=0;
			pane->top=0;
                }
                else {
                	ret = -1;
                }

		free(newpath);
	}

	return ret;
}

void display_fm_pane(struct fm_pane * pane) {
	if (!pane) {
		return;
	}

	werase(pane->win);
	wattroff(pane->win, A_REVERSE);
	box(pane->win, 0, 0);

	if (pane->active) {
		wattron(pane->win, A_REVERSE);
	}

	wmove(pane->win, 0, 2);
	wprintw(pane->win, "%.*s", P_HEADERLEN(pane),  pane->dir->path);

	if (pane->active) {
		wattroff(pane->win, A_REVERSE);
	}

	if (pane->dir->files_num < pane->top) {
		pane->top = 0;
	}
	if (pane->dir->files_num < pane->cur) {
		pane->cur = 0;
	}

	struct fm_file *files = pane->dir->files + pane->top;
	struct fm_file *cur_file = pane->dir->files + pane->cur;
	for (int y = 1; y <= pane->dir->files_num && y <= P_MAXY(pane); y++) {
		
		if (files == cur_file && pane->active) {
			wattron(pane->win, A_REVERSE);
		}
		else {
			wattroff(pane->win, A_REVERSE);
		}

		wmove(pane->win, y, 1);
		if (files->is_dir) {
			waddch(pane->win, '/');
		}
		wprintw(pane->win, "%.*s", P_ROWLEN(pane), files->name);

		if (files == cur_file && pane->active) {
			int cur_x = getcurx(pane->win);
			for (int i = cur_x; i < P_MAXX(pane); i++) {
				waddch(pane->win, ' ');
			}
		}
		files++;
	}
	wnoutrefresh(pane->win);
}

void msg(char const * msg) {
	werase(info_win);
	waddnstr(info_win, msg, COLS);
	wnoutrefresh(info_win);
}

void msg_rev(char const * msg) {
	werase(info_win);
	wattron(info_win, A_REVERSE);
	waddnstr(info_win, msg, COLS);
	wattroff(info_win, A_REVERSE);
	wrefresh(info_win);
}

WINDOW * create_lpane_win() {
	return newwin(LINES-1, COLS / 2, 0, 0);
}

WINDOW * create_rpane_win() {
	int ost = COLS % 2;
	return newwin(LINES-1, COLS / 2, 0, COLS / 2 + ost);
}

WINDOW * create_info_win() {
	return newwin(1, COLS, LINES-1, 0);
}

void fm_ui_reset(void) {
	endwin();
	refresh();
	clear();

	lpane->win = create_lpane_win();
	rpane->win = create_rpane_win();
	info_win = create_info_win();
}

int fm_ui_init(void) {
	info_win = create_info_win();
	if (!info_win) {
		goto error;
	}
	msg(INFOSTR);

	lpane = create_fm_pane();
	if (!lpane) {
		goto error;
	}
	lpane->win = create_lpane_win();
	if (!lpane->win) {
		goto error;
	}
	lpane->active = true;

	rpane = create_fm_pane();
	if (!rpane) {
		goto error;
	}
	rpane->win = create_rpane_win();
	if (!rpane->win) {
		goto error;
	}

	return SUCCESS;

error:
	return ERROR;
}

void fm_ui_display(void) {
		display_fm_pane(lpane);
		display_fm_pane(rpane);
		msg(INFOSTR);
		doupdate();
}

void fm_ui_destroy(void) {
	destroy_fm_pane(lpane);
	destroy_fm_pane(rpane);
}

void mv_pgdown(void) {
	struct fm_pane *pane = get_apane();

	for (int i = 0; i < P_HEIGHT(pane) ; i++) {
		mv_cur_down();
	}
}

void mv_pgup(void) {
	struct fm_pane *pane = get_apane();
	for (int i = 0; i < P_HEIGHT(pane); i++) {
		mv_cur_up();
	}
}

void mv_home(void) {
	struct fm_pane *pane = get_apane();
	pane->cur = 0;
	pane->top = 0;
}

void mv_end(void) {
	struct fm_pane *pane = get_apane();
	for (int i = pane->cur; i < pane->dir->files_num ; i++) {
		mv_cur_down();
	}
}

static inline struct fm_pane * get_apane(void) {
	return lpane->active ? lpane : rpane;
}

