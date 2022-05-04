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


	/* 多个窗体控件测试 */
	struct nwidget form1 , widget1;
	FORM_INIT(&form1,12,24,5,5);
	WIDGET_INIT(&widget1,&form1,6,12,3,3);

	struct nwidget form2 , widget2;
	FORM_INIT(&form2,12,24,8,12);
	WIDGET_INIT(&widget2,&form2,6,12,5,5);

	struct nwidget nform3;
	struct nwidget nwidget3;
	FORM_INIT(&nform3,12,24,12,16);
	WIDGET_INIT(&nwidget3,&nform3,6,12,5,5);

	struct nwidget nform4;
	struct nwidget nwidget4;
	FORM_INIT(&nform4,12,24,5,40);
	WIDGET_INIT(&nwidget4,&nform4,6,12,3,3);

	struct nwidget nform5;
	struct nwidget nwidget5;
	FORM_INIT(&nform5,12,24,8,48);
	WIDGET_INIT(&nwidget5,&nform5,6,12,3,3);

	nform4.found_by_tab = false;
	nform5.found_by_tab = false;

	desktop_editing();
	return 0;
}
