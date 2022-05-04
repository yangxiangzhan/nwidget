/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       弹出式窗体对象及其管理
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
#include "wg_form.h"
#include "wg_mutex.h"

/* Private macro ------------------------------------------------------------*/
/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Global  variables --------------------------------------------------------*/
int __attribute__((weak)) desktop_taskbar_register(struct form *form){ return 0;}
void __attribute__((weak)) desktop_taskbar_delete(struct form *form){}
void __attribute__((weak)) desktop_taskbar_select(struct form *form){}
void __attribute__((weak)) desktop_taskbar_unselect(struct form *form){}
/* Private function prototypes ----------------------------------------------*/
/* Gorgeous Split-line ------------------------------------------------------*/

/**
  * @brief    form3D被聚焦时的回调函数
  * @param    wg_entry : 目标按钮所在的 wg 控件句柄
  * @return   0
*/
static int form_focus(struct nwidget *wg)
{
	struct form *form = container_of(wg, struct form,wg);
	if (form->shadow) {
		top_panel(form->pr);
		top_panel(form->pb);
		desktop_refresh();
	}

	if (form->titlew && wgtheme.form_fg) {
		wattron(form->wg.win,wgtheme.form_fg);
		mvwaddstr(form->wg.win,0,3,form->title);
		wattroff(form->wg.win,wgtheme.form_fg);
		desktop_refresh();
	}

	if (form->wg.parent == &desktop) {
		desktop_taskbar_select(form);
	}
	return WG_OK;
}


/**
  * @brief    单个 form 的隐藏/显示函数
  * @param    wg : wg 控件句柄
  * @param    hide : 不为0时执行隐藏函数
  * @return   0
*/
static int form_do_hide(struct nwidget *wg,int hide)
{
	struct form *form = container_of(wg, struct form,wg);
	desktop_lock();
	if (hide) {
		hide_panel(wg->panel);
		if (form->shadow) {
			hide_panel(form->pb);
			hide_panel(form->pr);
		}
	} else {
		show_panel(form->wg.panel);
		if (form->shadow) {
			show_panel(form->pb);
			show_panel(form->pr);
		}
		form->wg.focus_time = desktop.focus_time;
	}
	wg->hidden = hide != 0;
	desktop_unlock();
	return 0;
}


/**
  * @brief    递归隐藏/显示 wg 下的所有子控件
  * @param    wg : wg 控件句柄
  * @param    hide : 隐藏或显示
  * @return   0
*/
static int wg_do_hide(struct nwidget *wg,int hide)
{
	if (wg->hide)
		wg->hide(wg,hide);
	wg->hidden = hide != 0;
	for (wg = wg->sub; wg; wg = wg->next) {
		wg_do_hide(wg,hide);
	}
	return 0;
}


/**
  * @brief    隐藏/显示 form
  * @param    form : 控件句柄
  * @param    hide : 不为0时执行隐藏函数
  * @return   0
*/
int wg_form_hide(struct form *form,int hide)
{
	wg_do_hide(&form->wg,hide);
	
	/* 如果是隐藏窗体且焦点在当前窗体上，聚焦于母窗体 */
	if (hide && form->wg.editing && form->wg.parent){
		desktop_focus_on(form->wg.parent);
	}
	return 0;
}


/**
  * @brief    form3D被聚焦时的回调函数
  * @param    wg_entry : 目标按钮所在的 wg 控件句柄
  * @return   0
*/
static int form_blur(struct nwidget *wg)
{
	struct form *form = container_of(wg, struct form,wg);
	if (form->titlew) {
		mvwaddstr(form->wg.win,0,3,form->title);
		desktop_refresh();
	}

	if (form->wg.parent == &desktop) {
		desktop_taskbar_unselect(form);
	}
	return WG_OK;
}

/**
 * @brief 移动控件面板
 * @param wg : 控件的 wg 入口
 * @param y : y offset
 * @param x : x offset
 * @return 0
 */
static int wg_move_panel(struct nwidget *wg,int y,int x)
{
	int (*wg_move)(struct nwidget *self,int y_offset,int x_offset);
	if ( (wg_move = wg->move) ){
		wg_move(wg,y,x);
	}

	for (wg = wg->sub; wg; wg = wg->next){
		wg_move_panel(wg,y,x);
	}
	return 0;
}

/**
  * @brief    表格 page down 、右箭头的默认响应函数
  * @param    wg : 目标表格所在的 wg 控件句柄
  * @param    shortcut : 键盘键值
*/
static wg_state_t form_move_down(struct nwidget *wg,long shortcut)
{
	struct form *form = container_of(wg, struct form,wg);
	if (form->wg.rely + form->height + 1 < LINES) {
		if (form->shadow) {
			move_panel(form->pr,wg->rely+1+1,wg->relx+wg->width);
			move_panel(form->pb,wg->rely+wg->height+1,wg->relx+1);
		}
		wg_move_panel(wg,1,0);
		desktop_refresh();
	}
	return WG_OK;
}

