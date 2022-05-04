/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       按钮控件对象及其管理
  ******************************************************************************
  *
  * COPYRIGHT(c) 2021 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "stringw.h"
#include "wg_button.h"

/* Private macro ------------------------------------------------------------*/
/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
static const char btn_tips[] = "button:<Space>/<Enter>:select" ;
/* Global  variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
static int button3D_display(struct button *btn,int pressed,int selected);
static wg_state_t button3D_clicked(struct button *btn);
/* Gorgeous Split-line ------------------------------------------------------*/


/**
  * @brief    按钮显示刷新
  * @param    btn : 目标按钮
  * @param    flags : display flags
  * @param    fmt : 按钮样式
  * @return   0
*/
static int button_display(struct button *btn,int flags,const char *fmt) 
{
	int x;

	if (flags)
		wattron(btn->wg.win,flags);

	mvwhline(btn->wg.win,0,0,' ',btn->wg.width);
	if (btn->label_len) {
		x = (btn->wg.width - btn->label_len) / 2;
		mvwaddstr(btn->wg.win,0,x,btn->label);
	}

	if (fmt) {
		x = btn->wg.width - 1;
		mvwaddch(btn->wg.win,0,0,fmt[0]);
		mvwaddch(btn->wg.win,0,x,fmt[1]);
	}

	if (flags)
		wattroff(btn->wg.win,flags);
	desktop_refresh();
	return 0;
}


/**
  * @brief    按钮被聚焦时的回调函数
  * @param    wg_entry : 目标按钮所在的 wg 控件句柄
  * @return   0
*/
static int button_focus(struct nwidget *wg_entry)
{
	struct button *btn = container_of(wg_entry,struct button,wg);
	if (btn->shadow)
		button3D_display(btn,false,true);
	else
		button_display(btn,A_REVERSE,wgtheme.btn_focus);
	return WG_OK;
}

/**
  * @brief    按钮被聚焦时的回调函数
  * @param    wg_entry : 目标按钮所在的 wg 控件句柄
  * @return   0
*/
static int button_blur(struct nwidget *wg_entry)
{
	struct button *btn = container_of(wg_entry,struct button,wg);
	if (btn->shadow)
		button3D_display(btn,false,false);
	else
		button_display(btn,0,wgtheme.btn_normal);
	return WG_OK;
}


/**
  * @brief    按钮被按下
  * @param    btn : 目标按钮
  * @return   0
*/
static wg_state_t button_clicked(struct button *btn)
{
	/* 激活动画 */
	desktop_lock();
	button_display(btn,0,wgtheme.btn_clicked);
	update_panels();
	doupdate();

	/* 延时 */
	halfdelay(1);
	getch();
	nocbreak(); /* quit halfdelay() */
	cbreak();
	
	/* 恢复状态 */
	button_display(btn,A_REVERSE,NULL);
	desktop_unlock();

	/* 触发用户事件 */
	if (btn->sig.clicked)
		btn->sig.clicked(btn,btn->sig.clicked_arg);
	if (btn->sig.selected)
		btn->sig.selected(btn,btn->sig.selected_arg);
	return WG_OK;
}


/**
  * @brief    按钮鼠标被按下时执行的回调函数，一般仅进行反显
  * @param    wg_entry : 目标按钮所在的 wg 控件句柄
  * @return   WG_OK
*/
static wg_state_t button_mousedown(struct nwidget *wg_entry)
{
	struct button *btn = container_of(wg_entry,struct button,wg);
	return btn->shadow ? button3D_clicked(btn) : button_clicked(btn);
}


/**
  * @brief    按钮鼠标被按下时执行的回调函数，一般仅进行反显
  * @param    wg_entry : 目标按钮所在的 wg 控件句柄
  * @return   WG_OK
*/
static wg_state_t button_clicked_handler(struct nwidget *wg_entry,long key)
{
	struct button *btn = container_of(wg_entry,struct button,wg);
	return btn->shadow ? button3D_clicked(btn) : button_clicked(btn);
}


/**
  * @brief    按钮销毁函数
  * @param    wg_entry : 目标按钮所在的 wg 控件句柄
  * @return   WG_OK
*/
static int button_destroy(struct nwidget *wg_entry)
{
	struct button *btn = container_of(wg_entry,struct button,wg);
	if (btn->created_by == wg_button_create){
		DEBUG_MSG("%s(%p)",__FUNCTION__,wg_entry);
		free(btn);
	}
	return 0;
}


/**
  * @brief    按钮预关闭函数
  * @param    wg : 目标按钮所在的 wg 控件句柄
  * @return   WG_OK
*/
static int button_preclose(struct nwidget *wg)
{
	struct button *btn = container_of(wg,struct button,wg);
	if (btn->sig.closed)
		btn->sig.closed(btn,btn->sig.closed_arg);
	return 0;
}


