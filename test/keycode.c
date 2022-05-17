#include "wg_component.h"

#ifdef __PDCURSES__
	#define GET_MOUSE(x) nc_getmouse(x)
	#define has_mouse()  1
#else
	#define GET_MOUSE(x) getmouse(x)
	int mouse_keycode(int y,int mousex)
	{
		long RELEASED = NCURSES_MOUSE_MASK(mousex,NCURSES_BUTTON_RELEASED);
		long PRESSED = NCURSES_MOUSE_MASK(mousex,NCURSES_BUTTON_PRESSED);
		long CLICKED = NCURSES_MOUSE_MASK(mousex,NCURSES_BUTTON_CLICKED);
		long DOUBLE_CLICKED = NCURSES_MOUSE_MASK(mousex,NCURSES_DOUBLE_CLICKED);
		long TRIPLE_CLICKED = NCURSES_MOUSE_MASK(mousex,NCURSES_TRIPLE_CLICKED);

		mvwprintw(stdscr,y,0,
			"BUTTON%d:RELEASED=%x,PRESSED=%x,CLICKED=%x,DOUBLE_CLICKED=%x,TRIPLE_CLICKED=%x",
			mousex,RELEASED,PRESSED,CLICKED,DOUBLE_CLICKED,TRIPLE_CLICKED);
		return y;
	}
#endif


int main(int argc, char *argv[])
{
	int y = 0;
	long key;
	desktop_init(NULL);

	#ifdef __PDCURSES__

	do {
		key = getch();
		if (y > LINES-1) {
			werase(stdscr);
			y = 0;
		}
		if (KEY_MOUSE == key) {
			GET_MOUSE(&mouse);
			mvwprintw(stdscr,y++,0,"id=%d,bstate=%x,y=%d,x=%d",
				mouse.id,mouse.bstate,mouse.y,mouse.x);
		} else {
			mvwprintw(stdscr,y++,0,"keycode=%d",key);
		}
	} while(key != 'q');

	#else
	mvwprintw(stdscr,y++,0,"NCURSES_MOUSE_VERSION=%d",NCURSES_MOUSE_VERSION);
	mouse_keycode(y++,1);
	mouse_keycode(y++,2);
	mouse_keycode(y++,3);
	mouse_keycode(y++,4);
	mouse_keycode(y++,5);
	do {
		key = getch();
		if (y > LINES-1) {
			werase(stdscr);
			y = 0;
		}
		if (KEY_MOUSE == key) {
			getmouse(&mouse);
			mvwprintw(stdscr,y++,0,"id=%d,bstate=%x,y=%d,x=%d",
				mouse.id,mouse.bstate,mouse.y,mouse.x);
		} else {
			mvwprintw(stdscr,y++,0,"keycode=%d",key);
		}
	} while(key != 'q');
	#endif
	return endwin();
}