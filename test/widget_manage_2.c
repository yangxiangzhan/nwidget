#include "wg_component.h"

#define WIDGET_INIT(_wg,_f,_L,_W,_y,_x)\
do{\
	widget_init(_wg,_f,_L,_W,_y,_x);\
	box((_wg)->win,0,0);\
	(_wg)->focus = my_focus;\
	(_wg)->blur = my_blur;\
}while(0)

#define FORM_INIT(_fwg,_L,_W,_y,_x)  \
do{\
	widget_init(_fwg,&desktop,_L,_W,_y,_x);\
	box((_fwg)->win,0,0);\
	(_fwg)->focus = my_focus;\
	(_fwg)->blur = my_blur;\
}while(0)

#define NFORM_INIT(_fwg,_f,_L,_W,_y,_x)  \
do{\
	widget_init(_fwg,_f,_L,_W,_y,_x);\
	box((_fwg)->win,0,0);\
	(_fwg)->focus = my_focus;\
	(_fwg)->blur = my_blur;\
}while(0)


static int my_blur(struct nwidget *self)
{
	char focus_info[128];
	sprintf(focus_info," %02d",self->focus_time);
	mvwaddstr(self->win,1,1,focus_info);
	desktop_refresh();
	return 0;
}

static int my_focus(struct nwidget *self)
{
	char focus_info[128];
	sprintf(focus_info,"*%02d",self->focus_time);
	mvwaddstr(self->win,1,1,focus_info);
	desktop_refresh();
	return 0;
}

int main(int argc, char *argv[])
{
	desktop_init(NULL);

	/* 多级窗体控件测试 */
	struct nwidget nform1;
	struct nwidget nform2;
	struct nwidget nform3;
	struct nwidget nwidget1;
	struct nwidget nwidget2;
	struct nwidget nwidget3;
	struct nwidget nwidget4;
	
	WIDGET_INIT(&nwidget1,&desktop,5,10,2,2);

	FORM_INIT(&nform1,18,40,2,12);
	NFORM_INIT(&nform2,&nform1,10,20,2,2);
	WIDGET_INIT(&nwidget2,&nform2,5,10,2,2);

	WIDGET_INIT(&nwidget3,&nform1,5,10,12,2);
	
	FORM_INIT(&nform3,10,20,12,42);
	WIDGET_INIT(&nwidget4,&nform3,6,12,3,3);

	desktop_editing();
	return 0;
}
