/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       文本显示控件对象及其管理
  ******************************************************************************
  *
  * COPYRIGHT(c) 2021 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "stringw.h"
#include "wg_mutex.h"
#include "wg_textarea.h"

/* Private macro ------------------------------------------------------------*/
#define MORE " ..."

#define TRY_REDRAW 0

/* Private types ------------------------------------------------------------*/
struct textline {
	struct wg_list node;
	int len;
	char end;
	char text[1];
};

/* Private variables --------------------------------------------------------*/
/* Global  variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
static wg_state_t textarea_line_up(struct nwidget *self,long shortcut);
static wg_state_t textarea_line_down(struct nwidget *self,long shortcut);
static wg_state_t textarea_page_up(struct nwidget *self,long shortcut);
static wg_state_t textarea_page_down(struct nwidget *self,long shortcut);
static int textarea_destroy(struct nwidget *self);
static int textarea_preclose(struct nwidget *wg_entry);
/* Gorgeous Split-line ------------------------------------------------------*/


/**
  * @brief    文本滚动条刷新
  * @param    area : 目标窗体
*/
static void textarea_scrollbar_update(struct textarea *area)
{
	if (area->scrollbar_refresh) {
		area->scrollbar_refresh(area->scrollbar_wg,area->start_display_at,area->lines);
	}
}


/**
  * @brief    表格控件移动函数
  * @param    wg : 控件句柄
  * @param    y : y 轴偏移
  * @param    x : x 轴偏移
  * @return   0
*/
static int textarea_move(struct nwidget *wg,int y,int x)
{
	struct textarea *area = container_of(wg, struct textarea,wg);

	/* 移动过程中不得更新窗口内容，此处需要加锁 */
	desktop_lock();
	wg->rely += y;
	wg->relx += x;
	if (wg->panel)
		move_panel(wg->panel,wg->rely,wg->relx);
	
	/* 当表格有边框或显示列标题时会创建子窗口，需要额外处理 */
	if (area->show_border) {
		y = wg->rely + 1;
		x = wg->relx + 1;
		move_panel(area->panel,y,x);
	}

	desktop_unlock();
	return 0;
}


/**
  * @brief    控件的隐藏/显示函数
  * @param    wg : wg 控件句柄
  * @param    hide : 不为0时执行隐藏函数
  * @return   0
*/
static int textarea_hide(struct nwidget *wg,int hide)
{
	struct textarea *area = container_of(wg, struct textarea,wg);
	int has_sub_window = area->show_border;

	desktop_lock();
	if (hide) {
		hide_panel(wg->panel);
		if (has_sub_window) {
			hide_panel(area->panel);
		}
	} else {
		show_panel(wg->panel);
		if (has_sub_window) {
			show_panel(area->panel);
		}
	}
	
	wg->hidden = hide != 0;
	desktop_unlock();
	return 0;
}

/**
  * @brief    文本框行信息打印
  * @param    area : 目标窗体
*/
static void textarea_lines_info(struct textarea *area)
{
	if (area->show_border && area->wg.width > 14) {
		int x,y;
		char info[13] = {0};
		x = area->wg.width - 12 - 1;
		y = area->wg.height - 1;

		desktop_lock();
		mvwhline(area->wg.win,y,x,wgtheme.bs,12);
		x = snprintf(info,sizeof(info)-1,"%d/%d",area->start_display_at,area->lines);
		x = area->wg.width - x - 1;
		mvwaddstr(area->wg.win,y,x,info);
		desktop_refresh();
		desktop_unlock();
	}
}


