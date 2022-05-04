/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       桌面任务栏
  ******************************************************************************
  *
  * COPYRIGHT(c) 2022 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "wg_button.h"
#include "wg_form.h"
#include "wg_table.h"
#include "wg_list.h"
#include "stringw.h"

/* Private macro ------------------------------------------------------------*/
/* Private types ------------------------------------------------------------*/

struct taskmore {
	struct wg_list node;
	struct form *form;
	int width;
};

struct taskbar {
	struct nwidget wg;
	struct wg_list tray; /**< 保存状态栏无法显示的窗口列表 */
	int xmax;
	int x;
	int newest;
	int num;
};

static struct taskbar taskbar = {0};

/* Private variables --------------------------------------------------------*/
static const char task_icon_tips[] = "show or hide this form";

/* Global  variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
int desktop_taskbar_register(struct form *form);
/* Gorgeous Split-line ------------------------------------------------------*/

/**
  * @brief    状态栏图标聚焦至窗口
  * @param    object : 控件对象
  * @param    key : 按键键值
  * @return   WG_OK 
*/
static wg_state_t taskbar_icon_to_form(struct nwidget *object,long key)
{
	struct button *icon = container_of(object,struct button,wg);
	struct form *form = (struct form *)icon->sig.clicked_arg;
	if (!form->wg.hidden) {
		desktop_focus_on(&form->wg);
	} else {
		desktop_focus_on(&desktop);
	}
	return WG_OK;
}


/**
  * @brief    桌面状态栏快捷键
  * @param    object : 控件对象
  * @param    key : 按键键值
  * @return   WG_OK 
*/
static int desktop_taskbar_fkey(struct nwidget *object,long key)
{
	struct nwidget *wg;
	struct button *icon;
	struct form *form;

	key -= KEY_F(1);
	if (!taskbar.num || key >= taskbar.num) {
		goto cleanup;
	}

	wg = taskbar.wg.sub;
	for (int i = 0; i < key; i++) {
		wg = wg->next;
	}

	if (wg->tips != task_icon_tips){
		/* 菜单按钮 */
		goto cleanup;
	}

	icon = container_of(wg,struct button,wg);
	form = (struct form *)icon->sig.clicked_arg;

	if (form->wg.hidden || taskbar.newest == icon->label[1]) {
		/* 如果当前窗体不可见，或已在最顶层，触发按钮动作 */
		/* if the form is not visible,or the form is on the top,trigger button press event */
		taskbar.newest = icon->label[1];
		desktop_focus_on(wg);
		icon->wg.handle_mouse_event(wg);
	} else {
		desktop_focus_on(&form->wg);
	}

cleanup:
	return WG_OK;
}


/**
  * @brief    桌面状态栏 F10 快捷键
  * @param    object : 控件对象
  * @param    key : 按键键值
  * @return   WG_OK 
*/
static int desktop_taskbar_f10(struct nwidget *object,long key)
{
	struct nwidget *wg;
	if (!wg_list_empty(&taskbar.tray)) {
		for (wg = taskbar.wg.sub; wg->next; wg = wg->next);
		desktop_focus_on(wg);
		wg->handle_mouse_event(wg);
	}
	return WG_OK;
}


/**
  * @brief    状态栏初始化
  * @return   成功返回 0 ，否则返回其他 
*/
int desktop_taskbar_init(void)
{
	static const struct wghandler Fx_handlers[] = {
		{KEY_F(1), desktop_taskbar_fkey},
		{KEY_F(2), desktop_taskbar_fkey},
		{KEY_F(3), desktop_taskbar_fkey},
		{KEY_F(4), desktop_taskbar_fkey},
		{KEY_F(5), desktop_taskbar_fkey},
		{KEY_F(6), desktop_taskbar_fkey},
		{KEY_F(7), desktop_taskbar_fkey},
		{KEY_F(8), desktop_taskbar_fkey},
		{KEY_F(9), desktop_taskbar_fkey},
		{KEY_F(10), desktop_taskbar_f10},
		{0,0},
	};
	int rc;
	rc = widget_init(&taskbar.wg,&desktop,1,COLS,0,0);
	if (0 == rc) {
		if ((taskbar.wg.bkg = wgtheme.form_bkg))
			wbkgd(taskbar.wg.win,taskbar.wg.bkg);
		taskbar.wg.found_by_tab = false;
		taskbar.xmax = COLS - 10;//[F10:...]
		handlers_update(&desktop,Fx_handlers);
		wg_list_init(&taskbar.tray);
	}
	return rc;
}


