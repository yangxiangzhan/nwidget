/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       输入控件对象及其管理
  ******************************************************************************
  *
  * COPYRIGHT(c) 2021 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#include "wg_scrollbar.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Private macro ------------------------------------------------------------*/
//#define SCROLLBAR_VLINE ACS_VLINE
//#define SCROLLBAR_HLINE ACS_HLINE//'-'//


/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Global  variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Gorgeous Split-line ------------------------------------------------------*/

/**
 * @brief 刷新滚动条显示 
 * @param bar : 滚动条 
 * @return 0
 */
static int scrollbar_display(struct scrollbar *bar)
{
	chtype line;
	if (!bar->wg.win) {
		return -1;
	}

	desktop_lock();
	if (bar->vertical) {
		line = wgtheme.rs ? wgtheme.rs : ACS_VLINE ; 
		mvwvline(bar->wg.win,0,0,line,bar->display_len);
		if (bar->slider_len){
			wattron(bar->wg.win,A_REVERSE);
			mvwvline(bar->wg.win,bar->slider_pos,0,' ',bar->slider_len);
			wattroff(bar->wg.win,A_REVERSE);
		}
	} else {
		line = wgtheme.ts ? wgtheme.ts : ACS_HLINE ; 
		mvwhline(bar->wg.win,0,0,line,bar->display_len);
		if (bar->slider_len){
			wattron(bar->wg.win,A_REVERSE);
			mvwhline(bar->wg.win,0,bar->slider_pos,' ',bar->slider_len);
			wattroff(bar->wg.win,A_REVERSE);
		}
	}

	desktop_refresh();
	desktop_unlock();
	return 0;
}

/**
 * @brief 滚动条滑块移动位置
 * @param bar : 滚动条
 * @param move : 移动距离
 * @return 0
 */
static int scrollbar_step_move(struct scrollbar *bar,int move)
{
	int scale_remain,scroll_remain,newval;
	int newpos = bar->slider_pos + move;
	int value = bar->value;

	if (newpos < 0 || newpos + bar->slider_len > bar->display_len) {
		return WG_OK;
	}

	/* scrollbar_update 的逆运算 */
	scroll_remain = bar->display_len - bar->slider_len - 2;
	if (scroll_remain) {
		scale_remain = bar->scale - bar->display_len - 2;
		newval = (newpos - 1) * scale_remain / scroll_remain + 1;
	} else {
		newval = newpos ;
	}
	
	wg_scrollbar_update(bar,newval,bar->scale);
	if (bar->sig.changed && value != bar->value)
		bar->sig.changed(bar,bar->sig.changed_arg);
	return WG_OK;
}

/**
 * @brief 滚动条被聚焦时的动作函数
 * @param self : 滚动条的 wg 入口
 * @return 0 
 */
static int scrollbar_focus(struct nwidget *self)
{
	struct scrollbar *bar = container_of(self, struct scrollbar,wg);
	wattron(bar->wg.win,A_DIM);
	scrollbar_display(bar);
	return 0;
}


/**
 * @brief 滚动条失焦时的动作函数
 * @param self : 滚动条的 wg 入口
 * @return 0 
 */
static int scrollbar_blur(struct nwidget *self)
{
	struct scrollbar *bar = container_of(self, struct scrollbar,wg);
	wattroff(bar->wg.win,A_DIM);
	scrollbar_display(bar);
	return 0;
}

/**
 * @brief 滚动条销毁动作函数
 * @param self : 滚动条的 wg 入口
 * @return 0 
 */
static int scrollbar_destroy(struct nwidget *self)
{
	struct scrollbar *bar = container_of(self, struct scrollbar,wg);
	if (bar->created_by == wg_scrollbar_create){
		free(bar);
		DEBUG_MSG("%s(%p)",__FUNCTION__,bar);
	}
	return 0;
}


/**
 * 
 * @brief 滚动条滑块移动
 * @param self : 滚动条的 wg 入口
 * @return 0 
 */
static int scrollbar_step_less(struct nwidget *wg,long shortcut)
{
	struct scrollbar *bar = container_of(wg,struct scrollbar,wg);
	scrollbar_step_move(bar,-1);
	return WG_OK;
}

/**
 * @brief 滚动条滑块移动
 * @param self : 滚动条的 wg 入口
 * @return 0 
 */
static int scrollbar_step_more(struct nwidget *wg,long shortcut)
{
	struct scrollbar *bar = container_of(wg,struct scrollbar,wg);
	scrollbar_step_move(bar,1);
	return WG_OK;
}