/**
  * @brief    文本框内容刷新
  * @param    area : 目标窗体
  * @param    force : 强制输出
  * @note     
*/
static int textarea_refresh(struct textarea *area)
{
	int display,scroll,width,height,y=0;
	struct textline *line;
	struct wg_list *node;
	
	display = area->start_display_at;
	height = area->wg.height - area->show_border;
	width = area->wg.width - area->show_border;

	/* 是否滚动到底部 */
	scroll = (display + height) >= area->lines;
	if (scroll != area->scrollok && area->scrollok != -1) {
		area->scrollok = scroll;
		scrollok(area->win,scroll);
	}

	/* 查找到当前窗口开始显示的行 */
	if (display < area->lines / 2) {
		for (node = area->values.next; display--; node = node->next);
	} else {
		display = area->lines - area->start_display_at;
		for (node = &area->values; display--; node = node->prev);
	}

	desktop_lock();

	/* 从第 display 行开始逐行显示，此处的一行数据有可能会显示为多行，
	   以实际的显示效果为准，显示至最后一行， */
	wmove(area->win,0,0);
	do {
		line = container_of(node,struct textline,node);
		node = node->next;
		waddstr(area->win,line->text);
		y = getcury(area->win);
	} while(y < height-1 && node != &area->values);

	if (node == &area->values) {
		/* 已输出完所有内容 */
	} else if (scroll) {
		/* 如果需滚动至底部，则输出所有的信息 */
		while (node != area->values.prev) {
			/* 除了最后一行的内容全部输出 */
			line = container_of(node,struct textline,node);
			waddstr(area->win,line->text);
			node = node->next;
		}

		/* 处理最后一行，此处不打印最后一行的回车符，这样可以多显示一行内容 */
		line = container_of(node,struct textline,node);
		width = line->len - (line->end == '\n');
		waddnstr(area->win,line->text,width);
	} else {
		mvwhline(area->win,height-1,0,' ',width);
		waddstr(area->win,MORE);
		//mvwaddstr(area->win,height-1,0,MORE);
	}

	desktop_refresh();
	desktop_unlock();

	textarea_lines_info(area);
	textarea_scrollbar_update(area);
	return 0;
}


static int textarea_redraw(struct nwidget *wg)
{
	return textarea_refresh(container_of(wg, struct textarea,wg));
}

static wg_state_t textarea_line_up(struct nwidget *self,long shortcut)
{
	struct textarea *area = container_of(self, struct textarea,wg);
	if (area->start_display_at){
		wg_textarea_jump_to(area,area->start_display_at-1);
	}
	return WG_OK;
}


static wg_state_t textarea_line_down(struct nwidget *self,long shortcut)
{
	struct textarea *area = container_of(self, struct textarea,wg);
	int height = area->wg.height - area->show_border;
	if (area->start_display_at < area->lines - height){
		wg_textarea_jump_to(area,area->start_display_at+1);
	}
	return WG_OK;
}

static wg_state_t textarea_page_up(struct nwidget *self,long shortcut)
{
	struct textarea *area = container_of(self, struct textarea,wg);
	if (area->start_display_at){
		int height = area->wg.height - area->show_border - 2;
		wg_textarea_jump_to(area,area->start_display_at - height);
	}
	return WG_OK;
}


static wg_state_t textarea_page_down(struct nwidget *self,long shortcut)
{
	struct textarea *area = container_of(self, struct textarea,wg);
	int height = area->wg.height - area->show_border;
	if (area->start_display_at < area->lines - height){
		wg_textarea_jump_to(area,area->start_display_at + height - 2);
	}
	return WG_OK;
}


/**
  * @brief    跳转至指定行
  * @param    area : 指定控件
  * @param    target_line : 目标行数
*/
int wg_textarea_jump_to(struct textarea *area,int target_line)
{
	int height;
	NWIDGET_MUTEX_LOCK(area->mutex);
	height = area->wg.height - area->show_border;
	if (target_line < 0) {
		target_line = 0 ;
	} else if (target_line + height > area->lines) {
		target_line = area->lines - height;
	}
	area->start_display_at = target_line;
	if (area->win){
		#if TRY_REDRAW
		/* 此处不直接调用 textarea_refresh(); 
		   因为上下翻一两行时窗口刷新会导致控件闪屏，原因未明;
		   所以先清空内容，然后在桌面刷新后马上重绘控件(textarea_redraw)。
		   虽然会造成文本框内容闪烁，但总比控件闪屏造成的影响小 */
		werase(area->win);
		desktop_redraw(&area->wg);
		#else
		textarea_refresh(area);
		#endif
	}
	NWIDGET_MUTEX_UNLOCK(area->mutex);
	return 0;
}


