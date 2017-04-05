#include "fm.h"

#include "fm_action.h"
#include "fm_ui.h"
#include "rc.h"

#include <limits.h>
#include <locale.h>
#include <ncurses.h>
#include <signal.h>
#include <unistd.h>

static char startdir[PATH_MAX];
static bool fm_term_resize;

static void sh_sigwinch(int unused);
static void term_init(void);
static void term_end(void);

int fm_prepare(void)
{
        getcwd(startdir, PATH_MAX);
	term_init();
	refresh();
	return fm_ui_init();
}

void fm_run(void)
{
	int ch = 0;
	bool inloop = true;
	while(inloop) {
		signal(SIGWINCH, sh_sigwinch); 
		if (fm_term_resize) {
			fm_ui_reset();
			fm_term_resize = false;
		}
		fm_ui_display();

		switch( ch = getch() ) {
		case KEY_F(10):
			inloop = false;
			break;
		case KEY_F(5):
			fm_copy();
			break;
		case KEY_DOWN:
			mv_cur_down();
			break;
		case KEY_UP:
			mv_cur_up();
			break;
		case KEY_NPAGE:
			mv_pgdown();
			break;
		case KEY_PPAGE:
			mv_pgup();
			break;
		case KEY_HOME:
			mv_home();
			break;
		case KEY_END:
			mv_end();
			break;
		case '\t':
			switch_pane();
			break;
		case '\n':
			if (fm_open() != SUCCESS)
				msg_win("Could not open");
			break;
		default:
			;
		}
	}
}

void fm_end(void)
{
	fm_ui_destroy();
	term_end();
	chdir(startdir);
}

static void sh_sigwinch(int unused)
{
	(void)unused;
	fm_term_resize = true;
}

static void term_init(void)
{
	setlocale(LC_ALL, "");
        initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(0);

	set_escdelay(20);
	timeout(FM_NCURSDELAY);
}

static void term_end(void)
{
        endwin();
}