static wg_state_t scrollbar_line_up(struct nwidget *self,long shortcut)
{
	int value;
	struct scrollbar *bar = container_of(self, struct scrollbar,wg);
	value = bar->value;
	wg_scrollbar_update(bar,value - 1,bar->scale);
	if (bar->sig.changed && value != bar->value) {
		bar->sig.changed(bar,bar->sig.changed_arg);
	}
	return WG_OK;
}

static wg_state_t scrollbar_line_down(struct nwidget *self,long shortcut)
{
	int value;
	struct scrollbar *bar = container_of(self, struct scrollbar,wg);
	value = bar->value;
	wg_scrollbar_update(bar,value + 1,bar->scale);
	if (bar->sig.changed && value != bar->value) {
		bar->sig.changed(bar,bar->sig.changed_arg);
	}
	return WG_OK;
}

static wg_state_t scrollbar_page_up(struct nwidget *self,long shortcut)
{
	int value;
	struct scrollbar *bar = container_of(self, struct scrollbar,wg);
	value = bar->value;
	wg_scrollbar_update(bar,value - (bar->display_len - 1),bar->scale);
	if (bar->sig.changed && value != bar->value) {
		bar->sig.changed(bar,bar->sig.changed_arg);
	}
	return WG_OK;
}
static wg_state_t scrollbar_page_down(struct nwidget *self,long shortcut)
{
	int value;
	struct scrollbar *bar = container_of(self, struct scrollbar,wg);
	value = bar->value;
	wg_scrollbar_update(bar,value + (bar->display_len - 1),bar->scale);
	if (bar->sig.changed && value != bar->value) {
		bar->sig.changed(bar,bar->sig.changed_arg);
	}
	return WG_OK;
}


static wg_state_t scrollbar_mousedown(struct nwidget *self)
{
	int newpos,newval,value,scroll_remain,scale_remain;
	struct scrollbar *bar = container_of(self, struct scrollbar,wg);

	value = bar->value;
	if (bar->vertical) {
		newpos = mouse.y - self->rely;
	} else {
		newpos = mouse.x - self->relx;
	}

	if (newpos == bar->slider_pos) {
		return WG_OK;
	}

	if (bar->slow_scroll) {
		if (newpos > bar->slider_pos) {
			return scrollbar_page_down(self,0);
		} else {
			return scrollbar_page_up(self,0);
		}
	}

	/* scrollbar_update 的逆运算 */
	scroll_remain = bar->display_len - bar->slider_len - 2;
	if (scroll_remain) {
		scale_remain = bar->scale - bar->display_len - 2;
		newval = (newpos - 1) * scale_remain / scroll_remain + 1;
	} else {
		newval = newpos ;
	}
	
	wg_scrollbar_update(bar,newval,bar->scale);
	if (bar->sig.changed && value != bar->value) {
		bar->sig.changed(bar,bar->sig.changed_arg);
	}
	return WG_OK;
}

/**
  * @brief    放置滚动条
  * @param    bar : 滚动条
  * @param    parent : 父窗体
  * @param    y : 相对于父窗体的位置
  * @param    x : 相对于父窗体的位置
  * @return   成功返回 0
*/
int wg_scrollbar_put(struct scrollbar *bar,struct nwidget *parent,int y,int x)
{
	static const char tips[]  = "Scrollbar";
	int height,width;
	if (bar->vertical) {
		width = 1;
		height = bar->display_len;
	} else {
		width = bar->display_len;
		height = 1;
	}

	if (widget_init(&bar->wg,parent,height,width,y,x)) {
		return -1;
	}
	
	bar->wg.tips = tips;
	bar->wg.destroy = scrollbar_destroy;
	bar->wg.focus = scrollbar_focus;
	bar->wg.blur = scrollbar_blur;
	bar->wg.handle_mouse_event = scrollbar_mousedown;
	if (bar->vertical) {
		chtype line = wgtheme.rs ? wgtheme.rs : ACS_VLINE;
		static const struct wghandler scrollbar_handlers[] = {
			{KEY_UP,scrollbar_line_up},
			{KEY_DOWN,scrollbar_line_down},
			{KEY_LEFT,scrollbar_page_up},
			{KEY_RIGHT,scrollbar_page_down},
			{KEY_PPAGE,scrollbar_page_up},
			{KEY_NPAGE,scrollbar_page_down},
			{KEY_CTRL_UP,scrollbar_step_less},/* 鼠标拖住上移 */
			{KEY_CTRL_DOWN,scrollbar_step_more},/* 鼠标拖住下移 */
			{0,0}
		};
		handlers_update(&bar->wg,scrollbar_handlers);
		mvwvline(bar->wg.win,0,0,line,bar->display_len);
	} else {
		chtype line = wgtheme.ts ? wgtheme.ts : ACS_HLINE;
		static const struct wghandler scrollbar_handlers[] = {
			{KEY_UP,scrollbar_page_up},
			{KEY_DOWN,scrollbar_page_down},
			{KEY_LEFT,scrollbar_line_up},
			{KEY_RIGHT,scrollbar_line_down},
			{KEY_PPAGE,scrollbar_page_up},
			{KEY_NPAGE,scrollbar_page_down},
			{KEY_CTRL_LEFT,scrollbar_step_less},/* 鼠标拖住左移 */
			{KEY_CTRL_RIGHT,scrollbar_step_more},/* 鼠标拖住右移 */
			{0,0}
		};
		handlers_update(&bar->wg,scrollbar_handlers);
		mvwhline(bar->wg.win,0,0,line,bar->display_len);
	}

	wattron(bar->wg.win,bar->wg.bkg);
	if (bar->scale)
		scrollbar_display(bar);
	return 0;
}