/**
  * @brief    按钮放置函数，将按钮显示于窗体或控件上
  * @param    btn : 目标按钮
  * @param    parent : 母窗口/母控件
  * @param    y : 母窗口的相对位置
  * @param    x : 母窗口的相对位置
  * @return   成功返回 0
*/
int wg_button_put(struct button *btn,struct nwidget *parent,int y,int x)
{
	int height,width;
	static const struct wghandler btn_handlers[] = {
		{KEY_ENTER,button_clicked_handler},
		{' ',button_clicked_handler},
		{'\n',button_clicked_handler},
		{0,0}
	};

	height = btn->shadow ? 4 : 1;
	width = btn->width + btn->shadow;
	if (widget_init(&btn->wg,parent,height,width,y,x)) {
		return -1;
	}

	handlers_update(&btn->wg,btn_handlers);
	btn->wg.tips = btn_tips;
	btn->wg.focus = button_focus;
	btn->wg.blur = button_blur;
	btn->wg.preclose = button_preclose;
	btn->wg.destroy = button_destroy;
	btn->wg.handle_mouse_event = button_mousedown;

	wbkgd(btn->wg.win,btn->wg.bkg);

	/* display button */
	if (parent)
		btn->wg.blur(&btn->wg);
	return 0;
}


/**
  * @brief    按钮初始化
  * @param    btn : 目标按钮
  * @param    label : 按钮标签
  * @param    width : 按钮显示宽度，为 -1 时取标签宽度
  * @return   成功返回 0
*/
static int wg_button_init(struct button *btn,const char *label,int width)
{
	memset(btn,0,sizeof(struct button));
	if (label) {
		int w = 2 + wstrlen(label);
		if (width < 2)
			width = w;
	}

	if (width > BTN_MAX_WIDTH)
		width = BTN_MAX_WIDTH;
	btn->width = width;
	wstrncpy(btn->label,label,width - 2);
	btn->label_len = wstrlen(btn->label);
	return 0;
}


/**
  * @brief    创建一个按钮
  * @param    label : 按钮标签值
  * @param    width : 按钮显示宽度,<2 时取标签值的显示宽度
  * @return   成功返回按钮句柄
*/
struct button *wg_button_create(const char *label,int width)
{
	struct button *newbtn;
	if (!label && width < 2) {
		return NULL;
	}

	if (NULL == (newbtn = malloc(sizeof(struct button)))) {
		return NULL;
	}

	wg_button_init(newbtn,label,width);
	newbtn->created_by = wg_button_create;
	DEBUG_MSG("%s(%p)",__FUNCTION__,newbtn);
	return newbtn;
}



int wg_button_set_text(struct button *btn,const char *text)
{
	wstrncpy(btn->label,text,btn->width - 2);
	btn->label_len = wstrlen(btn->label);
	if (btn->wg.editing) {
		btn->wg.focus(&btn->wg);
	} else {
		btn->wg.blur(&btn->wg);
	}
	return 0;
}

/* 3D button ------------------------------  */

static int button3D_display(struct button *btn,int pressed,int selected)
{
	int pos,x,y,width = btn->width;

	x = y = pressed!=0;
	werase(btn->wg.win);
	wattron(btn->wg.win,wgtheme.form_fg|A_REVERSE);
	mvwhline(btn->wg.win,y,x,wgtheme.ts,width);
	mvwaddch(btn->wg.win,y,x,wgtheme.tl);
	mvwaddch(btn->wg.win,y,x+width-1,wgtheme.tr);

	y++;
	if (selected) 
		wattroff(btn->wg.win,A_REVERSE);

	mvwhline(btn->wg.win,y,x,' ',width);
	pos = (btn->wg.width - btn->label_len) / 2;
	mvwaddstr(btn->wg.win,y,pos,btn->label);
	
	if (selected) 
		wattron(btn->wg.win,A_REVERSE);
	
	mvwaddch(btn->wg.win,y,x,wgtheme.ls);
	mvwaddch(btn->wg.win,y,x,ACS_VLINE);
	mvwaddch(btn->wg.win,y,x+width-1,wgtheme.rs);

	y++;
	mvwhline(btn->wg.win,y,x,wgtheme.ts,width);
	mvwaddch(btn->wg.win,y,x,wgtheme.bl);
	mvwaddch(btn->wg.win,y,x+width-1,wgtheme.br);
	wattroff(btn->wg.win,wgtheme.form_fg|A_REVERSE);
	
	if (!pressed) {
		/* shadow */
		wattron(btn->wg.win,wgtheme.shadow);
		mvwaddch(btn->wg.win,1,width,' ');
		mvwaddch(btn->wg.win,2,width,' ');
		mvwhline(btn->wg.win,3,1,' ',width);
		wattroff(btn->wg.win,wgtheme.shadow);
	}

	desktop_refresh();
	return 0;
}



/**
  * @brief    按钮被按下
  * @param    btn : 目标按钮
  * @return   0
*/
static wg_state_t button3D_clicked(struct button *btn)
{
	/* 激活动画 */
	desktop_lock();
	button3D_display(btn,true,true);
	update_panels();
	doupdate();

	/* 延时 */
	halfdelay(1);
	getch();
	nocbreak(); /* quit halfdelay() */
	cbreak();
	
	/* 恢复状态 */
	button3D_display(btn,false,true);
	desktop_unlock();

	/* 触发用户事件 */
	if (btn->sig.clicked)
		btn->sig.clicked(btn,btn->sig.clicked_arg);
	if (btn->sig.selected)
		btn->sig.selected(btn,btn->sig.selected_arg);
	return WG_OK;
}


/**
  * @brief    创建一个按钮
  * @param    label : 按钮标签值
  * @param    width : 按钮显示宽度,<2 时取标签值的显示宽度
  * @return   成功返回按钮句柄
*/
struct button *wg_button3D_create(const char *label,int width)
{
	struct button *btn = wg_button_create(label,width);
	if (NULL != btn)
		btn->shadow = true;
	return btn;
}