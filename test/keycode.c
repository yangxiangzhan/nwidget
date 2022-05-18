#include "wg_component.h"

#ifdef __PDCURSES__
#define NCURSES_MOUSE_VERSION 2
#endif

#if (NCURSES_MOUSE_VERSION == 1)
#define BUTTON5_RELEASED 0xFFFFFFFF
#define BUTTON5_PRESSED 0xFFFFFFFF
#define BUTTON5_CLICKED 0xFFFFFFFF
#define BUTTON5_DOUBLE_CLICKED 0xFFFFFFFF
#define BUTTON5_TRIPLE_CLICKED 0xFFFFFFFF
#endif

int mouse_keycode(int y,int mousex)
{
	static const long RELEASEDs[] = {
		0,BUTTON1_RELEASED,BUTTON2_RELEASED,BUTTON3_RELEASED,BUTTON4_RELEASED,BUTTON5_RELEASED,
	};
	static const long PRESSEDs[] = {
		0,BUTTON1_PRESSED,BUTTON2_PRESSED,BUTTON3_PRESSED,BUTTON4_PRESSED,BUTTON5_PRESSED,
	};
	static const long CLICKEDs[] = {
		0,BUTTON1_CLICKED,BUTTON2_CLICKED,BUTTON3_CLICKED,BUTTON4_CLICKED,BUTTON5_CLICKED,
	};
	static const long DOUBLE_CLICKEDs[] = {
		0,BUTTON1_DOUBLE_CLICKED,BUTTON2_DOUBLE_CLICKED,BUTTON3_DOUBLE_CLICKED,BUTTON4_DOUBLE_CLICKED,BUTTON5_DOUBLE_CLICKED,
	};
	static const long TRIPLE_CLICKEDs[] = {
		0,BUTTON1_TRIPLE_CLICKED,BUTTON2_TRIPLE_CLICKED,BUTTON3_TRIPLE_CLICKED,BUTTON4_TRIPLE_CLICKED,BUTTON5_TRIPLE_CLICKED,
	};

	long RELEASED = RELEASEDs[mousex];
	long PRESSED = PRESSEDs[mousex];
	long CLICKED = CLICKEDs[mousex];
	long DOUBLE_CLICKED = DOUBLE_CLICKEDs[mousex];
	long TRIPLE_CLICKED = TRIPLE_CLICKEDs[mousex];

	mvwprintw(stdscr,y,0,
		"BUTTON%d:RELEASED=%x,PRESSED=%x,CLICKED=%x,DOUBLE_CLICKED=%x,TRIPLE_CLICKED=%x",
		mousex,RELEASED,PRESSED,CLICKED,DOUBLE_CLICKED,TRIPLE_CLICKED);
	return y;
}


int main(int argc, char *argv[])
{
	int y = 0;
	long key;
	desktop_init(NULL);

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
			GET_MOUSE(&mouse);

			#ifdef MOUSE_MOVED
				if (MOUSE_MOVED)
					mvwprintw(stdscr,y++,0,"MOUSE_MOVED");
			#endif

			mvwprintw(stdscr,y++,0,"id=%d,bstate=%x,y=%d,x=%d",
				mouse.id,mouse.bstate,mouse.y,mouse.x);
		} else {
			mvwprintw(stdscr,y++,0,"keycode=%d",key);
		}
	} while(key != 'q');
	return endwin();
}