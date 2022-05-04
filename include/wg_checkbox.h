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
#ifndef __CURSES_CHECKBOX_
#define __CURSES_CHECKBOX_
/* Includes -----------------------------------------------------------------*/
#include "nwidget.h"
#include "wg_table.h"

struct choice {
	const char *name;
	int selected;
};

struct checkbox {
	union {
		struct nwidget wg;
		struct table table;
	};
	
	struct nwidget_signal sig;
	int items;
	struct choice *choices;
};

/** 实际上单选框和复选框都是一种类型 */
typedef struct checkbox wg_checkbox_t;

/** 实际上单选框和复选框都是一种类型 */
typedef struct checkbox wg_radio_t;

/* Global variables  --------------------------------------------------------*/
/* Global function prototypes -----------------------------------------------*/

/**
  * @brief    控件摆放函数
  * @param    chkbox : 指定控件
  * @param    parent : 母窗口/母控件
  * @param    y : 母窗口的相对位置
  * @param    x : 母窗口的相对位置
  * @return   成功返回 0
*/
int wg_checkbox_put(wg_checkbox_t *chkbox,struct nwidget *parent,int y,int x);

/**
  * @brief 创建一个复选框
  * @param height : 高度
  * @param width : 宽度
  * @param flags : TABLE_BORDER
  * @param choices : 选项值
  * @note choices 需以 {0,0} 结尾，且应为静态变量。因为状态改变时会修改对应的 selected 值
  * @return 控件句柄
*/
wg_checkbox_t *wg_checkbox_create(int height,int width,int flags,struct choice *choices);


/**
  * @brief 复选框当前选中行
  * @param chkbox : 控件句柄
  * @return 行数，从 0 开始
*/
static inline int wg_checkbox_current(wg_checkbox_t *chkbox)
{
	return wg_table_current_line(&chkbox->table);
}


/**
  * @brief 创建一个单选框
  * @param height : 高度
  * @param width : 宽度
  * @param flags : TABLE_BORDER
  * @param choices : 选项值
  * @note choices 需以 {0,0} 结尾，且应为静态变量。因为状态改变时会修改对应的 selected 值
  * @return 控件句柄
*/
wg_radio_t *wg_radio_create(int height,int width,int flags,struct choice *choices);

/**
  * @brief    控件摆放函数
  * @param    radio : 指定控件
  * @param    parent : 母窗口/母控件
  * @param    y : 母窗口的相对位置
  * @param    x : 母窗口的相对位置
  * @return   成功返回 0
*/
int wg_radio_put(wg_radio_t *radio,struct nwidget *parent,int y,int x);


/**
  * @brief 单选框当前选中行
  * @param radio : 控件句柄
  * @return 行数，从 0 开始
*/
static inline int wg_radio_current(wg_radio_t *radio)
{
	return wg_table_current_line(&radio->table);
}

#endif /* __CURSES_DROPDOWN_ */