/**
  * @brief    放置一个 textarea 控件
  * @param    area : textarea
  * @param    parent : 父窗体
  * @param    y : 父窗体的y坐标
  * @param    x : 父窗体的x坐标
  * @return   成功返回 0
*/
int wg_textarea_put(struct textarea *area,struct nwidget *parent,int y,int x)
{
	static const char tips[] = "textarea:<home/end/pgup/pgdn/ARROW>move cursor";
	static const struct wghandler textarea_handlers[] = {
		{KEY_UP,textarea_line_up},
		{KEY_DOWN,textarea_line_down},
		{KEY_LEFT,textarea_page_up},
		{KEY_RIGHT,textarea_page_down},
		{0,0}
	};

	int height,width;

	height = area->wg.height;
	width = area->wg.width;
	if (0 != widget_init(&area->wg,parent,height,width,y,x)) {
		return -1;
	}

	/* 填充背景色 */
	if (area->wg.bkg) {
		wattron(area->wg.win,area->wg.bkg);
		wbkgd(area->wg.win,area->wg.bkg);
	}

	handlers_update(&area->wg,textarea_handlers);
	area->wg.destroy = textarea_destroy;
	area->wg.preclose = textarea_preclose;
	area->wg.move = textarea_move;
	area->wg.hide = textarea_hide;
	area->wg.redraw = textarea_redraw;
	area->wg.tips = tips;

	if (area->show_border) {
		area->win = derwin(area->wg.win,height-2,width-2,1,1);
		area->panel = new_panel(area->win);
		if (area->wg.bkg){
			wattron(area->win,area->wg.bkg);
			wbkgd(area->win,area->wg.bkg);
		}
		wg_show_border(area->wg.win);
	} else {
		area->win = area->wg.win;
		area->panel = area->wg.panel;
	}

	#if 0
	if (area->flags & TEXT_JUMPTO) {
		static const struct wghandler jumpto_handlers[] = {
			{JUMPTO_SHORTCUT,textarea_jumpto},
			{0,0}
		};
		handlers_update(&area->wg,jumpto_handlers);
	}

	if (area->flags & TEXT_FILTER) {
		static const struct wghandler filter_handlers[] = {
			{FILTER_SHORTCUT,textarea_search},
			{0,0}
		};
		handlers_update(&area->wg,filter_handlers);
	}
	#endif

	scrollok(area->win, TRUE);
	area->scrollok = true;

	if (area->lines)
		wg_textarea_jump_to(area,area->start_display_at);
	return 0;
}

/**
  * @brief    初始化文本控件
  * @param    area   : 目标窗体
  * @param    name   : 窗体名称
  * @param    height : 窗体高度
  * @param    width  : 窗体宽度
  * @param    max_lines  : 文本控件最大记录行数
  * @param    flags  : @see enum text_flags 
  * @return   成功返回 0
*/
static int textarea_init(struct textarea *area,int height,int width,int max_lines,int flags)
{
	memset(area, 0, sizeof(struct textarea));
	NWIDGET_MUTEX_INIT(area->mutex);
	area->wg.height = height;
	area->wg.width = width;
	area->max = max_lines;
	area->flags = flags;
	area->values.next = area->values.prev = &area->values;

	if (flags & TEXT_BORDER) {
		area->show_border = 2;
	}
	return 0;
}


