/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       基本控件对象及其管理
  ******************************************************************************
  *
  * COPYRIGHT(c) 2021 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#include <stdlib.h>
#include <locale.h>
#include <assert.h>
#include <string.h>
#include "wg_mutex.h"
#include "nwidget.h"
#include "stringw.h"

/* Private macro ------------------------------------------------------------*/

#define VERSION "V1.3.10"

#ifdef UNIT_DEBUG
	int count = 0;
#endif



/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

static void *placed_mutex;
static struct nwidget * volatile desktop_focus = &desktop;

/** 默认主题 */
static const struct theme default_theme = {
	.title = WG_RED,
	.desktop_bkg = WG_BLUE,
	.desktop_fg = WG_WHITE,
	.form_bkg = WG_WHITE,
	.form_fg = WG_BLACK,
	.blue = WG_RGB(6,102,177),
	.white = WG_RGB(216,223,241),
	.black = WG_RGB(39,40,34),
	#if 0
	.ls = '|' ,
	.rs = '|' , 
	.ts = '-' ,
	.bs = '-' , 
	.tl = '+' , 
	.tr = '+' ,
	.bl = '+' , 
	.br = '+' , 
	#endif
	.btn_normal = "[]" ,
	.btn_clicked = "><" ,
	.btn_focus = "  " ,
	.chkbox_enabled = "(*)",
	.chkbox_disabled = "( )",
	.radio_enabled = "[*]",
	.radio_disabled = "[ ]",
};

/* Global  variables --------------------------------------------------------*/
struct nwidget desktop = {0};
struct theme wgtheme;
MEVENT mouse;

static const struct wghandler widget_default_handlers[] = {
	{'\t'      , widget_exit_right},
	{'\n'      , widget_exit_right},
	{KEY_DOWN  , widget_exit_right},
	{KEY_RIGHT , widget_exit_right},
	{KEY_BTAB  , widget_exit_left},
	{KEY_LEFT  , widget_exit_left},
	{KEY_UP    , widget_exit_left},
	{0,0},
};


/* Private function prototypes ----------------------------------------------*/
static int desktop_deinit(void);
/* Gorgeous Split-line ------------------------------------------------------*/

// 上锁整个桌面，原则上任何修改屏幕内容前都应对整个屏幕上锁
void desktop_lock(void) 
{
	NWIDGET_MUTEX_LOCK(placed_mutex);
}

void desktop_unlock(void)
{
	NWIDGET_MUTEX_UNLOCK(placed_mutex);
}


/**
  * @brief    update desktop tips
  * @param    tips : message
  * @return   0
*/
int desktop_tips(const char *tips)
{
	static const char *show = NULL;
	const char *update = tips && tips[0] ? tips : NULL;

	//desktop_lock();
	if (show){
		/* 当前有内容 */
		mvhline(LINES-1,0,' ',COLS);
		desktop_refresh();
	}
	if (update){
		mvwaddstr(stdscr,LINES-1,0,update);
		desktop_refresh();
	}

	show = update;
	//desktop_unlock();
	return 0;
}


/**
  * @brief    控件移动函数
  * @param    wg : 控件句柄
  * @param    y : y 轴偏移
  * @param    x : x 轴偏移
  * @return   0
*/
static int widget_move(struct nwidget *wg,int y,int x)
{
	desktop_lock();
	wg->rely += y;
	wg->relx += x;
	if (wg->panel)
		move_panel(wg->panel,wg->rely,wg->relx);
	else if (wg->win)
		mvwin(wg->win,wg->rely,wg->relx);
	desktop_unlock();
	return 0;
}


/**
  * @brief    控件的默认隐藏/显示函数
  * @param    wg : wg 控件句柄
  * @param    hide : 不为0时执行隐藏函数
  * @return   0
*/
static int widget_hide(struct nwidget *wg,int hide)
{
	desktop_lock();
	if (hide) 
		hide_panel(wg->panel);
	else 
		show_panel(wg->panel);
	wg->hidden = hide != 0;
	desktop_unlock();
	return 0;
}


