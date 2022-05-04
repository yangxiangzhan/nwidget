/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       下拉框对象及其管理
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
#include "wg_dropdown.h"
#include "wg_table.h"

/* Private macro ------------------------------------------------------------*/
/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
static int dropdown_focus(struct nwidget *wg_entry);
/* Global  variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Gorgeous Split-line ------------------------------------------------------*/


/**
  * @brief    菜单选项单击事件
  * @param    table : 菜单控件对应的 list 表格控件
  * @param    data : 
  * @note     此函数为 menu->list 的单击事件回调函数，在 menu_popup() 中注册
  * @return   成功返回 0
*/
static int menu_item_clicked(void *_table,void *data)
{
	int selected,changed;
	struct table *table = _table;
	struct dropdown *dropdown;
	struct menuitem *item;
	dropdown = container_of(table->wg.parent,struct dropdown,wg);
	selected = wg_table_current_line(table);
	for (item = dropdown->menu; selected-- ;item = item->next);
	
	changed = dropdown->selected!= item->str;
	dropdown->selected = item->str;
	dropdown_focus(&dropdown->wg);
	desktop_focus_on(&dropdown->wg);
	
	if (changed && dropdown->sig.changed)
		dropdown->sig.changed(dropdown,dropdown->sig.changed_arg);

	if (!dropdown->is_dropdown) 
		dropdown->selected = NULL;
	return 0;
}


static int popup_menu(struct dropdown *dropdown)
{
	int relx,rely;
	int width,height;
	struct table *table;
	struct menuitem *item = dropdown->menu;

	/* 计算弹出菜单的宽度 */
	width = dropdown->menu_width + 2 + 3 + 3;
	height = dropdown->items + 2;

	if (dropdown->is_dropdown) {
		/* 下拉框的菜单应在标签右侧 */
		relx = dropdown->wg.relx + dropdown->labelw;
	} else {
		relx = dropdown->wg.relx + 1;
	}

	if (relx + width >= COLS) {
		/* 右侧没有足够的控件展开弹窗，往左偏移 */
		relx = COLS - width;
		if (relx < 0) {
			beep();
			return -1;
		}
	}

	rely = dropdown->wg.rely + 1;
	if (rely + height >= LINES) {
		/* 下方没有足够的空间展开弹窗 */
		if (height < dropdown->wg.rely) {
			/* 如果在上方有足够的空间则在上方弹窗 */
			rely = dropdown->wg.rely - height;
		} else if (dropdown->wg.rely > LINES/2){
			/* 上方也不够空间展开，如果控件在下半屏，则弹窗在上半屏 */
			rely = 0;
			height = dropdown->wg.rely - 1;
		} else {
			/* 上方不够空间展开，如果控件在上半屏，则弹窗在下半屏 */
			height = LINES - rely;
		}
	}

	table = wg_table_create(height,width,TABLE_BORDER);

	wg_table_column_add(table,"padding",3);
	wg_table_column_add(table,"menu",dropdown->menu_width + 3);
	
	int selected = -1,idx = 0;
	char *values[2] = {"" , NULL};
	for ( ; item; item = item->next) {
		values[1] = item->str;
		wg_table_item_add(table,values);
		if (item->str == dropdown->selected)
			selected = idx;
		idx++;
	}

	if (selected != -1)
		wg_table_jump_to(table,selected);

	/* call menu_item_clicked when item was clicked */
	wg_signal_connect(table,clicked,menu_item_clicked,NULL);
	wg_signal_connect(table,selected,menu_item_clicked,NULL);

	if (0 != wg_table_put(table,NULL,rely,relx)) {
		desktop_tips("wg_table_put()failed!");
		table->wg.destroy(&table->wg);
		return -1;
	}

	/* 更新部分返回按键响应 */
	static const struct wghandler handlers[] = {
		{KEY_LEFT,widget_exit_left},
		{KEY_RIGHT,widget_exit_left},
		{KEY_BACKSPACE,widget_exit_left},
		{0,0}
	};
	handlers_update(&table->wg,handlers);

	desktop_tips("<Up/Down>select | <Space/Enter>comfirm");
	return 0;
}


static int drowdown_display(struct dropdown *dropdown,int label_attr,int val_attr,const char *fmt)
{
	const char *value;
	
	//mvwhline(dropdown->wg.win,0,0,' ',dropdown->wg.width);
	werase(dropdown->wg.win);
	wmove(dropdown->wg.win,0,0);

	if (dropdown->labelw) {
		if (label_attr) {
			wattron(dropdown->wg.win,label_attr);
			waddstr(dropdown->wg.win,dropdown->label);
			wattroff(dropdown->wg.win,label_attr);
		} else {
			waddstr(dropdown->wg.win,dropdown->label);
		}
	}

	value = dropdown->selected ? dropdown->selected : "---";
	
	if (val_attr)
		wattron(dropdown->wg.win,val_attr);
	
	/* whline 不移动光标 */
	whline(dropdown->wg.win,' ',dropdown->wg.width);
	wprintw(dropdown->wg.win,fmt,value);
	
	if (val_attr)
		wattroff(dropdown->wg.win,val_attr);

	desktop_refresh();
	return 0;
}

