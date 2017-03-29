#ifndef FM_UI_H 
#define FM_UI_H 

#include <ncurses.h>
#include <stdbool.h>

struct fm_pane {
	bool active;
	WINDOW *win;
	struct fm_dir *dir;
	int cur; //index for dir->files 
	int top; //index for dir->files 
};

void switch_pane(void);
void mv_cur_down(void);
void mv_cur_up(void);
void msg(char const * msg);
void msg_rev(char const * msg);
void fm_ui_reset(void);
int fm_ui_init(void);
void fm_ui_display(void);
void fm_ui_destroy(void);
void mv_pgdown(void);
void mv_pgup(void);
void mv_home(void);
void mv_end(void);
struct fm_pane * get_apane(void);
struct fm_file * get_afile(void);
struct fm_dir * get_adir(void);
char * get_adir_fpath(void);
char * get_afile_fpath(void);

#endif
