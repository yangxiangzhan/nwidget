/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       选择框相关控件
  ******************************************************************************
  *
  * COPYRIGHT(c) 2021 GoodMorning
  *
  ******************************************************************************
  */
#ifndef __CURSES_SCROLLBAR_
#define __CURSES_SCROLLBAR_
/* Includes -----------------------------------------------------------------*/
#include "nwidget.h"

/* Global macro -------------------------------------------------------------*/
/* Global type  -------------------------------------------------------------*/

/** 滚动条 */
struct scrollbar {
	struct nwidget wg;
	struct nwidget_signal sig;
	void *created_by;

	/* 放置函数 */
	int (*put_on)(void *self,struct nwidget *parent,int y,int x);
	
	int value;/**< 滚动条当前所在位置的值 */
	int scale;/**< 滚动条最大值 */
	int display_len;/**< 滚动条长度 */
	int slider_pos;/**< 滚动条滑块所在位置 */
	int slider_len;/**< 滚动条滑块宽度 */
	int vertical;/**< 是否为垂直的滚动条 */
	int slow_scroll;/**< 点击滚动条导轨部分时进行翻页，否则跳转至鼠标位置 */
};


/**
 * @brief 点击滚动条导轨位置时的动作
 * @param bar : 滚动条
 * @param set : 为 0 时执行跳转至鼠标位置，不为0时进行翻页
 * @return 0 
 */
static inline int scrollbar_slow_scroll(struct scrollbar *bar,int set)
{
	bar->slow_scroll = set;
	return 0;
}


/**
  * @brief    滚动条值更新
  * @param    bar : 滚动条
  * @param    value : 滚动条当前值
  * @param    scale_max : 滚动条最大值
  * @return   0
*/
int wg_scrollbar_update(struct scrollbar *bar,int value,int scale_max);


/**
  * @brief    创建一个滚动条
  * @param    length : 滚动条总长度
  * @param    vertical : 是否垂直
  * @return   滚动条
*/
struct scrollbar *wg_scrollbar_create(int length,int vertical);


/**
  * @brief    设置滚动条
  * @param    bar : 滚动条
  * @param    length : 滚动条总长度
  * @param    vertical : 是否垂直
  * @return   成功返回0
*/
int wg_scrollbar_init(struct scrollbar *bar,int length,int vertical);


/**
  * @brief    放置滚动条
  * @param    bar : 滚动条
  * @param    parent : 父窗体
  * @param    y : 相对于父窗体的位置
  * @param    x : 相对于父窗体的位置
  * @return   成功返回 0
*/
int wg_scrollbar_put(struct scrollbar *bar,struct nwidget *parent,int y,int x);


#endif /* __CURSES_SCROLLBAR_ */
