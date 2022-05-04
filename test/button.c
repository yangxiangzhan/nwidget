#include "wg_component.h"

static int clicked_exit(void *self,void *data)
{
	return desktop.editing = 0;
}


static int clicked_callback(void *_btn,void *arg)
{
	wg_button_t *btn = (wg_button_t *)_btn;
	mvwhline(desktop.win,0,0,' ',COLS);
	wprintw(desktop.win,"you clicked '%s'!arg=%s",btn->label,arg);
	return 0;
}

int main(int argc, char *argv[])
{
	wg_button_t *btn;

	desktop_init(NULL);

	btn = wg_button_create("button1",-1);
	wg_button_put(btn,&desktop,3,1);
	wg_signal_connect(btn,clicked,clicked_callback,"Hello");

	btn = wg_button_create("button2",16);
	wg_button_put(btn,&desktop,3,10);
	wg_signal_connect(btn,clicked,clicked_callback,"world");

	btn = wg_button3D_create("3Dbutton",-1);
	wg_button_put(btn,&desktop,5,4);
	wg_signal_connect(btn,clicked,clicked_callback,"Hello,World");

	btn = wg_button3D_create("quit",-1);
	wg_button_put(btn,&desktop,5,16);
	wg_signal_connect(btn,clicked,clicked_exit,NULL);

	desktop_editing();
	return 0;
}