/** 
  * @brief    获取两个控件的最近共同祖先节点
  * @param    wg1 : 目标对象1
  * @param    wg2 : 目标对象2
  * @return   最近共同祖先节点
*/
static struct nwidget *lowest_common_ancestor(struct nwidget *wg1, struct nwidget *wg2)
{
	struct nwidget *node;
	int len1 = 0,len2 = 0;
	
	if (&desktop == wg1 || &desktop == wg2) {
		return &desktop;
	}

	/* 先测量两个节点距离根节点(即桌面)的深度 */
	for (node = wg1; node != &desktop; node = node->parent){
		len1++;
	}

	for (node = wg2; node != &desktop; node = node->parent){
		len2++;
	}
	
	/* 较深节点回溯至较浅节点同一深度，使其两节点深度一致 */
	if (len1 > len2) {
		for ( ;len1 > len2; len1--)
			wg1 = wg1->parent;
	} else {
		for ( ;len2 > len1; len2--)
			wg2 = wg2->parent;
	}

	/* 对深度一致的节点同时回溯即可找到最近的共同祖先 */
	for (int i = 0; i < len1; i++) {
		if (wg1 == wg2)
			return wg1;
		wg1 = wg1->parent;
		wg2 = wg2->parent;
	}
	return &desktop;
}


/**
  * @brief    由底向上递归触发聚焦事件
  * @param    target : 目标控件，最顶控件
  * @param    ancestor : 所需更新状态控件的祖父节点，最底控件
  * @return   0
*/
static int widget_trigger_focus(struct nwidget *target,struct nwidget *ancestor)
{
	if (target->parent != ancestor)
		widget_trigger_focus(target->parent,ancestor);

	target->focus_time = ++desktop.focus_time;

	if (target->panel){
		top_panel(target->panel);
		desktop_refresh();
	}

	target->editing = 1;
	if (target->focus)
		target->focus(target);
	return 0;
}


/**
  * @brief    移动焦点至指定控件，控件至于顶层并执行聚焦回调
  * @param    target : 目标对象
  * @return   0
*/
int desktop_focus_on(struct nwidget *target)
{
	struct nwidget *ancestor,*wg = desktop_focus;
	ancestor = lowest_common_ancestor(target,desktop_focus);
	
	/* 失焦控件执行失焦事件 */
	while(wg != ancestor) {
		struct nwidget *parent = wg->parent;
		wg->editing = 0;
		if (wg->blur)
			wg->blur(wg);
		if (wg->delete_onblur)
			widget_delete(wg);
		wg = parent;
	}

	desktop_focus = target;
	desktop_tips(target->tips);
	return (target != ancestor) ? widget_trigger_focus(target,ancestor) : 0;
}


/**
  * @brief    判断鼠标是否在控件范围内
  * @param    widget   : 控件对象
  * @return   鼠标在控件内返回 1 ，否则返回 0
*/
static inline int mouse_on_widget(struct nwidget *widget)
{
	return  (mouse.x >= widget->relx && 
		mouse.x < widget->relx + widget->width &&
		mouse.y >= widget->rely && 
		mouse.y < widget->rely + widget->height);
}


/**
  * @brief    在 wg 的子控件内查找鼠标位置所在的顶层子控件
  * @param    wg : 最底层控件
  * @return   返回最后被鼠标点击激活的对象
*/
static struct nwidget *nwidget_mouse(struct nwidget *wg)
{
	struct nwidget *widget,*top = wg;

	if (!mouse_on_widget(wg)) {
		return NULL;
	}

	/* 遍历所有子节点，返回最后被激活的对象 */
	while(wg->sub) {
		top = NULL;
		for (widget = wg->sub; widget; widget = widget->next) {
			if (widget->hidden || !mouse_on_widget(widget)) {
				continue;
			}
			if (!top || widget->focus_time >= top->focus_time) {
				top = widget;
			}
		}

		if (!top) {
			break;
		} else {
			wg = top;
		}
	}
	return wg;
}

