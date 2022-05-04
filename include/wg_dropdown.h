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
#ifndef __CURSES_DROPDOWN_
#define __CURSES_DROPDOWN_
/* Includes -----------------------------------------------------------------*/
#include "nwidget.h"


typedef void (*menufn_t)(const char *item,void *arg);

/** 下拉框条目 */
struct menuitem {
	struct menuitem *next;
	char str[128];
	int width;
	void *data;
	menufn_t func;
};

/** 下拉框控件 */
struct dropdown {
	struct nwidget wg;
	struct nwidget_signal sig;
	char label[128];
	int labelw;/**< wstrlen(label); */
	int width;
	int items;
	
	int is_dropdown;/** dropdown/menu */

	int menu_width;
	int menu_height;
	int menux,menuy;

	void *data;
	menufn_t func;
	
	void *created_by;
	const char *selected;
	struct menuitem *menu;
};


typedef struct dropdown wg_dropdown_t;

typedef struct dropdown wg_menu_t;



/**
  * @brief    创建下拉框
  * @param    label    : 下拉框标签
  * @param    width    : 窗体宽度
  * @return   成功返回 0
*/
struct dropdown *wg_dropdown_create(const char *label,int width);


/**
  * @brief    放置函数，将控件显示于窗体或控件上
  * @param    btn : 目标控件
  * @param    parent : 母窗口/母控件
  * @param    y : 母窗口的相对位置
  * @param    x : 母窗口的相对位置
  * @return   成功返回 0
*/
int wg_dropdown_put(struct dropdown *dropdown,struct nwidget *parent,int y,int x);


/**
  * @brief    为下拉框添加条目
  * @param    dropdown : 目标下拉框
  * @param    value : 新条目内容
  * @return   成功返回 0
*/
int wg_dropdown_item_add(struct dropdown *dropdown,const char *value);


/**
  * @brief    设置下拉框的值
  * @param    dropdown : 目标下拉框
  * @param    text     : 目标值
  * @note     text 必须为下拉列表的值
  * @return   成功返回 0
*/
int wg_dropdown_select(struct dropdown *dropdown,const char *text);


/**
  * @brief    获取当前下拉框的值
  * @param    dropdown : 目标下拉框
  * @return   成功当前值字符串
*/
static inline const char *wg_dropdown_value(struct dropdown *dropdown)
{
	return dropdown->selected ;
}


/**
  * @brief    创建菜单入口
  * @param    label : 菜单入口标签
  * @return   成功句柄，否则返回NULL
*/
wg_menu_t *wg_menu_create(const char *label);
#define wg_menu_put wg_dropdown_put
#define wg_menu_item_add wg_dropdown_item_add
#define wg_menu_value wg_dropdown_value

#endif /* __CURSES_DROPDOWN_ */