/**
  * @brief 桌面状态栏图标被点击的事件回调
  * @param self : 状态栏图标，按钮对象
  * @param clicked_arg : wg_signal_connect 的事件绑定参数
  * @note 由 wg_signal_connect 绑定至控件
  * @return 0 
*/
static int taskbar_icon_clicked(void *self,void *clicked_arg)
{
	struct form *form = clicked_arg;
	int hide = !form->wg.hidden;
	wg_form_hide(form,hide);
	desktop_refresh();
	return 0;
}


/**
  * @brief 桌面状态栏 F10 托盘
*/
static int taskbar_tray(void *btn,void *arg)
{
	int height = 0,width = 0;
	int x = ((struct button *)btn)->wg.relx;
	struct wg_list *node = taskbar.tray.next;
	struct taskmore *task;
	struct table *table;
	for ( ; node != &taskbar.tray; node = node->next) {
		task = container_of(node,struct taskmore,node);
		height++;
		if (task->width > width)
			width = task->width;
	}

	if (height + 1 >= LINES)
		height = LINES - 2;

	if (width >= COLS)
		width = COLS - 3;

	if (x + width >= COLS)
		x = COLS - width;

	table = wg_table_create(height+2,width,TABLE_BORDER);
	wg_table_put(table,NULL,1,x);
	wg_table_column_add(table,"task",width-2);

	char *values[1];
	for (node = taskbar.tray.next; node != &taskbar.tray; node = node->next) {
		task = container_of(node,struct taskmore,node);
		values[0] = task->form->title;
		wg_table_item_add(table,values);
	}

	return 0;
}


/**
  * @brief 桌面状态栏 F10  托盘按钮
*/
static int taskbar_tray_create(void)
{
	struct button *icon;
	if (NULL == (icon = wg_button_create("F10:...",-1))){
		wg_msgbox("create button failed!");
	} else if (0 != wg_button_put(icon,&taskbar.wg,0,taskbar.x)) {
		icon->wg.destroy(&icon->wg);
	} else {
		icon->wg.found_by_tab = false;
		wg_signal_connect(icon,clicked,taskbar_tray,NULL);
	}

	return 0;
}