/**
  * @brief    获取 wg 下第一个可被 TAB 键聚焦的子控件
  * @param    wg : 起始根
  * @note     仅判断子控件，不包括 wg 本身
  * @return   返回第一个可被 TAB 键聚焦的子控件
*/
static struct nwidget *tab_first(struct nwidget *wg)
{
	struct nwidget *object,*res;
	for (object = wg->sub; object; object = object->next) {
		if (object->hidden) {
			continue;
		}

		if (object->found_by_tab) {
			return object;
		}
		
		res = tab_first(object);
		if (res != NULL) {
			return res;
		}
	}
	return NULL;
}


/**
  * @brief    获取下一个可被 TAB 键聚焦的控件
  * @param    wg : 起始点
  * @return   返回下一个可被 TAB 键聚焦的控件
*/
static struct nwidget *nwidget_next(struct nwidget *wg)
{
	struct nwidget *next,*parent;

	/* 存在子控件可被聚焦，返回第一个子控件 */
	next = tab_first(wg);
	if (next) {
		return next;
	}

	/* 如果 wg 是桌面或模态窗口，则返回自身 */
	while (wg != &desktop && !wg->always_top) {
		parent = wg->parent;
		
		/* 否则查找同级的兄弟节点 */
		for (wg = wg->next; wg; wg = wg->next) {
			if (wg->hidden) {
				continue;
			}
			
			if (wg->found_by_tab) {
				return wg;
			}

			if ( (next = tab_first(wg)) ) {
				return next;
			}
		}
		wg = parent;
	}

	return wg;
}


// 获取 wg 节点下最后一个可以被 tab 键 
static struct nwidget *tab_last(struct nwidget *wg)
{
	struct nwidget *res,*object = wg->sub;
	if (wg->hidden) {
		return NULL;
	}
	if (object) {
		/* 获取末节点 */
		while (object->next)
			object = object->next;
		
		do {
			if (object->hidden)
				continue;

			/* 从末位往前遍历，递归查找 */
			res = tab_last(object);
			if (NULL != res)
				return res;
		} while ((object = object->prev));
	}
	return wg->found_by_tab ? wg : NULL;
}


/**
  * @brief    获取上一个可被 TAB 键聚焦的控件
  * @param    wg : 起始点
  * @return   返回上一个可被 TAB 键聚焦的控件
*/
static struct nwidget *nwidget_prev(struct nwidget *wg)
{
	struct nwidget *found;
	assert(wg);
	
	while(wg != &desktop && !wg->always_top) {
		while (wg->prev) {
			wg = wg->prev;
			found = tab_last(wg);
			if (found)
				return found;
		}
		wg = wg->parent;
		if (wg->found_by_tab) {
			return wg;
		}
	}
	return tab_last(wg);
}


/**
  * @brief    上一个控件退出编辑后，根据退出模式聚焦至下一个需编辑的控件
  * @param    old_focus : 上一个退出编辑的控件
  * @param    blur_stat : 退出编辑的模式
  * @return   0
*/
static void widget_exit(struct nwidget *old_focus,int blur_stat)
{
	struct nwidget *focus;
	if (blur_stat == WG_EXIT_MOUSE) {
		focus = nwidget_mouse(&desktop);
	} else if (blur_stat < 0) {
		focus = nwidget_prev(old_focus);
	} else {
		focus = nwidget_next(old_focus);
	}

	if (focus == old_focus || !focus) {
		old_focus->editing = true;
		beep();
	} else {
		desktop_focus_on(focus);
		if (blur_stat == WG_EXIT_MOUSE && focus->handle_mouse_event) {
			focus->handle_mouse_event(focus);
		}
	}
}

/**
  * @brief    控件处理按键输入
  * @param    widget : 当前控件
  * @param    key    : 按键键值
  * @return   控件编辑结果， @see wg_state_t
*/
static wg_state_t widget_edit(struct nwidget *widget,int key)
{
	struct nwidget *interested;
	if (KEY_MOUSE == key) {
		interested = nwidget_mouse(widget);
		if (NULL == interested)
			return WG_EXIT_MOUSE;
		if (widget != interested)
			desktop_focus_on(interested);
		if (interested->handle_mouse_event) 
			return interested->handle_mouse_event(interested);
		return WG_OK;
	}

	for (int i = 0; i < MAX_HANDLERS && widget->shortcuts[i]; i++) {
		if (key == widget->shortcuts[i])
			return widget->handlers[i](widget,key);
	}

	if (widget->edit) 
		return widget->edit(widget,key);
	return WG_OK;
}


