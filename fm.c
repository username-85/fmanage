#include "fm.h"
#include "fm_ui.h"
#include "rc.h"

#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <ncurses.h>

static bool fm_term_resize;

static void sh_sigwinch(int unused);
static void term_init(void);
static void term_end(void);

void sh_sigwinch(int unused) {
	(void)unused;
	fm_term_resize = true;
}

void term_init(void) {
	setlocale(LC_ALL, "");
        initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(0);

	set_escdelay(20);
	timeout(FM_NCURSDELAY);
}

void term_end(void) {
        endwin();
}

int fm_prepare(void) {
	term_init();
	refresh();
	return fm_ui_init();
}

void fm_end(void) {
	fm_ui_destroy();
	term_end();
}

void fm_run(void) {
	int ch = 0;
	for (;;) {
		signal(SIGWINCH, sh_sigwinch); 
		if (fm_term_resize) {
			fm_ui_reset();
			fm_term_resize = false;
		}

		fm_ui_display();

		if ((ch = getch()) == ERR) { 
			continue;
		}

		if (KEY_F(10) == ch) {
			break;
		}
		else if (KEY_DOWN == ch) {
			mv_cur_down();
		}
		else if (KEY_UP == ch) {
			mv_cur_up();
		}
		else if (KEY_NPAGE == ch) {
			mv_pgdown();
		}
		else if (KEY_PPAGE == ch) {
			mv_pgup();
		}
		else if (KEY_HOME == ch) {
			mv_home();
		}
		else if (KEY_END == ch) {
			mv_end();
		}
		else if ('\t' == ch) {
			switch_pane();
		}
		else if ('\n' == ch) {
			if (open_file() != SUCCESS) {
				msg_rev("Could not open");
				getch();
			}
		}
	}
}