/**
  * @brief 桌面状态栏删除窗体对应的图标
  * @param form : 状态栏图标对应的窗体对象
*/
void desktop_taskbar_delete(struct form *form)
{
	int oldx,offset = 0;
	struct nwidget *wg = taskbar.wg.sub;
	struct button *icon;
	struct wg_list *node;
	struct taskmore *more;

	for ( ; wg; wg = wg->next) {
		icon = container_of(wg,struct button,wg);
		if (icon->sig.clicked_arg == form) {
			break;
		}
	}

	if (NULL == wg) {
		/* 窗体未显示在任务栏上，尝试查找托盘链表 */
		if (NULL == (node = taskbar.tray.next)) {
			/* 未开启 */
		} else for( ; node != &taskbar.tray; node = node->next) {
			more = container_of(node,struct taskmore,node);
			if (more->form == form) {
				wg_list_del(node);
				free(more);
				break;
			}
		}
		return ;
	}

	offset = wg->width;
	wg = wg->next;
	widget_delete(&icon->wg);
	taskbar.num--;
	taskbar.newest = 0;
	taskbar.x -= offset;

	if (!wg_list_empty(&taskbar.tray)) {
		struct nwidget *taskbarF10;
		for (taskbarF10 = wg; taskbarF10->next; taskbarF10 = taskbarF10->next);
		widget_delete(taskbarF10);
	}

	for ( ; wg ; wg = wg->next) {
		if (task_icon_tips != wg->tips) {
			continue;
		}

		oldx = wg->relx - taskbar.wg.relx;

		/* 注意，按钮的窗体是在母窗体的用 delwin 创建的，按钮的内容与母窗体的内容共享一致，
		所以此处先清空按钮窗体，其实就是清空母窗体对应位置的字符，防止母窗体的残影 */
		werase(wg->win);

		/* 同时此处需要移动子窗口 */
		mvderwin(wg->win,wg->rely,oldx-offset);

		/* 移动子面板 */
		int (*wgmove)(struct nwidget *,int y,int x) = wg->move;
		wgmove(wg,0,-offset);

		/* 重新刷新按钮内容 */
		icon = container_of(wg,struct button,wg);
		icon->label[1]--;
		wg->blur(wg);
	}

	while (!wg_list_empty(&taskbar.tray) && taskbar.num < 9) {
		/* 把托盘中的状体恢复至任务栏 */
		node = taskbar.tray.next;
		more = container_of(node,struct taskmore,node);
		if (more->width + taskbar.x >= taskbar.xmax) {
			break;
		}

		wg_list_del(node);
		desktop_taskbar_register(more->form);
		free(more);
	}

	if (!wg_list_empty(&taskbar.tray))
		taskbar_tray_create();
}


/**
  * @brief 桌面状态栏选中窗体对应的图标
  * @param form : 状态栏图标对应的窗体对象
*/
void desktop_taskbar_select(struct form *form)
{
	struct nwidget *wg = taskbar.wg.sub;
	struct button *btn;
	for ( ; wg; wg = wg->next) {
		btn = container_of(wg,struct button,wg);
		if (btn->sig.clicked_arg == form) {
			wattron(btn->wg.win,wgtheme.form_fg);
			mvwaddstr(btn->wg.win,0,1,btn->label);
			wattroff(btn->wg.win,wgtheme.form_fg);
			desktop_refresh();

			/* btn->label = "F{x}:{form name}",take the x */
			taskbar.newest = btn->label[1];
			break;
		}
	}
}


/**
  * @brief 桌面状态栏取消选中窗体对应的图标
  * @param form : 状态栏图标对应的窗体对象
*/
void desktop_taskbar_unselect(struct form *form)
{
	struct nwidget *wg = taskbar.wg.sub;
	struct button *btn;

	for ( ; wg; wg = wg->next) {
		btn = container_of(wg,struct button,wg);
		if (btn->sig.clicked_arg == form) {
			mvwaddstr(btn->wg.win,0,1,btn->label);
			desktop_refresh();
			break;
		}
	}
}


