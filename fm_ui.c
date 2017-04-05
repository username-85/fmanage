#include "fm_ui.h"
#include "fm_dir.h"
#include "fm_progress.h"
#include "rc.h"

#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#define P_MINY(pane) (getbegy(pane->win) + 1)
#define P_MAXY(pane) (getmaxy(pane->win) - 2)
#define P_MINX(pane) (1)
#define P_MAXX(pane) (getmaxx(pane->win) - 2)
#define P_ROWLEN(pane) (P_MAXX(pane) - P_MINX(pane))
#define P_HEADERLEN(pane) (P_MAXX(pane) - P_MINX(pane) - 1)
#define P_HEIGHT(pane) (P_MAXY(pane) - P_MINY(pane))

#define INFOSTR "F5-Copy F10-Quit"

static struct fm_pane *lpane;
static struct fm_pane *rpane;
static WINDOW *info_win;
static WINDOW *progress_win;

static struct fm_pane* create_fm_pane(void); 
static void destroy_fm_pane(struct fm_pane *);
static void display_fm_pane(struct fm_pane * pane);

static WINDOW * create_lpane_win();
static WINDOW * create_rpane_win();
static WINDOW * create_info_win();

static void create_progress_win(void);
static void destroy_progress_win(void); 

void switch_pane(void)
{
	if (rpane->active) {
		rpane->active = false;
		lpane->active = true;
	} else {
		rpane->active = true;
		lpane->active = false;
	}

	chdir(get_fm_adir_fpath());
}

void mv_cur_down(void)
{
	struct fm_pane *pane = get_fm_apane();

	int new_cur = pane->cur + 1;
	if (new_cur >= pane->dir->files_num)
		return;

	pane->cur = new_cur;

	if (new_cur >= pane->top + P_MAXY(pane))
		pane->top++;
}

void mv_cur_up(void)
{
	struct fm_pane *pane = get_fm_apane();
	if (pane->cur == 0)
		return;

	pane->cur--; 

	if (pane->cur < pane->top)
		pane->top = pane->cur;
}


void msg(char const * msg)
{
	werase(info_win);
	waddnstr(info_win, msg, COLS);
	wnoutrefresh(info_win);
}

void msg_rev(char const * msg)
{
	werase(info_win);
	wattron(info_win, A_REVERSE);
	waddnstr(info_win, msg, COLS);
	wattroff(info_win, A_REVERSE);
	wrefresh(info_win);
}

void fm_ui_reset(void)
{
	endwin();
	refresh();
	clear();
	lpane->win = create_lpane_win();
	rpane->win = create_rpane_win();
	info_win = create_info_win();
}

int fm_ui_init(void)
{
	info_win = create_info_win();
	if (!info_win)
		return ERROR;
	msg(INFOSTR);

	lpane = create_fm_pane();
	if (!lpane)
		return ERROR;

	lpane->win = create_lpane_win();
	if (!lpane->win) 
		return ERROR;
	
	lpane->active = true;

	rpane = create_fm_pane();
	if (!rpane) 
		return ERROR;
	
	rpane->win = create_rpane_win();
	if (!rpane->win) 
		return ERROR;
	
	return SUCCESS;
}

void fm_ui_display(void)
{
	display_fm_pane(lpane);
	display_fm_pane(rpane);
	msg(INFOSTR);
	doupdate();
}

void fm_ui_destroy(void)
{
	destroy_fm_pane(lpane);
	destroy_fm_pane(rpane);
}

void mv_pgdown(void)
{
	struct fm_pane *pane = get_fm_apane();

	for (int i = 0; i < P_HEIGHT(pane) ; i++)
		mv_cur_down();
}

void mv_pgup(void)
{
	struct fm_pane *pane = get_fm_apane();

	for (int i = 0; i < P_HEIGHT(pane); i++)
		mv_cur_up();
}

void mv_home(void)
{
	struct fm_pane *pane = get_fm_apane();
	pane->cur = 0;
	pane->top = 0;
}

void mv_end(void)
{
	struct fm_pane *pane = get_fm_apane();

	for (int i = pane->cur; i < pane->dir->files_num ; i++)
		mv_cur_down();
}

struct fm_pane * get_fm_apane(void)
{
	return lpane->active ? lpane : rpane;
}

struct fm_file * get_fm_afile(void)
{
	struct fm_pane *pane = get_fm_apane();
	return pane->dir->files + pane->cur;
}

struct fm_dir * get_fm_adir(void)
{
	struct fm_pane *pane = get_fm_apane();
	return pane->dir;
}

char * get_fm_adir_fpath(void)
{
	struct fm_dir *adir = get_fm_adir();
	return adir->path;
}