/**
  * @brief    更新对象的按键响应
  * @param    object   : 控件对象
  * @param    handlers : 键值-处理函数结构体数组，必须以 {0,0} 结尾
  * @return   0 
*/
int handlers_update(struct nwidget *object,const struct wghandler *handlers)
{
	assert(handlers && object);

	int i ;
	while(handlers->shortcut) {
		for (i = 0; i < MAX_HANDLERS - 1 ; i++) {
			if (!object->shortcuts[i] || 
				handlers->shortcut == object->shortcuts[i]) {
				break;
			}
		}

		if (i < MAX_HANDLERS - 2) {
			object->shortcuts[i] = handlers->shortcut;
			object->handlers[i] = handlers->func;
		}

		handlers++;
	}
	return 0;
}


/**
  * @brief    新控件加入树，连接父节点
  * @param    widget : 控件
  * @param    parent : 连接父节点
  * @note     当 parent 为空时默认为添加到当前聚焦控件
  * @return   成功返回 0
*/
static int widget_connect(struct nwidget *widget,struct nwidget *parent)
{
	struct nwidget *node;
	
	if (!parent)
		parent = desktop_focus;

	/* 查找 parent 下的末节点，将 widget 添加至末节点的后面 */
	node = parent->sub;
	if (node) {
		while(node->next)
			node = node->next;
		node->next = widget;
	} else {
		/* 成为 parent 第一个子节点 */
		parent->sub = widget;
	}

	widget->prev = node;
	widget->next = widget->sub = NULL;
	widget->parent = parent;
	widget->has_been_placed = HAS_BEEN_PLACED;
	widget->bkg = parent->bkg;
	return 0;
}


/**
  * @brief    控件从树删除
  * @param    widget : 控件
  * @return   成功返回 0
*/
static int widget_disconnect(struct nwidget *widget)
{
	struct nwidget *prev,*next,*parent;

	parent = widget->parent;
	prev = widget->prev;
	next = widget->next;

	/* 在同级链表中删除此节点 */
	if (NULL != next)
		next->prev = prev;
	if (NULL != prev)
		prev->next = next;

	/* 此节点为父节点的首个子节点，则首个子节点转移至下一个兄弟节点 */
	if (NULL != parent && parent->sub == widget)
		parent->sub = next;
	
	widget->parent = widget->next = widget->prev = NULL;
	widget->has_been_placed = 0;
	return 0;
}


/**
  * @brief    控件初始化
  * @param    widget : 控件
  * @param    form   : 控件所在的窗体
  * @param    rows   : 控件长度
  * @param    cols   : 控件宽度
  * @param    y      : Y 轴位置
  * @param    x      : X 轴位置
  * @return   成功返回 0
*/
int widget_init(struct nwidget *widget,struct nwidget *parent,int rows,int cols,int y,int x)
{
	assert(widget);
	int rc = -1;
	WINDOW *win;
	PANEL *panel = NULL;

	memset(widget,0,sizeof(struct nwidget));
	
	desktop_lock();
	if (parent == &desktop || !parent)
		win = newwin(rows,cols,y,x);
	else
		win = derwin(parent->win,rows,cols,y,x);

	/*win = newwin(rows,cols,y,x);*/
	if (win == NULL) {
		goto cleanup;
	}

	panel = new_panel(win);
	if (panel == NULL) {
		delwin(win);
		goto cleanup;
	}

	widget_connect(widget,parent);

	widget->win = win;
	widget->panel = panel;
	widget->found_by_tab = true;
	widget->focus_time = ++desktop.focus_time;
	widget->height = rows;
	widget->width = cols;
	widget->rely = y;
	widget->relx = x;
	widget->move = widget_move;
	widget->hide = widget_hide;
	if (parent) {
		widget->rely += parent->rely;
		widget->relx += parent->relx;
	} else {
		static const struct wghandler handlers[] = {
			{'\n', widget_exit_left},
			{0,0},
		};
		handlers_update(widget,handlers);
		widget->delete_onblur = true;
	}

	rc = handlers_update(widget,widget_default_handlers);
cleanup:
	desktop_unlock();
	if (!parent && !rc)
		desktop_focus_on(widget);
	return rc;
}