/**
  * @brief 桌面状态栏注册一个窗体对应的图标
  * @param form : 状态栏图标对应的窗体对象
*/
int desktop_taskbar_register(struct form *form)
{
	#define TITLE_MAX (BTN_MAX_WIDTH-5)
	static const struct wghandler taskbar_icon_handlers[] = {
		{'\t'      , taskbar_icon_to_form},
		{'\n'      , taskbar_icon_to_form},
		{KEY_DOWN  , taskbar_icon_to_form},
		{KEY_RIGHT , taskbar_icon_to_form},
		{KEY_BTAB  , taskbar_icon_to_form},
		{KEY_LEFT  , taskbar_icon_to_form},
		{KEY_UP    , widget_exit_left},
		{0,0},
	};
	char buf[BTN_MAX_WIDTH];
	int width;
	struct button *icon;
	if (NULL == taskbar.wg.win) {
		return 0;
	}

	buf[0] = 'F';
	buf[1] = '0' + taskbar.num + 1;
	buf[2] = ':';

	if (!form->titlew) {
		/* 没有标题的窗体 */
		int p = ((size_t)form) & 0x0FFFFFF;
		snprintf(&buf[3],sizeof(buf)-4,"%x",p);
	} else {
		/* 窗体标题过长，进行截取 */
		wstrncpy(&buf[3],form->title,TITLE_MAX);
	}
	
	width = wstrlen(buf) + 2;

	if (taskbar.num < 9 && width + taskbar.x < taskbar.xmax) {
		/* 如果状态栏有足够的空间容纳新的按钮 */
		icon = wg_button_create(buf,-1);
		if (0 == wg_button_put(icon,&taskbar.wg,0,taskbar.x)) {
			taskbar.x += icon->wg.width;
			taskbar.num++;

			/* 状态栏图标绑定点击事件 */
			wg_signal_connect(icon,clicked,taskbar_icon_clicked,form);
			icon->wg.tips = task_icon_tips;

			/* we don't need to focus the icon*/
			icon->wg.found_by_tab = false;

			/* click the taskbar icon and press TAB,we should fucos on the form */
			handlers_update(&icon->wg,taskbar_icon_handlers);
			icon->wg.edit = taskbar_icon_to_form;
		} else {
			icon->wg.destroy(&icon->wg);
		}
	} else {
		struct taskmore *more;
		int create = wg_list_empty(&taskbar.tray);
		more = malloc(sizeof(struct taskmore));
		memset(more,0,sizeof(struct taskmore));
		wg_list_add_tail(&more->node,&taskbar.tray);
		more->form = form;
		more->width = width;
		if (create)
			taskbar_tray_create();
	}
	return 0;
}


/**
  * @brief 窗体最小化(隐藏)按钮被点击时的事件回调
  * @param self : 窗体对象的最小化按钮对象
  * @param clicked_arg : wg_signal_connect 的事件绑定参数
*/
static int minimize_btn_clicked(void *self,void *clicked_arg)
{
	struct button *btn = self;
	struct form *form = container_of(btn->wg.parent,struct form,wg);
	wg_form_hide(form,true);
	desktop_refresh();
	return 0;
}


/**
  * @brief 窗体关闭按钮被点击时的事件回调
  * @param self : 窗体对象的最小化按钮对象
  * @param clicked_arg : wg_signal_connect 的事件绑定参数
*/
static int close_btn_clicked(void *self,void *clicked_arg)
{
	struct button *btn = self;
	struct form *form = container_of(btn->wg.parent,struct form,wg);
	return widget_delete(&form->wg);
}


/**
  * @brief    设置窗体的按钮
  * @param    height : 窗体高度
  * @param    width : 窗体宽度
  * @param    rely : 窗体坐标
  * @param    relx : 窗体坐标
  * @note     创建的窗体失去焦点时会自动删除
  * @return   成功窗体按钮句柄
*/
int wg_form_set(struct form *form,int minimize,int closable)
{
	int x;
	struct button *btn;
	static const char hide_tips[] = "minimize this form";
	static const char close_tips[] = "close this form";
	if (!form || !form->wg.win) {
		return -1;
	}

	if (!minimize && !closable) {
		return 0;
	}

	x = form->wg.width - 1 - 3;

	if (minimize && form->wg.parent == &desktop) {
		btn = wg_button_create("-",-1);
		if (closable)
			wg_button_put(btn,&form->wg,0,x-3);
		else
			wg_button_put(btn,&form->wg,0,x);
		
		wg_signal_connect(btn,clicked,minimize_btn_clicked,NULL);
		btn->wg.tips = hide_tips;
	}

	if (closable) {
		btn = wg_button_create("X",-1);
		wg_button_put(btn,&form->wg,0,x);
		wg_signal_connect(btn,clicked,close_btn_clicked,NULL);
		btn->wg.tips = close_tips;
	}

	return 0;
}