/**
  * @brief    创建一个按钮
  * @param    btn      : 按钮句柄
  * @param    name     : 按钮名字
  * @param    parent   : 父窗体，为 NULL 则指定 stdscr
  * @param    y        : 相对于父窗体的位置
  * @param    x        : 相对于父窗体的位置
  * @return   成功返回 0
*/
static int scrollbar_put_on(void *self,struct nwidget *parent,int y,int x)
{
	return wg_scrollbar_put(self,parent,y,x);
}


/**
  * @brief    设置滚动条
  * @param    bar : 滚动条
  * @param    length : 滚动条总长度
  * @param    vertical : 是否垂直
  * @return   成功返回0
*/
int wg_scrollbar_init(struct scrollbar *bar,int length,int vertical)
{
	assert(bar && length > 2);
	memset(bar,0,sizeof(struct scrollbar));
	bar->vertical = vertical != 0;
	bar->display_len = length;
	bar->put_on = scrollbar_put_on;
	return 0;
}


/**
  * @brief    创建一个滚动条
  * @param    length : 滚动条总长度
  * @param    vertical : 是否垂直
  * @return   滚动条
*/
struct scrollbar *wg_scrollbar_create(int length,int vertical)
{
	struct scrollbar *bar;
	bar = malloc(sizeof(struct scrollbar));
	if (bar == NULL) {
		return NULL;
	}
	if (0 != wg_scrollbar_init(bar,length,vertical)) {
		free(bar);
		return NULL;
	}
	
	/* 标记最底层面板的创建者，销毁控件时自动释放内存 */
	bar->created_by = wg_scrollbar_create;
	DEBUG_MSG("%s(%p)",__FUNCTION__,bar);
	return bar;
}


/**
  * @brief    滚动条值更新
  * @param    bar : 滚动条
  * @param    value : 滚动条当前值
  * @param    scale_max : 滚动条最大值
  * @return   0
*/
int wg_scrollbar_update(struct scrollbar *bar,int value,int scale_max)
{
	int scale_remain,slider_len,slider_pos,display_len;
	display_len = bar->display_len;
	if (scale_max <= display_len || display_len < 3){
		if (bar->scale) {
			bar->value = 0;
			bar->scale = 0;
			bar->slider_len = 0;
			bar->slider_pos = 0;
			scrollbar_display(bar);
		}
		return 0;
	}

	/* 换算滚动条的长度和位置 */
	slider_len = display_len * display_len / scale_max ;
	if (slider_len < 1){
		slider_len = 1;
	}

	if (value < 1){
		value = 0;
		slider_pos = 0;
	} else if (value + display_len >= scale_max){
		slider_pos = display_len - slider_len ;
		value = scale_max - display_len;
	} else {
		int scroll_remain = display_len - slider_len - 2;
		if (scroll_remain < 1) {
			slider_pos = 1 ;
		} else {
			scale_remain = scale_max - display_len - 2 ;
			/* slider_pos = 1 + value / (remain / scroll_remain); //as */
			slider_pos = 1 + value * scroll_remain / scale_remain;
		}
	}

	bar->value = value;
	bar->scale = scale_max;
	if (bar->slider_pos == slider_pos && bar->slider_len == slider_len) {
		return 0;
	}
	bar->slider_pos = slider_pos;
	bar->slider_len = slider_len;
	scrollbar_display(bar);
	return 0;
}
