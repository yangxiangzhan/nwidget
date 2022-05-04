/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       表格控件对象及其管理
  ******************************************************************************
  *
  * COPYRIGHT(c) GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "wg_mutex.h"
#include "wg_table.h"
#include "stringw.h"

/* Private macro ------------------------------------------------------------*/

#define A_UNFOCUS A_BOLD
#define A_FOCUS A_REVERSE

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Global  variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Gorgeous Split-line ------------------------------------------------------*/

#ifdef VISIBLE_PANEL
	#define TRY_REFRESH(x)
#else
	#define TRY_REFRESH(x) wnoutrefresh(x)
#endif

/**
  * @brief    表格控件移动函数
  * @param    wg : 控件句柄
  * @param    y : y 轴偏移
  * @param    x : x 轴偏移
  * @return   0
*/
static int table_move(struct nwidget *wg,int y,int x)
{
	struct table *table = container_of(wg, struct table,wg);

	/* 移动过程中不得更新窗口内容，此处需要加锁 */
	desktop_lock();
	wg->rely += y;
	wg->relx += x;
	if (wg->panel)
		move_panel(wg->panel,wg->rely,wg->relx);
	
	/* 当表格有边框或显示列标题时会创建子窗口，需要额外处理 */
	if (table->show_border || table->show_title) {
		y = wg->rely;
		x = wg->relx;
		if (table->show_title){
			y += 2;
		}
		
		if (table->show_border) {
			y += 1;
			x += 1;
		}
		#ifdef VISIBLE_PANEL
		mvwin(table->window,y,x);
		#else
		move_panel(table->panel,y,x);
		#endif
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
static int table_hide(struct nwidget *wg,int hide)
{
	struct table *table = container_of(wg, struct table,wg);
	int has_sub_window = table->show_border || table->show_title;

	desktop_lock();
	if (hide) {
		hide_panel(wg->panel);
		if (has_sub_window) {
			hide_panel(table->panel);
		}
	} else {
		show_panel(wg->panel);
		if (has_sub_window) {
			show_panel(table->panel);
		}
	}
	
	wg->hidden = hide != 0;
	desktop_unlock();
	return 0;
}


/**
  * @brief    文本滚动条刷新
  * @param    area : 目标窗体
*/
static void table_scrollbar_update(struct table *table)
{
	if (table->scrollbar_refresh) {
		table->scrollbar_refresh(table->scrollbar_wg,table->start_line,table->lines);
	}
}


/**
  * @brief    获取表格行数据
  * @param    table : 目标表格
  * @param    line : 指定行
  * @return   行数据，如 values[0] 为指定行第一列数据
*/
char **wg_table_values(struct table *table,int line)
{
	int lines;
	struct table_item *item = NULL;
	struct wg_list *node;
	assert(table);
	NWIDGET_MUTEX_LOCK(table->mutex);
	lines = table->keyword[0] ? table->filter : table->lines;
	if (line < 0 || line >= lines) {
		goto unlock;
	}

	if (table->keyword[0]) {
		node = table->filter_items.next;
		for (int i = 0; i < line; i++) {
			node = node->next;
		}
		item = container_of(node,struct table_item,filter);
	} else {
		node = table->items.next;
		for (int i = 0; i < line; i++) {
			node = node->next;
		}
		item = container_of(node,struct table_item,node);
	}
unlock:
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	return item ? item->values : NULL;
}


/**
  * @brief    获取表格指定单元格数据
  * @param    table   : 目标表格
  * @param    line    : 指定行
  * @param    column  : 指定列
  * @return   单元数据
*/
char *wg_table_value(struct table *table,int line,int column)
{
	char **row;
	assert(table);
	if (column < 0 || column >= table->cols) {
		return NULL;
	}

	row = wg_table_values(table,line);
	return row ? row[column] : NULL;
}


/**
  * @brief    删除表格的子面板和子窗口
  * @param    table : 目标表格
  * @param    line : 指定行
  * @param    column : 指定列
  * @return   0
*/
static int visible_column_cleanup(struct table *table)
{
	struct table_column **column = table->visible;
	for (int i = 0; i < table->visible_cols ; i++) {
		column[i]->display_width = 0;
		column[i] = NULL;
	}
	table->visible_cols = 0;
	return 0;
}


/**
  * @brief    检出可显示的列至 table->visible
  * @param    table : 目标表格
  * @param    start_col : 从第几列开始展示
  * @note     即 start_col 以下的列将折叠到左侧不被展示
  * @return   0
*/
static int visible_column_checkout(struct table *table,int start_col)
{
	int has_next_column = 0;
	int width,x,visible_cols = 0;
	struct table_column *column;
	struct wg_list *node = table->column.next;

	/* 查找第 start_col 列可显示的列 */
	for (x = 0; node != &table->column; node = node->next) {
		column = container_of(node,struct table_column,node);
		if (!column->hide && ++x > start_col)
			break;
	}

	if (node == &table->column) {
		goto cleanup;
	}

	/* 除去边框为表格的有效显示宽度 */
	width = table->wg.width - table->show_border;

	/* 将当前窗口可显示的列计入 visible[] 中 */
	visible_cols = 0;
	while (visible_cols < TAB_MAX_COL && node != &table->column) {
		column = container_of(node,struct table_column,node);
		node = node->next;
		if (!column->hide) {
			if (width > column->width){
				column->display_width = column->width;
			} else {
				column->display_width = width;
			}
			table->visible[visible_cols++] = column;

			/* 剩余空间不足 3 列或已遍历完所有节点，跳出 */
			width -= column->display_width;
			if (width < 3) {
				break;
			}
		}
	}

	if (!visible_cols) {
		goto cleanup;
	}

	/* 判断当前表格窗口右侧是否仍有未显示完全的内容 */
	column = table->visible[visible_cols - 1];
	has_next_column = (column->display_width < column->width - 1);
	while (!has_next_column && node != &table->column) {
		column = container_of(node,struct table_column,node);
		node = node->next;
		has_next_column = !column->hide;
	}

	/* 如果显示完所有列后仍有剩余空间，均匀分配至各列 */
	for (int i = 0 ; i < width ; i++) {
		table->visible[i%visible_cols]->display_width++;
	}

cleanup:
	table->visible_cols = visible_cols;
	table->has_prev_column = table->start_col = start_col;
	table->has_next_column = has_next_column;
	return 0;
}


/**
  * @brief    表格重新绘制列标题
  * @param    table : 目标表格
  * @param    draw_hline : 是否绘制水平分割线
  * @return   0
*/
static void table_column_title(struct table *table,int draw_hline)
{
	char value[256];
	int col,y,x,width;
	struct table_column **column = table->visible;

	y = x = table->show_border ? 1 : 0;
	width = table->wg.width - x * 2;

	mvwhline(table->wg.win,y,x,' ',width);
	if (draw_hline){
		chtype hline = table->ts ? table->ts : ACS_HLINE;
		mvwhline(table->wg.win,y+1,x,hline,width);
	}

	for (col = 0; col < table->visible_cols; col++) {
		width = column[col]->display_width;
		wstrncpy(value,column[col]->title,width);
		mvwaddstr(table->wg.win,y,x,value);
		x += width;
	}
	desktop_refresh();
}

/**
  * @brief    表格页脚刷新
  * @param    table : 目标表格
*/
static void table_footer(struct table *table)
{
	char value[13];
	int x,y,lines;

	y = table->wg.height - 1;
	x = table->wg.width - 2;
	lines = table->keyword[0] ? table->filter : table->lines;

	mvwhline(table->wg.win,y,1,table->bs,x);
	if (table->current_line > -1) {
		value[12] = '\0';
		snprintf(value,12,"%d/%d",table->current_line,lines);
		mvwaddstr(table->wg.win,y,1,value);
	}

	if (table->has_prev_column || table->has_next_column) {
		char *stat = "";
		x = table->wg.width-1-7;
		mvwaddch(table->wg.win,y,x,ACS_RTEE);
		mvwaddch(table->wg.win,y,x+6,ACS_LTEE);
		if (!table->has_prev_column)
			stat = "===  ";
		else if (!table->has_next_column)
			stat = "  ===";
		else
			stat = " === ";
		mvwaddstr(table->wg.win,y,x+1,stat);
	}
}

/**
  * @brief    刷新显示一个表格
  * @param    table : 目标表格
  * @param    refresh_title : 是否刷新标题栏
  * @param    refresh_each_row : 每行刷新强制输出
  * @return   0
*/
static int table_refresh_raw(struct table *table,int refresh_title,int refresh_each_row)
{
	char value[256];
	int display,width,height,lines,cols,x,y,i;
	long attr;
	struct table_item *item ;
	struct wg_list *node;
	struct table_column **column = table->visible;
	WINDOW *win = table->window;

	if (!table->wg.win) { /* 未放置的控件 */
		return -1;
	}

	if ( (cols = table->visible_cols) < 0) {
		return 0;
	}

	if (refresh_title && table->show_title){
		table_column_title(table,false);
	}

	lines = table->keyword[0] ? table->filter : table->lines;
	if (lines < 1) {
		return 0;
	}

	/* 当前表格被聚焦时的显示状态 */
	attr = &table->wg.editing ? A_FOCUS : A_UNFOCUS;

	height = table->wg.height - table->show_border;
	display = table->start_line;

	if (table->keyword[0]) {
		node = table->filter_items.next;
		for (int i = 0; i < display; i++) {
			node = node->next;
		}
		item = container_of(node,struct table_item,filter);
	} else {
		node = table->items.next;
		for (int i = 0; i < display; i++) {
			node = node->next;
		}
		item = container_of(node,struct table_item,node);
	}

	desktop_lock();
	werase(table->window);
	for (y = 0; y < height; y++) {
		if (display == table->current_line){
			wattron(win,attr);
		}

		mvwhline(table->window,y,0,' ',table->wg.width);
		for (x = i = 0; i < cols; i++) {
			int id = column[i]->index;
			width = column[i]->display_width;
			wstrncpy(value,item->values[id],width);
			mvwaddstr(win,y,x,value);
			x += width;
		}

		if (display == table->current_line){
			wattroff(win,attr);
		}

		if (refresh_each_row) {
			wnoutrefresh(win);
			doupdate();
		}

		display++;
		node = node->next;
		if (table->keyword[0]) {
			if (node == &table->filter_items) {
				break;
			} else {
				item = container_of(node,struct table_item,filter);
			}
		} else {
			if (node == &table->items) {
				break;
			} else {
				item = container_of(node,struct table_item,node);
			}
		}
	}

	/* 页脚显示行数信息 */
	if (table->show_footer && table->wg.width > 24) {
		table_footer(table);
	}
 
	if (!refresh_each_row){
		desktop_refresh();
	}
	desktop_unlock();

	table_scrollbar_update(table);
	return 0;
}


/**
  * @brief    表格上箭头按键的默认响应函数
  * @param    self     : 目标表格所在的 wg 控件句柄
  * @param    shortcut : 键盘键值
*/
static wg_state_t table_line_up(struct nwidget *self,long shortcut)
{
	int refresh_each_row = false;
	struct table *table = container_of(self, struct table,wg);
	if (table->current_line < 1)
		return WG_OK;
	NWIDGET_MUTEX_LOCK(table->mutex);
	table->current_line--;
	if (table->start_line > table->current_line){
		/* 如果当前行是窗口的第一行，需要整页下移，并强制每行刷新.
		   note:如果此处不强制刷新会导致闪屏，原因未明 */
		refresh_each_row = true;
		table->start_line = table->current_line;
	}

	table_refresh_raw(table,false,refresh_each_row);
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	if (table->sig.changed)
		table->sig.changed(table,table->sig.changed_arg);
	return WG_OK;
}

/**
  * @brief    表格下箭头按键的默认响应函数
  * @param    self     : 目标表格所在的 wg 控件句柄
  * @param    shortcut : 键盘键值
*/
static wg_state_t table_line_down(struct nwidget *self,long shortcut)
{
	int refresh_each_row = false;
	int start_display_at,height,selected,lines;
	struct table *table = container_of(self, struct table,wg);

	lines = table->keyword[0] ? table->filter : table->lines;
	selected = table->current_line;
	start_display_at = table->start_line;
	if (selected >= lines-1){
		return WG_OK;
	}
	NWIDGET_MUTEX_LOCK(table->mutex);
	selected++;
	height = table->wg.height - table->show_border - table->show_title;
	if (selected >= start_display_at + height) {
		/* 如果当前行是窗口的最后一行，需要整页上移，并强制每行刷新.
		   note:如果此处不强制刷新会导致闪屏，原因未明 */
		refresh_each_row = true;
		start_display_at++;
	}

	table->current_line = selected;
	table->start_line = start_display_at;

	table_refresh_raw(table,false,refresh_each_row);
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	if (table->sig.changed)
		table->sig.changed(table,table->sig.changed_arg);
	return WG_OK;
}


/**
  * @brief    表格 page up ，左箭头的默认响应函数
  * @param    self : 目标表格所在的 wg 控件句柄
  * @param    shortcut : 键盘键值
*/
static wg_state_t table_page_up(struct nwidget *self,long shortcut)
{
	int start_display_at,height;
	struct table *table = container_of(self, struct table,wg);
	start_display_at = table->start_line;
	if (start_display_at < 1){
		return WG_OK;
	}
	
	NWIDGET_MUTEX_LOCK(table->mutex);
	height = table->wg.height - table->show_border - table->show_title ;
	start_display_at -= (height - 1);
	if (start_display_at < 0)
		start_display_at = 0;
	table->current_line = start_display_at;
	table->start_line = start_display_at;
	table_refresh_raw(table,false,false);
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	if (table->sig.changed)
		table->sig.changed(table,table->sig.changed_arg);
	return WG_OK;
}

/**
  * @brief    表格 page down 、右箭头的默认响应函数
  * @param    self     : 目标表格所在的 wg 控件句柄
  * @param    shortcut : 键盘键值
*/
static wg_state_t table_page_down(struct nwidget *self,long shortcut)
{
	int start_display_at,height,lines;
	struct table *table = container_of(self, struct table,wg);
	lines = table->keyword[0] ? table->filter : table->lines;
	start_display_at = table->start_line;
	height = table->wg.height - table->show_border - table->show_title ;
	if (start_display_at >= lines - height) {
		return WG_OK;
	}

	NWIDGET_MUTEX_LOCK(table->mutex);
	start_display_at += (height-1);
	if (start_display_at + height > lines)
		start_display_at = lines - height;
	table->current_line = start_display_at;
	table->start_line = start_display_at;
	table_refresh_raw(table,false,false);
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	if (table->sig.changed)
		table->sig.changed(table,table->sig.changed_arg);
	return WG_OK;
}


/**
  * @brief    表格 end 键的默认响应函数
  * @param    self     : 目标表格所在的 wg 控件句柄
  * @param    shortcut : 键盘键值
*/
static wg_state_t table_line_end(struct nwidget *self,long shortcut)
{
	int start_display_at,height,lines;
	struct table *table = container_of(self, struct table,wg);
	lines = table->keyword[0] ? table->filter : table->lines;
	height = table->wg.height - table->show_border - table->show_title ;
	start_display_at = lines - height;
	if (start_display_at < 0 || start_display_at == table->start_line) {
		return WG_OK;
	}
	NWIDGET_MUTEX_LOCK(table->mutex);
	table->current_line = start_display_at;
	table->start_line = start_display_at;
	table_refresh_raw(table,false,false);
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	if (table->sig.changed)
		table->sig.changed(table,table->sig.changed_arg);
	return WG_OK;
}


/**
  * @brief    表格 home 键的默认响应函数
  * @param    self     : 目标表格所在的 wg 控件句柄
  * @param    shortcut : 键盘键值
*/
static wg_state_t table_line_home(struct nwidget *self,long shortcut)
{
	struct table *table = container_of(self, struct table,wg);
	if (!table->current_line && !table->start_line) {
		return WG_OK;
	}
	NWIDGET_MUTEX_LOCK(table->mutex);
	table->current_line = 0;
	table->start_line = 0;
	table_refresh_raw(table,false,false);
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	if (table->sig.changed)
		table->sig.changed(table,table->sig.changed_arg);
	return WG_OK;
}


/**
  * @brief    表格向右滑动展示右边未完全展示的列
  * @param    table
*/
static wg_state_t table_column_move_right(struct table *table)
{
	if (table->has_next_column) {
		NWIDGET_MUTEX_LOCK(table->mutex);
		visible_column_cleanup(table);
		visible_column_checkout(table,table->start_col+1);
		table_refresh_raw(table,true,false);
		NWIDGET_MUTEX_UNLOCK(table->mutex);
	}
	return WG_OK;
}


/**
  * @brief    表格向左滑动展示左边未展示的列
  * @param    table
*/
static wg_state_t table_column_move_left(struct table *table)
{
	NWIDGET_MUTEX_LOCK(table->mutex);
	if (table->has_prev_column) {
		visible_column_cleanup(table);
		visible_column_checkout(table,table->start_col-1);
		table_refresh_raw(table,true,false);
	}
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	return WG_OK;
}


/**
  * @brief 表格滑动展示未展示的列
  * @param self : 目标表格所在的 wg 控件句柄
  * @param key : 键盘键值
*/
static wg_state_t table_column_move(struct nwidget *self,long key)
{
	struct table *table = container_of(self,struct table,wg);
	int move_right = (key == KEY_CTRL_RIGHT || key == '>');

	/* 如果是按住鼠标拖动窗体，则移动方向需要做反处理 */
	if (mouse.bstate)
		move_right ^= 1;
	if (move_right) 
		table_column_move_right(table);
	else
		table_column_move_left(table);
	return WG_OK;
}


/**
  * @brief    表格被选中执行函数，' ','\n' 的执行函数
  * @param    self   : 目标表格所在的 wg 控件句柄
*/
static wg_state_t table_selected(struct nwidget *self,long key)
{
	struct table *table = container_of(self, struct table,wg);
	if (table->current_line > -1 && table->sig.selected)
		table->sig.selected(table,table->sig.selected_arg);
	return WG_OK;
}


/**
  * @brief    对表格的当前选中行进行显示刷新
  * @param    table : 目标表格
  * @param    display_attr : 显示格式
  * @return   0
*/
static int table_refresh_current_line(struct table *table,int display_attr)
{
	struct wg_list *node;
	struct table_item *item;
	struct table_column **column;
	int visible_cols,line;

	NWIDGET_MUTEX_LOCK(table->mutex);
	line = table->current_line;
	column = table->visible;
	visible_cols = table->visible_cols;

	/* 如果无选中行或无显示列 */
	if (line < 0 || visible_cols < 1){
		goto cleanup;
	}
	
	if (table->keyword[0]) {
		/* 如果是过滤过的表格，则查找过滤链表 */
		node = table->filter_items.next;
		for (int i = 0; i < line; i++) {
			node = node->next;
		}
		item = container_of(node,struct table_item,filter);
	} else {
		node = table->items.next;
		for (int i = 0; i < line; i++) {
			node = node->next;
		}
		item = container_of(node,struct table_item,node);
	}

	/* 得到当前行在窗口内的高度 */
	line -= table->start_line;
	if ( visible_cols ) {
		char value[256];
		int id,x = 0;
		desktop_lock();
		wattron(table->window,display_attr);
		mvwhline(table->window,line,x,' ',table->wg.width);
		for (int j = 0 ; j < visible_cols ; j++) {
			id = column[j]->index;
			wstrncpy(value,item->values[id],column[j]->display_width);
			mvwaddstr(table->window,line,x,value);
			x += column[j]->display_width;
		}
		wattroff(table->window,display_attr);
		
		desktop_refresh();
		desktop_unlock();
	}
cleanup:
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	return 0;
}

/**
  * @brief    表格控件聚焦时的回调
  * @param    self : 目标表格所在的 wg 控件句柄
*/
static int table_focus(struct nwidget *self)
{
	struct table *table = container_of(self, struct table,wg);
	table_refresh_current_line(table,A_FOCUS);
	/*if (table->show_title && table->wg.bkg){
		int color,y,x,len;
		y = x = table->show_border ? 1 : 0;
		len = table->wg.width - x * 2;
		if (table->wg.bkg == wgtheme.desktop_bkg )
			color = wgtheme.desktop_fg;
		else 
			color = wgtheme.form_fg;
		wattron(table->wg.win,color);
		mvwhline(table->wg.win,y+1,x,table->ts,len);
		wattroff(table->wg.win,color);
	}*/
	return WG_OK;
}


/**
  * @brief    表格控件失焦时的回调
  * @param    self : 目标表格所在的 wg 控件句柄
*/
static int table_blur(struct nwidget *self)
{
	struct table *table = container_of(self, struct table,wg);
	table_refresh_current_line(table,A_UNFOCUS);
	/*if (table->show_title && table->wg.bkg){
		int y,x,len;
		y = x = table->show_border ? 1 : 0;
		len = table->wg.width - table->show_border;
		mvwhline(table->wg.win,y+1,x,table->ts,len);
	}*/
	return WG_OK;
}


/**
  * @brief    表格鼠标被按下时执行的回调函数，一般仅进行反显
  * @param    self   : 目标表格所在的 wg 控件句柄
  * @return   WG_OK
*/
static wg_state_t table_mousedown(struct nwidget *self)
{
	int selected,last,lines,min;
	struct table *table = container_of(self, struct table,wg);
	last = table->current_line;
	lines = table->keyword[0] ? table->filter : table->lines;
	if (mouse.bstate & BUTTON1_RELEASED){
		return WG_OK;
	}

	min = table->show_title + (table->show_border != 0);
	selected = mouse.y - self->rely ;
	if (selected < min) {
		return WG_OK;
	}

	NWIDGET_MUTEX_LOCK(table->mutex);
	selected -= table->show_border != 0;
	selected -= table->show_title;
	selected += table->start_line;
	if (selected >= lines) {
		NWIDGET_MUTEX_UNLOCK(table->mutex);
		return WG_OK;
	}

	table->current_line = selected;
	if (last != selected)
		table_refresh_raw(table,false,false);

	NWIDGET_MUTEX_UNLOCK(table->mutex);

	if (table->sig.clicked) 
		table->sig.clicked(table,table->sig.clicked_arg);
	
	if (last != selected && table->sig.changed)
		table->sig.changed(table,table->sig.changed_arg); 
	
	/*else if (table->sig.selected){
		table->sig.selected(table,table->sig.selected_arg);
	}*/
	if ((mouse.bstate & BUTTON1_DOUBLE_CLICKED) && table->sig.selected)
		table->sig.selected(table,table->sig.selected_arg);
	return WG_OK;
}


/**
  * @brief    删除一个表格所有数据
  * @param    table : 目标表格
  * @return   成功返回 0 
*/
int table_clear(struct table *table)
{
	struct wg_list *node,*next;

	NWIDGET_MUTEX_LOCK(table->mutex);
	node = table->items.next;
	wg_list_init(&table->items);
	memset(table->keyword,0,sizeof(table->keyword));
	table->start_line = table->filter = table->lines = 0;
	table->current_line = table->current_col = -1;
	NWIDGET_MUTEX_UNLOCK(table->mutex);

	desktop_lock();
	werase(table->window);
	desktop_refresh();
	desktop_unlock();

	table_scrollbar_update(table);

	while (node != &table->items) {
		next = node->next;
		free(container_of(node,struct table_item,node));
		node = next;
	}
	return 0;
}


/**
  * @brief    表格预关闭函数，通知用户层表格将被关闭
  * @param    self : 表格所属的 widget 句柄
  * @return   成功返回 0 
*/
static int table_preclose(struct nwidget *self)
{
	struct table *table = container_of(self, struct table,wg);
	if (table->sig.closed)
		table->sig.closed(table,table->sig.closed_arg);
	return 0;
}


/**
  * @brief    销毁一个表格，释放表格的内存
  * @param    self : 表格所属的 widget 句柄
  * @note     一般在 widget_delete 中由 table->wg.destroy 调用
  * @return   成功返回 0 
*/
static int table_destroy(struct nwidget *self)
{
	int item_num = 0;
	struct wg_list *next,*node;
	struct table *table = container_of(self, struct table,wg);

	if (table->sig.closed)
		table->sig.closed(table,table->sig.closed_arg);

	node = table->items.next;
	while(node != &table->items){
		next = node->next;
		free(container_of(node,struct table_item,node));
		node = next;
		item_num++;
	}

	DEBUG_MSG("%s(free %d items)",__FUNCTION__,item_num);

	visible_column_cleanup(table);
	node = table->column.next;
	while(node != &table->column) {
		next = node->next;
		free(container_of(node, struct table_column,node));
		node = next;
	}

	/* 当表格有边框或显示列标题时会创建子窗口 */
	if (table->show_border || table->show_title) {
		#ifdef VISIBLE_PANEL
			if (table->panel)
				del_panel(table->panel);
		#endif
		if (table->window)
			delwin(table->window);
	}

	NWIDGET_MUTEX_DEINIT(table->mutex);

	/* 由 table_create 创建的表格需要释放内存 */
	if (table->created_by == wg_table_create){
		DEBUG_MSG("%s(%p)",__FUNCTION__,table);
		free(table);
	}
	return 0;
}


/**
  * @brief    filter editline '\n' 的执行函数
  * @param    _filter   : filter editline
  * @param    _table    : 回调参数
*/
static int table_column_do_hide(void *_selection,void *_table)
{
	struct wg_list *node;
	struct table_column *column;
	struct table *selection,*table;
	int col,hide;
	
	selection = (struct table *)_selection;
	table = (struct table *)_table;
	col = wg_table_current_line(selection);
	
	NWIDGET_MUTEX_LOCK(table->mutex);
	node = table->column.next;
	for (int i = 0; i < col; i++) {
		node = node->next;
	}
	column = container_of(node,struct table_column,node);
	hide = !column->hide;
	if (hide) {
		/* 遍历所有列，如果仅存 1 列不允许隐藏 */
		int show = 0;
		node = table->column.next;
		while (node != &table->column) {
			struct table_column *check;
			check = container_of(node,struct table_column,node);
			node = node->next;
			show += !check->hide;
		}
		if (show < 2) {
			hide = 0;
			beep();
			goto unlock;
		}
	}

	column->hide = hide;
	visible_column_cleanup(table);
	visible_column_checkout(table,0);
	table_refresh_raw(table,true,false);
unlock:
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	wg_table_cell_update(selection,col,0,hide ? "[ ]":"[*]");
	return WG_OK;
}


/**
  * @brief    响应 '/' 弹出列隐藏表格弹框
  * @param    self : 目标表格所在的 wg 控件句柄
  * @param    key : 键
*/
static wg_state_t table_column_hide(struct nwidget *self,long key)
{
	static const struct wghandler wg_handlers[] = {
		{'/',widget_exit_left},
		{'\t',widget_exit_left},
		{'\e',widget_exit_left},
		{KEY_BACKSPACE,widget_exit_left},
		{0,0}
	};
	int height,width,x,y;
	char *values[2];
	struct wg_list *node;
	struct table_column *column;
	struct table *selection,*table = container_of(self, struct table,wg);
	if (table->cols < 2) {
		beep();
		return WG_OK;
	}

	/* 换算弹框的位置，尽量在原表格的居中位置 */
	height = table->cols + 4;
	if (table->wg.height > height) {
		y = table->wg.rely + (table->wg.height-height)/2;
		height = height;
	} else {
		y = table->wg.rely;
		height = table->wg.height;
	}

	if (table->wg.width > 32) {
		x = table->wg.relx + (table->wg.width-32)/2;
		width = 32;
	} else if (table->wg.width >= 16) {
		x = table->wg.relx + 1;
		width = table->wg.width - 2;
	} else {
		x = table->wg.relx - (16-table->wg.width)/2;
		width = 16;
	}

	char msg[256];
	snprintf(msg, sizeof(msg),"%d,%d",y,x);
	desktop_tips(msg);

	/* 创建一个新的表格覆盖于当前表格之上 */
	selection = wg_table_create(height,width,TABLE_BORDER|TABLE_TITLE);
	wg_table_column_add(selection,"",3);
	wg_table_column_add(selection,"column name",32);

	/* 新建的表格悬空，使其失焦后自动销毁 */
	wg_table_put(selection,NULL,y,x);

	/* 更新键盘响应和信号，在选择窗口被销毁后刷新原表格的列 */
	handlers_update(&selection->wg,wg_handlers);
	wg_signal_connect(selection,selected,table_column_do_hide,table);
	
	for (node = table->column.next; node != &table->column; node = node->next) {
		column = container_of(node,struct table_column,node);
		values[0] = column->hide ? "[ ]" : "[*]";
		values[1] = column->title;
		wg_table_item_add(selection,values);
	}

	return WG_OK;
}

/**
  * @brief    放置一个 table 控件
  * @param    table : 表格
  * @param    parent : 父窗体
  * @param    y : 父窗体的y坐标
  * @param    x : 父窗体的x坐标
  * @return   成功返回 0
*/
int wg_table_put(struct table *table,struct nwidget *parent,int y,int x)
{
	static const char tips[] = 
		"table:<home/end/pgup/pgdn/ARROW>move cursor |"
		"('<'/'>')move visible column |"
	;
	int height,width,lines;
	static const struct wghandler table_handlers[] = {
		{KEY_UP   ,table_line_up},
		{KEY_DOWN ,table_line_down},
		{KEY_LEFT ,table_page_up},
		{KEY_RIGHT,table_page_down},
		{KEY_PPAGE,table_page_up},
		{KEY_NPAGE,table_page_down},
		{KEY_HOME ,table_line_home},
		{KEY_END  ,table_line_end},
		{KEY_CTRL_LEFT,table_column_move},
		{KEY_CTRL_RIGHT,table_column_move},
		{'>',table_column_move},
		{'<',table_column_move},
		{' ' ,table_selected},
		{'\n',table_selected},
		{0,0}
	};

	height = table->wg.height;
	width = table->wg.width;
	if (!width || !height) {
		return -1;
	}

	if (0 != widget_init(&table->wg,parent,height,width,y,x)) {
		return -1;
	}

	/* 填充背景色 */
	if (table->wg.bkg)
		wbkgd(table->wg.win,table->wg.bkg);

	visible_column_checkout(table,0);
	
	y = x = 0;
	if (table->show_border) {
		x += 1;
		y += 1;
		width -= 2;
		height -= 2;
		wborder(table->wg.win,
			table->ls,table->rs,table->ts,table->bs,
			table->tl,table->tr,table->bl,table->br);
		
	}

	if (table->show_title){
		y += 2;
		height -= 2;
		table_column_title(table,true);
	}

	if (!x && !y) {
		table->window = table->wg.win;
		table->panel = table->wg.panel;
	} else {
		table->window = derwin(table->wg.win,height,width,y,x);
		table->panel = new_panel(table->window);
		if (table->wg.bkg)
			wbkgd(table->window,table->wg.bkg);
	}

	/* 如果使能了列隐藏功能，新增 '/' 键响应 */
	if (table->option & TABLE_COL_HIDE) {
		static const struct wghandler hide_handlers[] = {
			{'/' ,table_column_hide},
			{0,0}
		};
		handlers_update(&table->wg,hide_handlers);
	}

	/* table 控件会创建子窗体，需更新动作响应函数 */
	handlers_update(&table->wg,table_handlers);
	table->wg.handle_mouse_event = table_mousedown;
	table->wg.focus = table_focus;
	table->wg.blur = table_blur;
	table->wg.destroy = table_destroy;
	table->wg.preclose = table_preclose;
	table->wg.move = table_move;
	table->wg.hide = table_hide;
	table->wg.tips = tips;

	lines = table->keyword[0] ? table->filter : table->lines;
	if (lines > 0) {
		/* 放置前已有数据，刷新 */
		NWIDGET_MUTEX_LOCK(table->mutex);
		table_refresh_raw(table,false,false);
		NWIDGET_MUTEX_UNLOCK(table->mutex);
	}
	return 0;
}


/**
  * @brief    初始化一个 table 控件
  * @param    table  : table 句柄
  * @param    height : 控件高度
  * @param    width  : 控件宽度
  * @param    flags  : 控件参数 @see enum table_flags
  * @return   成功返回 0
*/
int wg_table_init(struct table *table,int height,int width,int flags)
{
	if (height < 1) {
		return -1;
	}
	memset(table, 0, sizeof(struct table));
	NWIDGET_MUTEX_INIT(table->mutex);
	
	wg_list_init(&table->column);
	wg_list_init(&table->items);
	wg_list_init(&table->filter_items);

	table->current_col = -1;
	table->current_line = -1;
	table->wg.height = height;
	table->wg.width = width;
	table->option = flags;
	table->show_title = (flags & TABLE_TITLE) ? 2 : 0;
	table->show_border = (flags & TABLE_BORDER) ? 2 : 0;
	table->show_footer = (flags & TABLE_FOOTER) == TABLE_FOOTER;
	
	wg_table_border(table,wgtheme.ls,wgtheme.rs,wgtheme.ts,wgtheme.bs,
		wgtheme.tl,wgtheme.tr,wgtheme.bl,wgtheme.br);
	return 0;
}


/**
  * @brief    创建一个 table 控件
  * @param    height : 控件高度
  * @param    width  : 控件宽度
  * @param    flags  : 控件参数 @see enum table_flags
  * @return   成功返回 table 句柄，否则返回 0
*/
struct table *wg_table_create(int height,int width,int flags)
{
	struct table *table = malloc(sizeof(struct table));
	if (table == NULL) {
		return NULL;
	}
	
	if (0 != wg_table_init(table,height,width,flags) ) {
		free(table);
		return NULL;
	}

	table->created_by = wg_table_create;
	DEBUG_MSG("%s(%p)",__FUNCTION__,table);
	return table;
}


/**
  * @brief    设置 table 控件的边框显示字符
  * @return   成功返回 0
*/
int wg_table_border(struct table *table,chtype ls,chtype  rs,
	chtype  ts,chtype  bs,chtype  tl,chtype  tr,chtype  bl,chtype  br)
{
	table->ls = ls ? ls : ACS_VLINE;
	table->rs = rs ? rs : ACS_VLINE;
	table->ts = ts ? ts : ACS_HLINE;
	table->bs = bs ? bs : ACS_HLINE;
	table->tl = tl ? tl : ACS_ULCORNER;
	table->tr = tr ? tr : ACS_URCORNER;
	table->bl = bl ? bl : ACS_LLCORNER;
	table->br = br ? br : ACS_LRCORNER;
	return 0;
}


/**
  * @brief    table 控件添加一列
  * @param    table : table 句柄
  * @param    column_name : 新列列名
  * @param    width : 列显示宽度
  * @return   成功返回 0
*/
int wg_table_column_add(struct table *table,char *column_name,int width)
{
	int size = 0;
	struct table_column *newcol;

	if (width < 1) {
		return 0;
	}

	size = column_name ? strlen(column_name) : 0;
	size += sizeof(struct table_column);
	if (NULL == (newcol = malloc(size))) {
		return -1;
	}
	memset(newcol,0,size);
	if (column_name) 
		strcpy(newcol->title,column_name);

	newcol->width = (width > table->wg.width) ? table->wg.width-3 : width;

	NWIDGET_MUTEX_LOCK(table->mutex);
	wg_list_add_tail(&newcol->node,&table->column);
	newcol->index = table->cols++;

	/* 已经放置了的控件，进行显示 */
	if (table->wg.win && table->visible_cols < TAB_MAX_COL) {
		if (table->visible_cols)
			visible_column_cleanup(table);
		table->start_col = -1;
		visible_column_checkout(table,0);
		table_refresh_raw(table,true,false);
	}
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	return 0;
}


/**
  * @brief    向表格添加一行
  * @param    table  : 目标窗体
  * @param    values : 行内容
  * @return   成功返回 table_item 中的 values 数组 
*/
char **wg_table_item_add(struct table *table,char *values[])
{
	char *value ;
	int size,visible_height,display,lines;
	struct table_item *newitem;

	size = sizeof(struct table_item) ;
	size += sizeof(char *) * table->cols; /* for table_item->values[] */
	size += (table->wg.width * 2) * table->cols ; 
	
	if (!(newitem = (struct table_item *)malloc(size))) {
		return NULL;
	}
	memset(newitem,0,size);

	value = (char *)&newitem->values[table->cols] ;
	for (int i = 0 ; i < table->cols ; i++) {
		newitem->values[i] = value;
		if (values[i])
			strncpy(value,values[i],table->wg.width*2-1);
		value += table->wg.width * 2;
	}

	NWIDGET_MUTEX_LOCK(table->mutex);

	wg_list_init(&newitem->node);
	wg_list_init(&newitem->filter);
	wg_list_add_tail(&newitem->node,&table->items);

	visible_height = table->wg.height - table->show_border;
	lines = table->keyword[0] ? table->filter : table->lines;
	display = lines - table->start_line;
	table->lines++;

	if (NULL == table->wg.win) {
		/* 未放置的控件 */
	} else if (table->visible_cols && display <= visible_height) {
		/* 在可视区域添加行，进行内容刷新 */
		char value[256];
		int id,width,x = 0;
		struct table_column **column = table->visible;
		
		/* 可见范围内新增行 */
		desktop_lock();
		for (int i = 0; i < table->visible_cols ; i++){
			id = column[i]->index;
			width = column[i]->display_width;
			wstrncpy(value,newitem->values[id],width);
			mvwaddstr(table->window,display,x,value);
			x += width;
		}
		desktop_refresh();
		desktop_unlock();

		table_scrollbar_update(table);
	} else {
		/* 在不可视区域添加行，只需刷新滚动条 */
		table_scrollbar_update(table);
	}

	NWIDGET_MUTEX_UNLOCK(table->mutex);
	return newitem->values;
}


/**
  * @brief    更新单元格的值
  * @param    table  : 目标窗体
  * @param    line   : 目标行
  * @param    col    : 目标列
  * @param    value  : 内容
  * @return   成功返回0 
*/
int wg_table_cell_update(struct table *table,int line,int col,const char *value)
{
	int visible_height,is_current_line,x = 0;
	WINDOW *win;
	struct wg_list *node;
	struct table_item *item;
	struct table_column *column;
	
	assert(table && value);
	if (line > table->lines || col > table->cols) {
		return -1;
	}

	NWIDGET_MUTEX_LOCK(table->mutex);
	node = table->items.next;
	for (int i = 0; i < line ; i++) {
		node = node->next;
	}
	item = container_of(node,struct table_item,node);
	strncpy(item->values[col],value,table->wg.width*2);

	/* 如果表格未放置，退出 */
	if (NULL == (win = table->window)) {
		goto cleanup;
	}

	/* 移动至指定列 */
	node = table->column.next;
	for (int i = 0; i < col ; i++) {
		column = container_of(node,struct table_column,node);
		node = node->next;
		x += column->display_width;
	}

	column = container_of(node,struct table_column,node);
	
	/* 可视区域高度 */
	visible_height = table->wg.height - table->show_border - table->show_title;
	
	/* 得到所更新行在可视区域的高度，如果在可视化区域内则进行更新 */
	is_current_line = line == table->current_line;
	line -= table->start_line;
	if (line >= 0 && line < visible_height) {
		char str[256];
		long attr = table->wg.editing ? A_FOCUS : A_UNFOCUS;
		int width = column->display_width;
		
		wstrncpy(str,value,width);

		desktop_lock();
		if (is_current_line) {
			wattron(win,attr);
			mvwhline(win,line,x,' ',width);
			mvwaddstr(win,line,x,str);
			wattroff(win,attr);
		} else {
			mvwhline(win,line,x,' ',width);
			mvwaddstr(win,line,x,str);
		}
		
		if (!table->wg.hidden)
			desktop_refresh();
		desktop_unlock();
	}
cleanup:
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	return 0;
}


/**
  * @brief    跳转至指定行
  * @param    table       : 指定表格
  * @param    target_line : 目标行数
*/
int wg_table_jump_to(struct table *table,int target_line)
{
	int visible_height,lines;
	lines = table->keyword[0] ? table->filter : table->lines;
	if (lines < 1 || target_line == table->current_line) {
		return 0;
	}

	if (target_line >= lines) {
		target_line = lines - 1;
	} else if (target_line < 0) {
		target_line = 0;
	}

	NWIDGET_MUTEX_LOCK(table->mutex);
	visible_height = table->wg.height - table->show_border - table->show_title ;
	if (target_line + visible_height > lines) {
		table->start_line = lines - visible_height;
	} else {
		table->start_line = target_line;
	}
	table->current_line = target_line;
	table_refresh_raw(table,false,false);
	NWIDGET_MUTEX_UNLOCK(table->mutex);
	if (table->sig.changed) {
		table->sig.changed(table,table->sig.changed_arg);
	}
	return 0;
}


/**
  * @brief    清空表格内容
  * @param    table : 目标控件
  * @return   成功返回0 
*/
int wg_table_clear(wg_table_t *table)
{
	struct wg_list *node,*next;
	if (!table || !table->lines) {
		return 0;
	}

	NWIDGET_MUTEX_LOCK(table->mutex);
	table->current_line = -1;
	table->lines = 0;
	node = table->items.next;
	wg_list_init(&table->items);
	wg_list_init(&table->filter_items);
	werase(table->window);
	NWIDGET_MUTEX_UNLOCK(table->mutex);

	while(node != &table->items){
		next = node->next;
		free(container_of(node,struct table_item,node));
		node = next;
	}

	if (table->sig.changed)
		table->sig.changed(table,table->sig.changed_arg);
	return 0;
}