/**
  * @brief    创建一个文本窗体
  * @param    name      : 窗体名称
  * @param    height    : 窗体高度
  * @param    width     : 窗体宽度
  * @param    max_lines : 文本控件最大记录行数
  * @param    flags     : @see enum text_flags 
  * @return   成功返回窗体句柄
*/
struct textarea *wg_textarea_create(int height,int width,int max_lines,int flags) 
{
	struct textarea *area;
	area = malloc(sizeof(struct textarea));
	if (!area) {
		return NULL;
	}

	if (textarea_init(area,height,width,max_lines,flags)) {
		free(area);
		return NULL;
	}
	area->created_by = wg_textarea_create;
	DEBUG_MSG("%s(%p)",__FUNCTION__,area);
	return area;
}


/**
  * @brief    控件预关闭函数，控件关闭前通知用户
  * @param    wg : 控件所属的 widget 句柄
  * @note     一般在 widget_delete 中由 area->wg.preclose 调用
  * @return   成功返回 0 
*/
static int textarea_preclose(struct nwidget *wg_entry)
{
	struct textarea *area = container_of(wg_entry,struct textarea,wg);
	if (area->sig.closed)
		area->sig.closed(area,area->sig.closed_arg);
	return 0;
}

/**
  * @brief    销毁一个由 textarea_create 创建的控件，释放控件的内存
  * @param    wg_entry : 控件所属的 widget 句柄
  * @note     一般在 widget_delete 中由 area->wg.destroy 调用
  * @return   成功返回 0 
*/
static int textarea_destroy(struct nwidget *wg_entry)
{
	struct wg_list *node,*next;
	struct textarea *area = container_of(wg_entry,struct textarea,wg);

	NWIDGET_MUTEX_DEINIT(area->mutex);
	if (area->show_border) {
		del_panel(area->panel);
		area->panel = NULL;
		delwin(area->win);
		area->win = NULL;
	}

	node = area->values.next;
	while (node != &area->values) {
		next = node->next;
		free(container_of(node,struct textline,node));
		node = next;
	}
	if (area->created_by == wg_textarea_create){
		free(area);
		DEBUG_MSG("%s(%p)",__FUNCTION__,area);
	}
	return 0;
}