/**
  * @brief    表格 page down 、右箭头的默认响应函数
  * @param    wg : 目标表格所在的 wg 控件句柄
  * @param    shortcut : 键盘键值
*/
static wg_state_t form_move_up(struct nwidget *wg,long shortcut)
{
	struct form *form = container_of(wg, struct form,wg);
	if (form->wg.rely > 0) {
		if (form->shadow) {
			move_panel(form->pr,wg->rely,wg->relx+wg->width);
			move_panel(form->pb,wg->rely+wg->height-1,wg->relx+1);
		}
		wg_move_panel(wg,-1,0);
		desktop_refresh();
	}
	return WG_OK;
}

/**
  * @brief    表格 page down 、右箭头的默认响应函数
  * @param    wg : 目标表格所在的 wg 控件句柄
  * @param    shortcut : 键盘键值
*/
static wg_state_t form_move_right(struct nwidget *wg,long shortcut)
{
	struct form *form = container_of(wg, struct form,wg);
	if (form->wg.relx + form->wg.width + 1 < COLS) {
		if (form->shadow) {
			move_panel(form->pr,wg->rely+1,wg->relx+wg->width+1);
			move_panel(form->pb,wg->rely+wg->height,wg->relx+1+1);
		}
		wg_move_panel(wg,0,1);
		desktop_refresh();
	}
	return WG_OK;
}


/**
  * @brief    表格 page down 、右箭头的默认响应函数
  * @param    wg : 目标表格所在的 wg 控件句柄
  * @param    shortcut : 键盘键值
*/
static wg_state_t form_move_left(struct nwidget *wg,long shortcut)
{
	struct form *form = container_of(wg, struct form,wg);
	if (form->wg.relx) {
		if (form->shadow) {
			move_panel(form->pr,wg->rely+1,wg->relx+wg->width-1);
			move_panel(form->pb,wg->rely+wg->height,wg->relx);
		}
		wg_move_panel(wg,0,-1);
		desktop_refresh();
	}
	return WG_OK;
}


/**
  * @brief    窗体销毁函数，删除一个窗体并释放内存
  * @param    self : wg 入口
  * @return   0
*/
static int form_destroy(struct nwidget *self)
{
	struct form *form = container_of(self, struct form,wg);

	if (form->pr)
		del_panel(form->pr);
	if (form->pb)
		del_panel(form->pb);
	form->pb = form->pr = NULL;
	
	if (form->wr)
		delwin(form->wr);
	if (form->wb)
		delwin(form->wb);
	form->wb = form->wr = NULL;

	//if (form->wg.parent == &desktop) {
		desktop_taskbar_delete(form);
	//}

	if (form->created_by == wg_form_create) {
		free(form);
	}
	return 0;
}

static int form_put(struct form *form,struct nwidget *parent,int y,int x,int bkg)
{
	/* widget_init 会清空 wg  */
	if (widget_init(&form->wg,parent,form->height,form->width,y,x)) {
		return -1;
	}

	if (!(form->wg.bkg = bkg))
		form->wg.bkg = wgtheme.form_bkg ? wgtheme.form_bkg : 0;
	form->wg.tips = form->tips;
	form->wg.destroy = form_destroy;
	form->wg.focus = form_focus;
	form->wg.blur = form_blur;
	form->wg.hide = form_do_hide;
	if (form->wg.bkg)
		wbkgd(form->wg.win,form->wg.bkg);
	
	wg_show_border(form->wg.win);
	for (int y = 1; y < form->height-1;y++)
		mvwhline(form->wg.win,y,1,' ',form->width-2);
	
	if (form->titlew) {
		mvwhline(form->wg.win,0,1,' ',form->titlew+4);
		mvwaddstr(form->wg.win,0,3,form->title);
		if (wgtheme.ts == ACS_HLINE) {
			mvwaddch(form->wg.win,0,1,ACS_RTEE);
			mvwaddch(form->wg.win,0,form->titlew+4,ACS_LTEE);
		} else {
			mvwaddch(form->wg.win,0,1,'|');
			mvwaddch(form->wg.win,0,form->titlew+4,'|');
		}
	}

	if (form->shadow) {
		/* 阴影特效主要分为右边框阴影和下边框阴影 */
		y = form->wg.rely+1;
		x = form->wg.relx + form->wg.width;
		form->wr = newwin(form->height,1,y,x);
		form->pr = new_panel(form->wr);

		y = form->wg.rely + form->wg.height;
		x = form->wg.relx + 1;
		form->wb = newwin(1,form->width-1,y,x);
		form->pb = new_panel(form->wb);

		if (wgtheme.shadow) {
			wbkgd(form->wr,wgtheme.shadow);
			wbkgd(form->wb,wgtheme.shadow);
		}
	}

	if (form->movable) {
		static const struct wghandler table_handlers[] = {
			{KEY_CTRL_DOWN,form_move_down},
			{KEY_CTRL_UP,form_move_up},
			{KEY_CTRL_RIGHT,form_move_right},
			{KEY_CTRL_LEFT,form_move_left},
			{'k',form_move_down},
			{'i',form_move_up},
			{'l',form_move_right},
			{'j',form_move_left},
			{0,0}
		};
		handlers_update(&form->wg,table_handlers);
	}

	if (parent == &desktop) {
		desktop_taskbar_register(form);
	}
	return 0;
}


