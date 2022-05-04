#include "wg_component.h"

static int checkbox_changed(void *pchkbox,void *arg)
{
	wg_checkbox_t *chkbox = (wg_checkbox_t *)pchkbox;
	struct choice *choices = chkbox->choices;
	int row = wg_checkbox_current(chkbox);
	mvwhline(stdscr,0,0,' ',COLS);
	if (choices[row].selected)
		printw("you chooce %s!",choices[row].name);
	else
		printw("you unchooce %s!",choices[row].name);
	return 0;
}

static int radio_changed(void *pchkbox,void *arg)
{
	wg_radio_t *radio = (wg_radio_t *)pchkbox;
	struct choice *choices = radio->choices;
	int row = wg_radio_current(radio);
	mvwhline(stdscr,0,0,' ',COLS);
	printw("you chooce %s!",choices[row].name);
	return 0;
}

int main(int argc, char *argv[])
{
	wg_checkbox_t *chkbox;
	wg_radio_t *radio;

	static struct choice color[] = {
		{"red",1},
		{"green",0},
		{"blue",0},
		{"yellow",0},
		{0,0}
	};

	static struct choice fruit[] = {
		{"apple",1},
		{"banana",1},
		{"malon",0},
		{0,0}
	};

	desktop_init(NULL);

	chkbox = wg_checkbox_create(8,20,TABLE_BORDER,fruit);
	wg_checkbox_put(chkbox,&desktop,2,4);
	wg_signal_connect(chkbox,changed,checkbox_changed,NULL);


	radio = wg_radio_create(8,20,TABLE_BORDER,color);
	wg_radio_put(radio,&desktop,2,24);
	wg_signal_connect(radio,changed,radio_changed,NULL);

	desktop_editing();
	return 0;
}


