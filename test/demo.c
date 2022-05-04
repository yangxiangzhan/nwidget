#include "wg_component.h"

wg_textarea_t *dbgbox;
static struct choice tabflags[] = {
	{"border",true},
	{"footer",true},
	{"column title",true},
	{"column hide",true},
	{"scrollbar",true},
	{0,0}
};



static int demo_form_create(const char *title,struct nwidget *parent)
{
	static int y = 2;
	static int x = 56;
	int options;
	wg_form_t *form;
	wg_table_t *table;
	form = wg_form_create(title,12,32,WG_SHADOW);
	if (0 != wg_form_put(form,parent,y,x)) {
		form->wg.destroy(&form->wg);
		return -1;
	}

	if (parent) 
		wg_form_set(form,true,true);

	y += 2;
	x += 2;
	if (y + 12 >= LINES)
		y = 2;
	if (x + 24 >= COLS)
		x = 56;
	
	options = 0;
	if (tabflags[0].selected)
		options |= TABLE_BORDER;
	if (tabflags[1].selected)
		options |= TABLE_FOOTER;
	if (tabflags[2].selected)
		options |= TABLE_TITLE;
	if (tabflags[3].selected)
		options |= TABLE_COL_HIDE;
	
	table = wg_table_create(9,30,options);
	wg_table_put(table,&form->wg,2,1);
	if (tabflags[4].selected)
		wg_table_set_scrollbar(table);
	wg_table_column_add(table,"ID",6);
	wg_table_column_add(table,"name",16);
	wg_table_column_add(table,"value",16);

	char *red[3] = {"1","red","255,0,0"};
	char *green[3] = {"2","green","0,255,0"};
	char *blue[3] = {"3","blue","0,0,255"};
	char *black[3] = {"4","black","0,0,0"};
	char *white[3] = {"5","white","255,255,255"};
	char *yellow[3] = {"6","yellow","255,255,0"};
	char *cyan[3] = {"7","cyan","0,255,255"};
	char *magenta[3] = {"8","magenta","228,0,127"};
	wg_table_item_add(table,red);
	wg_table_item_add(table,green);
	wg_table_item_add(table,blue);
	wg_table_item_add(table,black);
	wg_table_item_add(table,white);
	wg_table_item_add(table,yellow);
	wg_table_item_add(table,cyan);
	wg_table_item_add(table,magenta);

	return 0;
}


static int normal_clicked(void *_btn,void *arg)
{
	const char *title = wg_editline_value(arg);
	wg_textarea_sprintf(dbgbox,"create a normal form\n");
	return demo_form_create(title,&desktop);
}

static int title_input(void *editline,void *arg)
{
	wg_textarea_sprintf(dbgbox,"input:%s\n",wg_editline_value(editline));
	return 0;
}

static int tabflags_changed(void *pchkbox,void *arg)
{
	wg_checkbox_t *chkbox = (wg_checkbox_t *)pchkbox;
	int row = wg_checkbox_current(chkbox);
	const char *res = tabflags[row].selected ? "enable" : "disable";
	wg_textarea_sprintf(dbgbox,"%s %s\n",res,tabflags[row].name);
	return 0;
}

static int clicked_exit(void *self,void *data)
{
	return desktop.editing = 0;
}


int main(int argc, char *argv[])
{
	wg_form_t *form;
	wg_button_t *btn;
	wg_editline_t *editline;
	wg_checkbox_t *chkbox;


	/* init ncurses */
	desktop_init(NULL);
	desktop_taskbar_init();

	/* create first form */
	form = wg_form_create("window1",20,48,WG_SHADOW);
	wg_form_put(form,&desktop,2,4);

	editline = wg_editline_create("new form:",30);
	wg_editline_put(editline,&form->wg,2,2);
	wg_signal_connect(editline,changed,title_input,NULL);
	
	chkbox = wg_checkbox_create(7,30,TABLE_BORDER,tabflags);
	wg_checkbox_put(chkbox,&form->wg,3,2);
	wg_signal_connect(chkbox,changed,tabflags_changed,NULL);
	mvwaddstr(chkbox->wg.win,0,1," table option ");

	btn = wg_button3D_create("create",-1);
	wg_button_put(btn,&form->wg,1,36);
	wg_signal_connect(btn,clicked,normal_clicked,editline);

	btn = wg_button3D_create(" quit ",-1);
	wg_button_put(btn,&form->wg,6,36);
	wg_signal_connect(btn,clicked,clicked_exit,NULL);

	dbgbox = wg_textarea_create(9,44,1024,TEXT_BORDER);
	wg_textarea_put(dbgbox,&form->wg,10,2);
	wg_textarea_set_scrollbar(dbgbox);

	desktop_editing();
	return 0;
}

