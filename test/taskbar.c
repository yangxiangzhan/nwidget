#include "wg_component.h"

static int count;

static int clicked_callback(void *_btn,void *arg)
{
	static int y = 1;
	static int x = 10;

	struct form *form;
	char title[64];
	sprintf(title,"FORM%d",++count);
	form = wg_form_create(title,12,24,0);
	wg_form_put(form,&desktop,y,x);
	wg_form_set(form,true,true);

	if (++y + 12 >= LINES) 
		y = 1;
	if (++x + 24 >= COLS)
		x = 10;
	return 0;
}


int main(int argc, char *argv[])
{
	wg_button_t *btn;

	desktop_init(NULL);
	desktop_taskbar_init();

	btn = wg_button3D_create("create form",-1);
	wg_button_put(btn,&desktop,1,1);
	wg_signal_connect(btn,clicked,clicked_callback,"Hello,World");

	desktop_editing();
	return 0;
}

