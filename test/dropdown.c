#include "wg_component.h"

static int dropdown_changed(void *dropdown,void *arg)
{
	mvwhline(stdscr,0,0,' ',COLS);
	printw("you chooce %s!",wg_dropdown_value(dropdown));
	return 0;
}


int main(int argc, char *argv[])
{
	wg_dropdown_t *dropdown;
	wg_menu_t *menu;

	desktop_init(NULL);

	dropdown = wg_dropdown_create("color:",20);
	wg_dropdown_put(dropdown,&desktop,2,10);
	wg_dropdown_item_add(dropdown,"red");
	wg_dropdown_item_add(dropdown,"green");
	wg_dropdown_item_add(dropdown,"blue");
	wg_dropdown_item_add(dropdown,"yellow");
	wg_dropdown_item_add(dropdown,"black");
	wg_signal_connect(dropdown,changed,dropdown_changed,NULL);

	menu = wg_menu_create("my menu");
	wg_menu_put(menu,&desktop,3,10);
	wg_menu_item_add(menu,"apple");
	wg_menu_item_add(menu,"banana");
	wg_menu_item_add(menu,"watermelon");
	wg_signal_connect(menu,changed,dropdown_changed,NULL);

	desktop_editing();
	return 0;
}