/**
  * @brief  文本显示追加
  * @param  area : 目标窗体
  * @param  str : 追加字符串
  * @note   原理上 scrollok(WINDOW*) 后窗体可实现自动滚动,
  *         但在有背景色的情况下追加 '\n' 的字符串会导致背景色割裂，
  *         所以此处对字符串按照 '\n' 分割，并对每行的空白处进行空字符串填补
  * @return 返回是否需要刷新
*/
int wg_textarea_append(struct textarea *area, const char *str)
{
	int scroll = 0,visible,len,height,append = 0;
	char *tail;
	struct textline *newline,*line;
	struct wg_list *node;

	if (!area || !str || !str[0]) {
		return 0;
	}

	/* 上锁，防止在刷新的时候添加新文本 */
	NWIDGET_MUTEX_LOCK(area->mutex);

	height = area->wg.height - area->show_border;
	visible = (area->lines - area->start_display_at) <= height;

	/* 如果当前最后一行文本不是以 '\n' 结尾，把追加的 str 添加至最后一行文本后面 */
	line = container_of(area->values.prev,struct textline,node);
	if (area->lines && line->end != '\n') {
		tail = strchr(str, '\n');
		len = tail ? (tail - str + 1) : strlen(str);

		/* 把当前行追加至最后一行后面，并替换最后一行的 node 节点 */
		newline = malloc(sizeof(struct textline) + len + line->len);
		memcpy(newline->text,line->text,line->len);
		tail = &newline->text[line->len];
		for (int i = 0; i < len; i++) {
			if (str[i] != '\r')
				*tail++ = str[i];
		}
		str += len;

		*tail = '\0';
		newline->end = tail[-1];
		newline->len = tail - newline->text;
		wg_list_replace(&line->node,&newline->node);
		free(line);
	}

	/* 将新增的内容以 '\n' 分割储存至 area->values 链表 */
	for ( ; *str ; str += len) {
		tail = strchr(str, '\n');
		len = tail ? (tail - str + 1) : strlen(str);
		newline = malloc(sizeof(struct textline) + len);
		tail = newline->text;
		for (int i = 0; i < len; i++) {
			if (str[i] != '\r')
				*tail++ = str[i];
		}
		
		*tail = '\0';
		newline->end = tail[-1];
		newline->len = tail - newline->text;

		/* 加入链表末端 */
		wg_list_add_tail(&newline->node,&area->values);
		if (area->lines >= area->max) {
			/* 超过最大行数，删除最早的数据 */
			node = area->values.next;
			wg_list_del(node);
			free(container_of(node,struct textline,node));
			scroll = true;
		} else {
			area->lines++;
			if (visible && area->lines - area->start_display_at > height){
				area->start_display_at++;
				scroll = true;
			}
		}
		append++;
	}

	if (!area->win) {
		/* 未放置的控件 */
	} else if (!visible){
		/* 在不可视区域内添加内容，只需刷新页脚和滚动条 */
		textarea_lines_info(area);
		textarea_scrollbar_update(area);
	} else if (!scroll) {
		/* 在可视区域内添加内容并且未产生页面滚动，刷新显示 */
		textarea_refresh(area);
	} else if (!area->wg.redraw_requested){
		#if TRY_REDRAW
		/* 追加文本产生页面滚动，先清空文本框内容，然后请求重绘.
		   此处不直接调用 textarea_refresh(); 
		   因为上下滚动一两行时窗口刷新会导致控件闪屏，原因未明;
		   所以先清空内容，然后在桌面刷新后马上重绘控件(textarea_redraw)。
		   虽然会造成文本框内容闪烁，但总比控件闪屏造成的影响小 */
		desktop_lock();
		werase(area->win);
		desktop_redraw(&area->wg);
		desktop_unlock();
		#else
		werase(area->win);
		textarea_refresh(area);
		#endif
	}

	NWIDGET_MUTEX_UNLOCK(area->mutex);
	return visible;
}


/**
  * @brief    文本框内容清空
  * @param    area : 目标窗体
*/
int wg_textarea_clear(struct textarea *area)
{
	int x,y;
	struct wg_list *node,*next;
	NWIDGET_MUTEX_LOCK(area->mutex);
	if (area->win) {
		x = area->wg.width - 12 - 1;
		y = area->wg.height - 1;
		mvwhline(area->wg.win,y,x,wgtheme.bs,12);
		werase(area->win);
		scrollok(area->win, TRUE);
	}

	node = area->values.next;
	area->scrollok = true;
	area->values.next = area->values.prev = &area->values;
	area->lines = area->start_display_at = 0;
	textarea_scrollbar_update(area);
	NWIDGET_MUTEX_UNLOCK(area->mutex);
	
	while (node != &area->values) {
		next = node->next;
		free(container_of(node,struct textline,node));
		node = next;
	}
	desktop_refresh();
	return 0;
}


/**
  * @brief 文本框内容导出至文件
  * @param area : 目标控件
  * @param file : 文件路径
  * @return 成功返回 0
*/
int wg_textarea_export(struct textarea *area,const char *file)
{
	FILE *fp;
	struct wg_list *node;
	struct textline *line;

	fp = fopen(file,"w");
	if (fp == NULL) {
		return -1;
	}
	NWIDGET_MUTEX_LOCK(area->mutex);
	node = area->values.next;
	while(node != &area->values) {
		line = container_of(node,struct textline,node);
		fwrite(line->text,1,line->len,fp);
		node = node->next;
	}
	NWIDGET_MUTEX_UNLOCK(area->mutex);
	fclose(fp);
	return 0;
}