static int menu_label_display(wg_menu_t *menu,int attr,const char *fmt)
{
	werase(menu->wg.win);
	wmove(menu->wg.win,0,0);
	if (attr) {
		wattron(menu->wg.win,attr);
		waddch(menu->wg.win,fmt[0]);
		waddstr(menu->wg.win,menu->label);
		waddch(menu->wg.win,fmt[1]);
		wattroff(menu->wg.win,attr);
	} else {
		waddch(menu->wg.win,fmt[0]);
		waddstr(menu->wg.win,menu->label);
		waddch(menu->wg.win,fmt[1]);
	}
	return 0;
}

/**
  * @brief    按钮被按下
  * @param    btn : 目标按钮
  * @return   0
*/
static wg_state_t dropdown_clicked(struct dropdown *dropdown)
{
	popup_menu(dropdown);
	return WG_OK;
}

/**
  * @brief    标签被关闭时的回调函数
  * @param    wg_entry : 目标标签所在的 wg 控件句柄
  * @return   0
*/
static int dropdown_preclose(struct nwidget *wg_entry)
{
	struct dropdown *dropdown = container_of(wg_entry,struct dropdown,wg);
	if (dropdown->sig.closed)
		dropdown->sig.closed(dropdown,dropdown->sig.closed_arg);
	return 0;
}


/**
  * @brief    标签被聚焦时的回调函数
  * @param    wg_entry : 目标标签所在的 wg 控件句柄
  * @return   0
*/
static int dropdown_focus(struct nwidget *wg_entry)
{
	int rc;
	struct dropdown *item = container_of(wg_entry,struct dropdown,wg);
	if (item->is_dropdown) {
		int color;
		if (item->wg.bkg == wgtheme.desktop_bkg) 
			color = wgtheme.desktop_fg;
		else
			color = wgtheme.form_fg;
		rc = drowdown_display(item,color,0,"%s↓");
	} else {
		rc = menu_label_display(item,A_REVERSE,"  ");
	}

	return rc;
}

/**
  * @brief    标签被聚焦时的回调函数
  * @param    wg_entry : 目标标签所在的 wg 控件句柄
  * @return   0
*/
static int dropdown_blur(struct nwidget *wg_entry)
{
	struct dropdown *item = container_of(wg_entry,struct dropdown,wg);
	int rc;
	
	if (item->is_dropdown)
		rc = drowdown_display(item,0,A_UNDERLINE,"%s");
	else
		rc = menu_label_display(item,0,"<>");
	return rc;
}



/**
  * @brief    按钮销毁函数
  * @param    wg_entry : 目标按钮所在的 wg 控件句柄
  * @return   WG_OK
*/
static int dropdown_destroy(struct nwidget *wg_entry)
{
	struct dropdown *dropdown = container_of(wg_entry,struct dropdown,wg);
	struct menuitem *item,*next;

	item = dropdown->menu;
	while(item) {
		next = item->next;
		free(item);
		item = next;
	}

	dropdown->menu = NULL;
	if (dropdown->created_by == wg_dropdown_create){
		free(dropdown);
	}
	return 0;
}


/**
  * @brief    按钮鼠标被按下时执行的回调函数，一般仅进行反显
  * @param    wg_entry : 目标按钮所在的 wg 控件句柄
  * @return   WG_OK
*/
static wg_state_t dropdown_mousedown(struct nwidget *wg_entry)
{
	return dropdown_clicked(container_of(wg_entry,struct dropdown,wg));
}

/**
  * @brief    按钮鼠标被按下时执行的回调函数，一般仅进行反显
  * @param    wg_entry : 目标按钮所在的 wg 控件句柄
  * @return   WG_OK
*/
static wg_state_t dropdown_clicked_handler(struct nwidget *wg_entry,long key)
{
	return dropdown_clicked(container_of(wg_entry,struct dropdown,wg));
}