char * get_afile_fpath(void)
{
	char *path = NULL;

	struct fm_file *fmfile = get_fm_afile();
	char *fname = fmfile->name;
	char *dir = get_fm_adir_fpath();

	if (fmfile->is_dir && (strcmp(fname, "..") == 0)) {
		path = strdup(dir);
		dirname(path);
	} else {
		//                         '/'                 '\0'
		path = malloc(strlen(dir) + 1 + strlen(fname) + 1); 
		if (strcmp(dir, "/") == 0)
			sprintf(path, "%s%s", dir, fname);
		else
			sprintf(path, "%s%s%s", dir, "/", fname);
	}

	return path;
}

struct fm_pane * get_fm_pane(void)
{
	return !lpane->active ? lpane : rpane;
}

struct fm_dir * get_fm_dir(void)
{
	struct fm_pane *pane = get_fm_pane();
	return pane ? pane->dir : NULL;
}

char * get_fm_dir_fpath(void)
{
	struct fm_dir *dir = get_fm_dir();
	return dir ? dir->path : NULL;
}

void msg_win(char const * msg)
{
	int wlines = 3;
	int topborder = 1;
	WINDOW *win = newwin(wlines, COLS - 2,
	                     (LINES / 2) - (wlines / 2) - topborder, 1);
	box(win, 0, 0);
	wmove(win, 1, 1);
	wprintw(win, msg);
	wnoutrefresh(win);
	doupdate();
	getch();

	werase(win);
	wnoutrefresh(win);
	doupdate();
	delwin(win);
	flushinp();
}

void display_progress(void) 
{
	if (!progress_win) {
		create_progress_win();
		box(progress_win, 0, 0);
	}

	size_t total_bytes = fm_progress_get_total_bytes();
	if (total_bytes) {
		size_t bytes = fm_progress_get_bytes();
		size_t maxx = getmaxx(progress_win) - 1;
		size_t bar_filled = maxx * ((double) bytes / total_bytes);

		wmove(progress_win, 1, 1);
		wattron(progress_win, A_REVERSE);
		for (size_t i = 0; i < bar_filled; i++)
			waddch(progress_win, ' ');

		wattroff(progress_win, A_REVERSE);
	}

	wnoutrefresh(progress_win);
	doupdate();
}

void display_progress_end(void) 
{
	destroy_progress_win(); 
	flushinp();
}

static struct fm_pane* create_fm_pane(void)
{
	struct fm_pane *pane = calloc(1, sizeof(struct fm_pane));
	if (!pane)
		goto error;
	
	char cwd[PATH_MAX];
	if (!getcwd(cwd, PATH_MAX))
		goto error;

	struct fm_dir *dir = create_fm_dir(cwd);
	if (!dir)
		goto error;

	pane->dir = dir;
	return pane;

error:
	free(pane);
	return NULL;
}

static void destroy_fm_pane(struct fm_pane * pane)
{
	if (pane) {
		destroy_fm_dir(pane->dir);
		free(pane);
	}
}

static void display_fm_pane(struct fm_pane * pane)
{
	if (!pane) 
		return;

	werase(pane->win);
	wattroff(pane->win, A_REVERSE);
	box(pane->win, 0, 0);

	if (pane->active) 
		wattron(pane->win, A_REVERSE);

	wmove(pane->win, 0, 2);
	wprintw(pane->win, "%.*s", P_HEADERLEN(pane),  pane->dir->path);

	if (pane->active) 
		wattroff(pane->win, A_REVERSE);

	if (pane->dir->files_num < pane->top)
		pane->top = 0;

	if (pane->dir->files_num < pane->cur)
		pane->cur = 0;

	struct fm_file *files = pane->dir->files + pane->top;
	struct fm_file *cur_file = pane->dir->files + pane->cur;
	for (int y = 1; y <= pane->dir->files_num && y <= P_MAXY(pane); y++) {
		
		if (files == cur_file && pane->active)
			wattron(pane->win, A_REVERSE);
		else
			wattroff(pane->win, A_REVERSE);

		wmove(pane->win, y, 1);
		if (files->is_dir)
			waddch(pane->win, '/');

		wprintw(pane->win, "%.*s", P_ROWLEN(pane), files->name);

		if (files == cur_file && pane->active) {
			int cur_x = getcurx(pane->win);
			for (int i = cur_x; i < P_MAXX(pane); i++) 
				waddch(pane->win, ' ');
		}
		files++;
	}

	wnoutrefresh(pane->win);
}

static WINDOW * create_lpane_win()
{
	return newwin(LINES-1, COLS / 2, 0, 0);
}

static WINDOW * create_rpane_win()
{
	int ost = COLS % 2;
	return newwin(LINES-1, COLS / 2, 0, COLS / 2 + ost);
}

static WINDOW * create_info_win()
{
	return newwin(1, COLS, LINES-1, 0);
}

static void create_progress_win(void) 
{
	int wlines = 3;
	int topborder = 1;
	progress_win = newwin(wlines, COLS - 2,
	                      (LINES / 2) - (wlines / 2) - topborder, 1);
}

static void destroy_progress_win(void) 
{
	if (progress_win) {
		werase(progress_win);
		wnoutrefresh(progress_win);
		doupdate();

		delwin(progress_win);
		progress_win = NULL;
	}
}