/**
  * @brief    从桌面删除一个控件及其子控件
  * @param    widget : 控件
  * @return   成功返回 0
*/
static int do_delete(struct nwidget *widget)
{
	struct nwidget *wg,*next;

	/* 递归删除子节点 */
	for (wg = widget->sub; wg; wg = next) {
		next = wg->next;
		do_delete(wg);
	}

	/* 预关闭 */
	if (widget->preclose)
		widget->preclose(widget);

	desktop_lock();
	
	/* 如果当前节点是桌面的聚焦节点，将焦点转移至父节点
	if (widget == desktop_focus){
		desktop_focus = widget->parent;
		desktop_tips(desktop_focus->tips);
	} */

	widget_disconnect(widget);

	if (widget->panel){
		del_panel(widget->panel);
		widget->panel = NULL;
	}

	if (widget->win) {
		werase(widget->win);
		delwin(widget->win);
		widget->win = NULL;
	}
	
	desktop_unlock();

	/* 执行销毁函数 */
	if (widget->destroy)
		widget->destroy(widget);
	return 0;
}


/**
  * @brief    请求桌面删除一个控件及其子控件
  * @param    widget : 控件
  * @return   成功返回 0
*/
int widget_delete(struct nwidget *widget)
{
	int rc = -1;
	if (widget->has_been_placed == HAS_BEEN_PLACED) {
		desktop_lock();
		if (widget->editing){
			desktop_focus = widget->parent;
			desktop_tips(desktop_focus->tips);
		}
		widget_disconnect(widget);
		desktop_unlock();

		rc = do_delete(widget);
		desktop.editing++;
	}
	return rc;
}

static int desktop_func_key(long key)
{
	struct nwidget *wg;

	for (wg = desktop_focus; wg != &desktop; wg = wg->parent) {
		for (int i = 0; i < MAX_HANDLERS && wg->shortcuts[i]; i++) {
			if (key == wg->shortcuts[i]){
				return desktop_focus_on(wg);
			}
		}
	}
	return desktop_focus_on(&desktop);
}


long mouse_handle()
{
	long key = KEY_MOUSE;
	static MEVENT prev = {0};
	if (OK != GET_MOUSE(&mouse)) {
		return 0;
	}

#ifdef __PDCURSES__
	#define NCURSES_MOUSE_VERSION 2
#else
	/* pdcurses has MOUSE_MOVED but ncurses doesn't */
	#define MOUSE_MOVED (mouse.bstate & REPORT_MOUSE_POSITION) && (prev.bstate == mouse.bstate)
#endif

#if (NCURSES_MOUSE_VERSION > 1)
	if (mouse.bstate & BUTTON4_PRESSED) { /* 鼠标滚轮键值 */
		key = KEY_UP;
	} else if (mouse.bstate & BUTTON5_PRESSED) {
		key = KEY_DOWN;
	} else if (MOUSE_MOVED) {
		mouse.bstate |= REPORT_MOUSE_POSITION; /* for pdcurses */
		if (prev.x != mouse.x) {
			key = prev.x > mouse.x ? KEY_CTRL_LEFT : KEY_CTRL_RIGHT;
		}
		
		if (prev.y != mouse.y) {
			if (prev.x != mouse.x)
				widget_edit(desktop_focus,key);
			key = prev.y > mouse.y ? KEY_CTRL_UP : KEY_CTRL_DOWN;
		}
	}
#else
	if (mouse.bstate & BUTTON4_PRESSED) {
		key = KEY_UP;
	} else if (MOUSE_MOVED) {
		int xdff = abs(mouse.x - prev.x);
		int ydff = abs(mouse.y - prev.y);
		if (ydff + xdff < 1) {
			key = KEY_DOWN;
		} else {
			if (prev.x != mouse.x) {
				key = prev.x > mouse.x ? KEY_CTRL_LEFT : KEY_CTRL_RIGHT;
			}
			
			if (prev.y != mouse.y) {
				if (prev.x != mouse.x)
					widget_edit(desktop_focus,key);
				key = prev.y > mouse.y ? KEY_CTRL_UP : KEY_CTRL_DOWN;
			}
		}
	}
#endif
	prev = mouse;
	return key;
}