/**
  * @brief    放置函数，将控件显示于窗体或控件上
  * @param    btn : 目标控件
  * @param    parent : 母窗口/母控件
  * @param    y : 母窗口的相对位置
  * @param    x : 母窗口的相对位置
  * @return   成功返回 0
*/
int wg_dropdown_put(struct dropdown *dropdown,struct nwidget *parent,int y,int x)
{
	static const struct wghandler handlers[] = {
		{KEY_ENTER,dropdown_clicked_handler},
		{' ',dropdown_clicked_handler},
		{'\n',dropdown_clicked_handler},
		{0,0}
	};

	static const char tips[] = "dropdown menu";

	/* note:widget_init 会清空 wg */
	if (widget_init(&dropdown->wg,parent,1,dropdown->width,y,x)) {
		return -1;
	}

	handlers_update(&dropdown->wg,handlers);
	dropdown->wg.tips = tips;
	dropdown->wg.focus = dropdown_focus;
	dropdown->wg.blur = dropdown_blur;
	dropdown->wg.preclose = dropdown_preclose;
	dropdown->wg.destroy = dropdown_destroy;
	dropdown->wg.handle_mouse_event = dropdown_mousedown;

	wbkgd(dropdown->wg.win,dropdown->wg.bkg);

	/* display label */
	return dropdown_blur(&dropdown->wg);
	//return drowdown_display(dropdown,0,A_UNDERLINE,"%s");
}


/**
  * @brief    下拉框初始化
  * @param    dropdown : 下拉框控件句柄
  * @param    label    : 下拉框标签
  * @param    parent   : 父窗体，为 NULL 则指定 desktop
  * @param    width    : 窗体宽度
  * @param    y : 相对于父窗体的位置
  * @param    x : 相对于父窗体的位置
  * @return   成功返回 0
*/
static int dropdown_init(struct dropdown *dropdown,const char *label,int width) 
{
	int displayw = 0;

	memset(dropdown,0,sizeof(struct dropdown));

	if (label && label[0]) {
		displayw = wstrlen(label);
		if (displayw > sizeof(dropdown->label)-1)
			displayw = 127;
		
		if (displayw > width){
			if (width < 0) 
				width = displayw + 8;
			else 
				displayw = width;
		}
		wstrncpy(dropdown->label,label,displayw);
		dropdown->labelw = displayw;
	}

	dropdown->is_dropdown = true;
	if (width > 0)
		dropdown->width = width;
	return dropdown->width ? 0 : -1;
}


/**
  * @brief    创建下拉框
  * @param    label : 下拉框标签
  * @param    width : 窗体宽度
  * @return   成功返回 0
*/
struct dropdown *wg_dropdown_create(const char *label,int width)
{
	struct dropdown *dropdown;
	dropdown = malloc(sizeof(struct dropdown));
	if (!dropdown) {
		return NULL;
	}

	if (dropdown_init(dropdown,label,width)) {
		free(dropdown);
		return NULL;
	}

	dropdown->created_by = wg_dropdown_create;
	DEBUG_MSG("%s(%p)",__FUNCTION__,dropdown);
	return dropdown;
}


/**
  * @brief    为下拉框添加条目
  * @param    dropdown : 目标下拉框
  * @param    value : 新条目内容
  * @return   成功返回 0
*/
int wg_dropdown_item_add(struct dropdown *dropdown,const char *value)
{
	struct menuitem **node,*item;

	assert(dropdown && value);

	if (!value[0] || strlen(value) >= sizeof(item->str)-1) {
		return -1;
	}

	for (node = &dropdown->menu; *node; node = &(*node)->next) {
		if (0 == strcmp((*node)->str,value)) {
			return -1;
		}
	}

	item = malloc(sizeof(struct menuitem));
	if (NULL == item) {
		return -1;
	}

	/* 接入链表末端 */
	memset(item,0,sizeof(struct menuitem));
	item->width = wstrlen(value);
	if (item->width >= COLS) {
		free(item);
		return -1;
	}

	strcpy(item->str,value);
	if (item->width > dropdown->menu_width)
		dropdown->menu_width = item->width;
	dropdown->items++;
	*node = item;
	return 0;
}

static int menu_label_init(wg_menu_t *menu,const char *label)
{
	memset(menu,0,sizeof(struct dropdown));
	wstrncpy(menu->label,label,sizeof(menu->label));
	menu->labelw = wstrlen(menu->label);
	menu->width = menu->labelw + 2;
	menu->is_dropdown = false;
	return 0;
}

/**
  * @brief    创建菜单入口
  * @param    label : 菜单入口标签
  * @return   成功句柄，否则返回NULL
*/
wg_menu_t *wg_menu_create(const char *label)
{
	wg_menu_t *menu;

	if (!label || !label[0])
		return NULL;

	menu = malloc(sizeof(struct dropdown));
	if (!menu)
		return NULL;

	if (menu_label_init(menu,label)) {
		free(menu);
		return NULL;
	}

	menu->created_by = wg_dropdown_create;
	return menu;
}