/**
  * @brief    窗体放置函数，窗体显示
  * @param    form : 窗体
  * @param    parent : 母窗口/母控件
  * @param    y : 母窗口的相对位置
  * @param    x : 母窗口的相对位置
  * @return   成功返回 0
*/
int wg_form_put(struct form *form,struct nwidget *parent,int y,int x)
{
	return form_put(form,parent,y,x,0);
}


/**
  * @brief    创建一个窗体
  * @param    title : 窗体标题
  * @param    height : 窗体高度
  * @param    width : 窗体宽度
  * @param    flags : WG_SHADOW/WG_UNMOVABLE
  * @return   成功窗体按钮句柄
*/
struct form *wg_form_create(const char *title,int height,int width,int flags)
{
	#if (KEY_CTRL_UP == KEY_SR)
		static const char tips[] = "form:<shift + ARROW>/<i,k,j,l>:MOVE";
	#else
		static const char tips[] = "form:<Ctrl+ARROW>/<i,k,j,l>:MOVE";
	#endif

	int w;
	struct form *form = malloc(sizeof(struct form));

	memset(form,0,sizeof(struct form));

	form->created_by = wg_form_create;
	form->shadow = (flags & WG_SHADOW) != 0;
	form->movable = !(flags & WG_UNMOVABLE);
	form->width = width;
	form->height = height;
	
	if (title && title[0]) {
		w = wstrlen(title);
		if (w >= sizeof(form->title)-1)
			w = sizeof(form->title)-1;
		if (w >= width-8)
			w = width-8;
		wstrncpy(form->title,title,w);
		form->titlew = w;
		if (form->movable)
			snprintf(form->tips,sizeof(form->tips)-1,"%s |%s",tips,form->title);
		else
			snprintf(form->tips,sizeof(form->tips)-1,"form:%s",form->title);
	} else {
		if (form->movable)
			snprintf(form->tips,sizeof(form->tips)-1,"%s |%p",tips,form);
		else
			snprintf(form->tips,sizeof(form->tips)-1,"form:%p",form);
	}

	return form;
}


/**
  * @brief    创建一个弹出式的窗体
  * @param    height : 窗体高度
  * @param    width : 窗体宽度
  * @param    rely : 窗体坐标
  * @param    relx : 窗体坐标
  * @note     创建的窗体失去焦点时会自动删除
  * @return   成功窗体按钮句柄
*/
struct form *wg_popup_create(int height,int width,int rely,int relx)
{
	struct form *form;
	form = wg_form_create(NULL,height,width,0);
	if (form) 
		wg_form_put(form,NULL,rely, relx);
	return form;
}


static int toast_edit(struct nwidget *wg,long key)
{
	wg = wg->parent;
	
	desktop_focus_on(wg);
	for (int i = 0; i < MAX_HANDLERS && wg->shortcuts[i]; i++) {
		if (key == wg->shortcuts[i])
			return wg->handlers[i](wg,key);
	}

	if (wg->edit) 
		return wg->edit(wg,key);
	return WG_OK;
}

int wg_msgbox(const char *msg)
{
	#define BOX_HEIGHT 12
	#define BOX_WIDTH  40

	static void *mutex = NULL;
	static struct form toast = {0};
	int y,x;
	toast.height = BOX_HEIGHT;
	toast.width = BOX_WIDTH;
	if (!mutex)
		NWIDGET_MUTEX_INIT(mutex);

	NWIDGET_MUTEX_LOCK(mutex);
	if (!toast.wg.has_been_placed){
		form_put(&toast,NULL,1,COLS-toast.width,wgtheme.form_fg|A_REVERSE);
		toast.wr = derwin(toast.wg.win,BOX_HEIGHT-2,BOX_WIDTH-2,1,1);
		memset(&toast.wg.shortcuts,0,sizeof(toast.wg.shortcuts));
		toast.wg.edit = toast_edit;
	}

	mvwaddstr(toast.wr,0,0,msg);
	y = getcury(toast.wr);
	x = getcurx(toast.wr);

	x = y ? 0 : (BOX_WIDTH - 2 - x) / 2;
	if (y < BOX_HEIGHT-2-1) {
		y = (BOX_HEIGHT - 2 -1 - y) / 2;
		werase(toast.wr);
		mvwaddstr(toast.wr,y,x,msg);
	}

	NWIDGET_MUTEX_UNLOCK(mutex);
	return 0;
}