/**
  * @brief    获取键盘输入
  * @return   0 
*/
int desktop_keyboard(long key)
{
	int blur = 0;
	
	if (0x03 == key) /* 'q' == key || ctrl+C */
		return -1;
	
	if (KEY_RESIZE == key) /* 终端大小改变 */
		return 0;
	
	if (key > KEY_F0 && key <= KEY_F(12)) {
		/* 功能键特殊处理，将遍历父节点并定位到响应功能键的控件 */
		desktop_func_key(key);
	} else if (KEY_MOUSE == key) {
		key = mouse_handle();
	}
	#ifdef __NCURSES_H 
		/* pdcurses will get a unicode but ncurses get utf8 encoding */
		else if ((key >= 0xc0 && key < 0xff)) {
			int len = utf8len(key);
			key &= (1 << (8 - len)) - 1;
			for (int i = 1 ; i < len ; i++){
				key <<= 6;
				key |= getch() & 0x3f;
			}
		}
	#endif

	DEBUG_LOG(LINES-2,1,"(count:key)%03d:%03ld",count++,key);

	blur = widget_edit(desktop_focus,key);
	if (blur || !desktop_focus->editing)
		widget_exit(desktop_focus,blur);
	if (mouse.bstate)
		mouse.bstate = 0;
	return 0;
}


/**
  * @brief    对桌面的控件进行重绘
  * @param    widget : 控件
  * @return   成功返回 0
*/
static int do_redraw(struct nwidget *widget)
{
	struct nwidget *wg;
	if (widget->redraw_requested && widget->redraw) 
		widget->redraw(widget);
	widget->redraw_requested = 0;
	for (wg = widget->sub; wg; wg = wg->next){
		do_redraw(wg);
	}
	return 0;
}


/**
  * @brief    接收输入，交由 curses 托管
  * @return   0 
*/
int desktop_editing(void)
{
	long key = 0;

	update_panels();
	doupdate();

	timeout(REFRESH_DELAY_MS);
	while(desktop.editing) {
		key = getch();
		if (key > 0 && desktop_keyboard(key) < 0)
			break;

		if (desktop.editing > 1) {
			/* 需要执行刷新操作 */
			desktop.editing = 1;
			desktop_lock();
			update_panels();
			doupdate();
			desktop_unlock();
		}

		if (desktop.redraw_requested) {
			do_redraw(&desktop);
			desktop_lock();
			update_panels();
			doupdate();
			desktop_unlock();
		}
	}

	do_delete(&desktop);
	desktop_deinit();
	return 0;
}


/**
  * @brief    初始化 curses 颜色
  * @param    curses : 颜色代码
  * @param    rgb : rgb 24 bits
  * @note     init_color() 的 RGB 单个色的范围为 0-1000
  * @return   0 
*/
static int wg_init_color(int color_code,int rgb)
{
	int red,green,blue,tmp;

	tmp = ((rgb >> 16) & 0x00FF) * 4;
	red = tmp > 1000 ? 1000 : tmp;

	tmp = ((rgb >> 8) & 0x00FF) * 4;
	green = tmp > 1000 ? 1000 : tmp;

	tmp = ((rgb >> 0) & 0x00FF) * 4;
	blue = tmp > 1000 ? 1000 : tmp;

	return init_color(color_code,red,green,blue);
}

