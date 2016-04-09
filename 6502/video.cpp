//
// Kind of a temporary file to deal with basic text I/O for now.  Will
// use PDCurses for development
//

#include "curses.h"
#include "video.h"

chtype flag = 0;

void init_text_screen()
{
	initscr();
}

void clear_screen()
{
	clear();
}

void move_cursor(int row, int col)
{
	move(row, col);
}

void set_raw(bool state)
{
	if (state == true) {
		raw();
		noecho();
		scrollok(stdscr, 1);
		timeout(0);
	} else {
		noraw();
		echo();
		timeout(-1);
	}
}

void output_char(char c)
{
	echochar(flag | c);
}


void screen_bottom()
{
	move_cursor(MAX_LINES,0);
}

void scroll_screen()
{
	scrl(1);
}

void set_normal()
{
	flag = A_NORMAL;
}

void set_inverse()
{
	flag = A_REVERSE;
}

void set_flashing()
{
	flag = A_BLINK;
}
