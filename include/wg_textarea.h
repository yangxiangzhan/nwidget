/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       文本显示控件
  ******************************************************************************
  *
  * COPYRIGHT(c) 2020 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#ifndef __CURSES_TEXTBOX_
#define __CURSES_TEXTBOX_

/* Includes -----------------------------------------------------------------*/
#include "nwidget.h"
#include "wg_list.h"

/* Global macro -------------------------------------------------------------*/

enum text_flags {
	TEXT_BORDER = 0x02,
	TEXT_SRCROLLBAR = 0x06,
	TEXT_JUMPTO = 0x10,
	TEXT_FILTER = 0x20,
};


#define wg_textarea_sprintf(_area,...) \
do {\
	char _str[1024];\
	snprintf(_str,sizeof(_str)-1,__VA_ARGS__);\
	wg_textarea_append(_area,_str);\
}while(0)

/* Global type  -------------------------------------------------------------*/

/** 文本控件 */
typedef struct textarea {
	struct nwidget wg;
	struct nwidget_signal sig;
	WINDOW *win;
	PANEL *panel;
	void *created_by;
	void *mutex;
	int start_display_at;/**< 从第几行开始显示 */
	int lines;/**< number of lines */
	int max;/**< maximum number of lines */
	int flags;
	int show_border;
	int scrollok;/**< 当前窗口是否自动滚动 */

	/** 滚动条控件属于外挂 */
	struct nwidget *scrollbar_wg;
	int (*scrollbar_refresh)(struct nwidget *scrollbar_wg,int current,int max);
	struct wg_list values;
} wg_textarea_t;


/* Global variables  --------------------------------------------------------*/
/* Global function prototypes -----------------------------------------------*/

/**
  * @brief    创建一个文本窗体
  * @param    height    : 窗体高度
  * @param    width     : 窗体宽度
  * @param    max_lines : 文本控件最大记录行数
  * @param    flags     : @see enum text_flags 
  * @return   成功返回窗体句柄
*/
struct textarea *wg_textarea_create(int height,int width,int max_lines,int flags) ;


/**
  * @brief    放置一个 textarea 控件
  * @param    area : textarea
  * @param    parent : 父窗体
  * @param    y : 父窗体的y坐标
  * @param    x : 父窗体的x坐标
  * @return   成功返回 0
*/
int wg_textarea_put(struct textarea *area,struct nwidget *parent,int y,int x);


/**
  * @brief  文本显示追加
  * @param  area : 目标窗体
  * @param  str : 追加字符串
  * @note   原理上 scrollok(WINDOW*) 后窗体可实现自动滚动,
  *         但在有背景色的情况下追加 '\n' 的字符串会导致背景色割裂，
  *         所以此处对字符串按照 '\n' 分割，并对每行的空白处进行空字符串填补
  * @return 返回是否需要刷新
*/
int wg_textarea_append(struct textarea *area, const char *str);


/**
  * @brief    跳转至指定行
  * @param    area : 指定表格
  * @param    target_line : 目标行数
*/
int wg_textarea_jump_to(struct textarea *area,int target_line);


/**
  * @brief    文本框内容清空
  * @param    area : 目标窗体
*/
int wg_textarea_clear(struct textarea *area);


/**
  * @brief 文本框内容导出至文件
  * @param area : 目标控件
  * @param file : 文件路径
  * @return 成功返回 0
*/
int wg_textarea_export(struct textarea *area,const char *file);


#endif /* __CURSES_TEXTBOX_ */
