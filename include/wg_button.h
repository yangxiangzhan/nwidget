/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       按钮控件
  ******************************************************************************
  *
  * COPYRIGHT(c) 2021 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#ifndef __CURSES_BUTTON_
#define __CURSES_BUTTON_


/* Includes -----------------------------------------------------------------*/
#include "nwidget.h"

/* Global macro -------------------------------------------------------------*/

/* 按钮最大宽度 */
#define BTN_MAX_WIDTH 64

/* Global type  -------------------------------------------------------------*/
/** 基础表格控件 */
typedef struct button {
	struct nwidget wg;
	struct nwidget_signal sig;
	void *created_by ;
	char label[128];
	int label_len;
	int width;
	int shadow;
}wg_button_t;

/* Global variables  --------------------------------------------------------*/
/* Global function prototypes -----------------------------------------------*/


/**
  * @brief    创建一个按钮
  * @param    label : 按钮标签值
  * @param    width : 按钮显示宽度,<2 时取标签值的显示宽度
  * @return   成功返回按钮句柄
*/
struct button *wg_button_create(const char *label,int width);


/**
  * @brief    创建一个按钮
  * @param    label : 按钮标签值
  * @param    width : 按钮显示宽度,<2 时取标签值的显示宽度
  * @return   成功返回按钮句柄
*/
struct button *wg_button3D_create(const char *label,int width);

/**
  * @brief    按钮放置函数，将按钮显示于窗体或控件上
  * @param    btn : 目标按钮
  * @param    parent : 母窗口/母控件
  * @param    y : 母窗口的相对位置
  * @param    x : 母窗口的相对位置
  * @return   成功返回 0
*/
int wg_button_put(struct button *btn,struct nwidget *parent,int y,int x);


int wg_button_set_text(struct button *btn,const char *text);

#endif /* __CURSES_BUTTON_ */