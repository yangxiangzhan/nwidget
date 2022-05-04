/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       表格控件
  ******************************************************************************
  *
  * COPYRIGHT(c) 2021 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#ifndef __CURSES_TABLE_
#define __CURSES_TABLE_


/* Includes -----------------------------------------------------------------*/
#include "nwidget.h"
#include "wg_list.h"

/* Global macro -------------------------------------------------------------*/

#define VISIBLE_PANEL (1)

/* 表格可视窗口同时可显示的最大列数，超过这个列数将需要用 ctrl+left 和 ctrl+right 横向滚动显示 */
#define TAB_MAX_COL     16


enum table_flags {
	TABLE_BORDER = 0x01,
	TABLE_SRCROLLBAR = 0x03,
	TABLE_FOOTER = 0x05,
	TABLE_TITLE = 0x08,
	TABLE_JUMPTO = 0x10,
	TABLE_FILTER = 0x20,
	TABLE_COL_HIDE = 0x40,
};

/* Global type  -------------------------------------------------------------*/

/** 表格每行的内容，双向链表 */
struct table_item {
	struct wg_list node;
	struct wg_list filter;
	char *values[1];
};


/** 表格列 */
struct table_column {
	struct wg_list node;
	/**<int relx,rely; 窗口绝对坐标 */
	int index;/**< 当前列在表格中的列序号,>= 0 */
	int width;/**< 最小宽度 */
	int display_width;/**< 实际显示宽度 */
	int hide;
	char title[4];
};

/** 基础表格控件 */
typedef struct table {
	struct nwidget wg;
	struct nwidget_signal sig;
	
	void *created_by;
	void *mutex;

	struct wg_list items;
	struct wg_list filter_items;
	struct wg_list column;

	WINDOW *window;/**< 可视区域子窗口 */
	
	#ifdef VISIBLE_PANEL
		PANEL *panel;/**< 可视区域子窗口面板 */
	#endif
	
	struct table_column *visible[TAB_MAX_COL];/**< 当前表格窗口的可视列 */
	int visible_cols;/**< 当前表格窗口的可视列列数 */

	char keyword[128];/**< 表格条目过滤词 */
	int filter;
	int option;
	int show_footer;
	int show_title;/**< 显示列标题 */
	int show_border;/**< 显示边框 */
	int show_scrollbar;
	int lines;/**< 当前表格总条目行数 */
	int cols;/**< 当前表格总列数 */
	int current_line;/**< 当前选中行 */
	int current_col;/**< 当前选中列 */
	int start_line;/**< 当前界面下第一条显示的行 */
	int start_col;/**< 当前界面下第一条显示的列 */
	int has_prev_column,has_next_column;
	chtype ls, rs, ts, bs, tl, tr, bl, br;
	
	/** 滚动条控件属于外挂 */
	struct nwidget *scrollbar_wg;
	int (*scrollbar_refresh)(struct nwidget *scrollbar_wg,int current,int max);
}wg_table_t;


/* Global variables  --------------------------------------------------------*/
/* Global function prototypes -----------------------------------------------*/


/**
  * @brief    创建一个 table 控件
  * @param    height : 控件高度
  * @param    width  : 控件宽度
  * @param    flags  : 控件参数 @see enum table_flags
  * @return   成功返回 table 句柄，否则返回 0
*/
struct table *wg_table_create(int height,int width,int flags);


/**
  * @brief    放置一个 table 控件
  * @param    table : 表格
  * @param    parent : 父窗体
  * @param    y : 父窗体的y坐标
  * @param    x : 父窗体的x坐标
  * @return   成功返回 0
*/
int wg_table_put(struct table *table,struct nwidget *parent,int y,int x);


/**
  * @brief    table 控件添加一列
  * @param    table  : table 句柄
  * @param    column_name : 新列列名
  * @param    width : 列显示宽度
  * @return   成功返回 0
*/
int wg_table_column_add(struct table *table,char *column_name,int width);


/**
  * @brief    table 控件搜索检出
  * @param    table  : table 句柄
  * @param    filter : 关键词，检索词
  * @return   成功返回 检出数
*/
int wg_table_filter(struct table *table,const char *filter);


static inline int wg_table_current_line(struct table *table)
{
	return table->current_line;
}


/**
  * @brief    设置 table 控件的边框显示字符
  * @return   成功返回 0
*/
int wg_table_border(struct table *table,chtype ls,chtype  rs,
	chtype  ts,chtype  bs,chtype  tl,chtype  tr,chtype  bl,chtype  br);


/**
  * @brief    向表格添加一行
  * @param    field  : 目标窗体
  * @param    values : 行内容
  * @return   成功返回 table_item 中的 values 数组 
*/
char **wg_table_item_add(struct table *table,char *values[]) ;


/**
  * @brief    跳转至指定行
  * @param    field       : 指定表格
  * @param    target_line : 目标行数
*/
int wg_table_jump_to(struct table *field,int target_line);


/**
  * @brief    删除一个表格所有数据
  * @param    table : 目标表格
  * @return   成功返回 0 
*/
int table_clear(struct table *table);


/**
  * @brief    获取表格行数据
  * @param    field   : 目标表格
  * @param    line    : 指定行
  * @return   行数据，如 values[0] 为指定行第一列数据
*/
char **wg_table_values(struct table *field,int line);


/**
  * @brief    获取表格指定单元格数据
  * @param    field   : 目标表格
  * @param    line    : 指定行
  * @param    column  : 指定列
  * @return   单元数据
*/
char *wg_table_value(struct table *field,int line,int column);


/**
  * @brief    更新单元格的值
  * @param    table   : 目标窗体
  * @param    lines  : 目标行
  * @param    cols   : 目标列
  * @param    value  : 内容
  * @return   成功返回0 
*/
int wg_table_cell_update(struct table *table,int lines,int cols,const char *value);


/**
  * @brief    清空表格内容
  * @param    table : 目标控件
  * @return   成功返回0 
*/
int wg_table_clear(wg_table_t *table);

#endif /* __CURSES_GRID_ */