/**
  * @brief    初始化 curses 相关组件
  * @return   0 
*/
int desktop_init(struct theme *user)
{
	const char *css;

	mmask_t old;
	mmask_t mask = ALL_MOUSE_EVENTS|REPORT_MOUSE_POSITION;

	//memcpy(&wgtheme,&no_color,sizeof(no_color));
	if (!user)
		user = (struct theme*)&default_theme;

	/* 将 enum wg_color 的值转为 curses.h 中的颜色值 */
	wgtheme.title = user->title ? user->title-1 : COLOR_RED;
	wgtheme.desktop_bkg = user->desktop_bkg ? user->desktop_bkg-1 : COLOR_BLUE;
	wgtheme.desktop_fg = user->desktop_fg ? user->desktop_fg-1 : COLOR_WHITE;
	wgtheme.form_bkg = user->form_bkg ? user->form_bkg-1 : COLOR_WHITE;
	wgtheme.form_fg = user->form_fg ? user->form_fg-1 : COLOR_BLACK;
	wgtheme.shadow = user->shadow ? user->shadow-1 : COLOR_BLACK;

	/* 重定义调色板 */
	wgtheme.black = user->black;
	wgtheme.red = user->red;
	wgtheme.green = user->green;
	wgtheme.yellow = user->yellow;
	wgtheme.blue = user->blue;
	wgtheme.magenta = user->magenta;
	wgtheme.cyan = user->cyan;
	wgtheme.white = user->white;

	/* 按钮样式表 */
	css = user->btn_normal[0] ? user->btn_normal : default_theme.btn_normal;
	wgtheme.btn_normal[0] = css[0];
	wgtheme.btn_normal[1] = css[1];
	css = user->btn_focus[0] ? user->btn_focus : default_theme.btn_focus;
	wgtheme.btn_focus[0] = css[0];
	wgtheme.btn_focus[1] = css[1];
	css = user->btn_clicked[0] ? user->btn_clicked : default_theme.btn_clicked;
	wgtheme.btn_clicked[0] = css[0];
	wgtheme.btn_clicked[1] = css[1];

	/* 复选框/单选框样式 */
	css = user->chkbox_enabled[0] ? user->chkbox_enabled : default_theme.chkbox_enabled;
	memcpy(wgtheme.chkbox_enabled,css,3);
	css = user->chkbox_disabled[0] ? user->chkbox_disabled : default_theme.chkbox_disabled;
	memcpy(wgtheme.chkbox_disabled,css,3);
	css = user->radio_enabled[0] ? user->radio_enabled : default_theme.radio_enabled;
	memcpy(wgtheme.radio_enabled,css,3);
	css = user->radio_disabled[0] ? user->radio_disabled : default_theme.radio_disabled;
	memcpy(wgtheme.radio_disabled,css,3);

	/* initialize Ncurses */
	setlocale(LC_ALL, "");
	initscr();
	curs_set(0);
	cbreak();
	noecho(); // disable echoing of characters on the screen
	keypad(stdscr, TRUE ); // enable keyboard input for the window.
	mousemask(mask,&old);
	
	#ifdef __PDCURSES__
	mouse_set(mask);
	request_mouse_pos();
	#endif

	/* https://mudhalla.net/tintin/info/xterm/
	Makes the terminal report mouse movement events 1
	1003: always get a position event
	1002: when the cursor moves to a different cell while a button is being held down
	*/
	printf("\033[?1002h\n");
	
	if (!has_mouse()) {
		mvaddstr(2,0,"without mouse");
	}

	/* 边框样式表，ACS_VLINE 等变量需要初始化 ncurses 后生效 */
	wgtheme.ls = user->ls ? user->ls : ACS_VLINE;
	wgtheme.rs = user->rs ? user->rs : ACS_VLINE;
	wgtheme.ts = user->ts ? user->ts : ACS_HLINE;
	wgtheme.bs = user->bs ? user->bs : ACS_HLINE;
	wgtheme.tl = user->tl ? user->tl : ACS_ULCORNER;
	wgtheme.tr = user->tr ? user->tr : ACS_URCORNER;
	wgtheme.bl = user->bl ? user->bl : ACS_LLCORNER;
	wgtheme.br = user->br ? user->br : ACS_LRCORNER;

	if (has_colors() == TRUE) {
		/** 颜色对用法定义 */
		enum wg_color_pair {
			WG_DESKTOP_BKG = 1,/**< 桌面的基础底色调颜色对 */
			WG_DESKTOP_TITLE, /**< 窗口或控件被聚焦时的标题颜色对 */
			WG_FORM_BKG,/**< 窗口的基础色调颜色对 */
			WG_FORM_TITLE,/**< 窗口或控件被聚焦时的标题颜色对 */
			WG_SHADOW,
		};

		start_color();
		
		init_pair(WG_DESKTOP_BKG,wgtheme.desktop_fg,wgtheme.desktop_bkg);
		init_pair(WG_DESKTOP_TITLE,wgtheme.title,wgtheme.desktop_bkg);

		init_pair(WG_FORM_BKG,wgtheme.form_fg,wgtheme.form_bkg);
		init_pair(WG_FORM_TITLE,wgtheme.title,wgtheme.form_bkg);

		init_pair(WG_SHADOW,wgtheme.desktop_fg,wgtheme.shadow);
		//init_pair(6,COLOR_BLUE,COLOR_RED);

		wgtheme.desktop_bkg = COLOR_PAIR(WG_DESKTOP_BKG);
		wgtheme.desktop_fg = COLOR_PAIR(WG_DESKTOP_TITLE);
		wgtheme.form_bkg = COLOR_PAIR(WG_FORM_BKG);
		wgtheme.form_fg = COLOR_PAIR(WG_FORM_TITLE)|A_BOLD;
		wgtheme.shadow = COLOR_PAIR(WG_SHADOW);

		if (can_change_color()) {
			if (wgtheme.black)
				wg_init_color(COLOR_BLACK,wgtheme.black);
			if (wgtheme.red)
				wg_init_color(COLOR_RED,wgtheme.red);
			if (wgtheme.green)
				wg_init_color(COLOR_GREEN,wgtheme.green);
			if (wgtheme.yellow)
				wg_init_color(COLOR_YELLOW,wgtheme.yellow);
			if (wgtheme.blue)
				wg_init_color(COLOR_BLUE,wgtheme.blue);
			if (wgtheme.magenta)
				wg_init_color(COLOR_MAGENTA,wgtheme.magenta);
			if (wgtheme.cyan)
				wg_init_color(COLOR_CYAN,wgtheme.cyan);
			if (wgtheme.white)
				wg_init_color(COLOR_WHITE,wgtheme.white);
		} else {
			//mvaddstr(3,0,"cannot change color");
		}

		//wattron(stdscr,wgtheme.desktop_bkg);
		wbkgd(stdscr,wgtheme.desktop_bkg);
	} else {
		wgtheme.desktop_bkg = wgtheme.form_bkg = 0;
		wgtheme.desktop_fg = wgtheme.form_fg = A_BOLD;
		wgtheme.shadow = A_REVERSE;
	}
	
	NWIDGET_MUTEX_INIT(placed_mutex);

	desktop.bkg = wgtheme.desktop_bkg;
	desktop.parent = &desktop; /* 桌面的父节点为自己，在 do_delete 的时候防止错误 */
	desktop.win = stdscr;
	desktop.relx = 0;
	desktop.rely = 0;
	desktop.width = COLS;
	desktop.height = LINES;
	desktop.editing = 1;
	desktop.found_by_tab = 1;
	desktop.has_been_placed = HAS_BEEN_PLACED;
	desktop.tips = "desktop " VERSION " | <TAB>switch cursor";
	desktop_tips(desktop.tips);
	handlers_update(&desktop,widget_default_handlers);
	return 0;
}

/**
  * @brief    去初始化 curses 
  * @return   0 
*/
static int desktop_deinit(void)
{
	NWIDGET_MUTEX_DEINIT(placed_mutex);
	memset(&desktop,0,sizeof(desktop));
	printf("\033[?1002l\n"); // Disable mouse movement events, as l = low
	return endwin();
}



#ifdef UNIT_DEBUG

int debug_msg(const char *msg)
{
	static int rely = 1;
	int relx ;
	relx = COLS - 1 - wstrlen(msg);
	if (relx < 1) {
		relx = 1;
	}
	mvaddstr(rely,relx,msg);
	if (++rely >= LINES - 1) {
		werase(stdscr);
		wg_show_border(stdscr);
		rely = 1;
	}
	return 0;
}

#endif

