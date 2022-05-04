#include "wg_component.h"

static int editline_changed(void *editline,void *data)
{
	mvhline(0,0,' ',COLS);
	wprintw(desktop.win,"value=%s",wg_editline_value(editline));
	return 0;
}


int main(int argc, char *argv[])
{
	desktop_init(NULL);

	wg_editline_t *editline;
	editline = wg_editline_create("type something:",32);
	wg_signal_connect(editline,changed,editline_changed,NULL);
	wg_editline_put(editline,&desktop,3,6);
	desktop_editing();

	return 0;
}
