/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       文本框/表格的滚动条
  ******************************************************************************
  *
  * COPYRIGHT(c) 2022 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#include "wg_scrollbar.h"
#include "wg_textarea.h"
#include "wg_table.h"

/* Private macro ------------------------------------------------------------*/
/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
static const struct wghandler scrollbar_handlers[] = {
	{KEY_UP,widget_exit_left},
	{KEY_DOWN,widget_exit_left},
	{KEY_LEFT,widget_exit_left},
	{KEY_RIGHT,widget_exit_left},
	{KEY_PPAGE,widget_exit_left},
	{KEY_NPAGE,widget_exit_left},
	{0,0}
};
/* Global  variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Gorgeous Split-line ------------------------------------------------------*/

/**
  * @brief    滚动条设置值
  * @return   成功返回 0
*/
static int scrollbar_refresh(struct nwidget *wg,int line,int max)
{
	struct scrollbar *bar = container_of(wg,struct scrollbar,wg);
	return wg_scrollbar_update(bar,line,max);
}



/**
  * @brief    滚动条值改变事件函数
  * @return   成功返回 0
*/
static int textarea_scrollbar_changed(void *scrollbar,void *related)
{
	struct scrollbar *bar = (struct scrollbar *)scrollbar;
	struct textarea *area = container_of(bar->wg.parent,struct textarea,wg);
	return wg_textarea_jump_to(area,bar->value);
}

/**
  * @brief    文本控件设置滚动条
  * @param    area : 目标窗体
  * @note     仅可用于设置了边框的已放置文本框
  * @return   成功返回 0
*/
int wg_textarea_set_scrollbar(struct textarea *area)
{
	struct scrollbar *bar;

	if (!area || !area->show_border || !area->wg.win) {
		return -1;
	}

	/* 创建垂直滚动条,放置于文本框右边边框上 */
	bar = wg_scrollbar_create(area->wg.height-2,true);
	wg_scrollbar_put(bar,&area->wg,1,area->wg.width-1);

	/* 更新快捷键，输入键值后返回至文本框 */
	handlers_update(&bar->wg,scrollbar_handlers);

	/* 绑定滚动条值改变时的事件 */
	wg_signal_connect(bar,changed,textarea_scrollbar_changed,NULL);

	/* 滚动条不可被 TAB 键找到 */
	bar->wg.found_by_tab = false;

	/* 使能滚动条刷新函数 */
	area->scrollbar_wg = &bar->wg;
	area->scrollbar_refresh = scrollbar_refresh;
	return 0;
}



/**
  * @brief    滚动条值改变事件函数
  * @return   成功返回 0
*/
static int table_scrollbar_changed(void *scrollbar,void *related)
{
	struct scrollbar *bar = (struct scrollbar *)scrollbar;
	struct table *table = container_of(bar->wg.parent,struct table,wg);
	return wg_table_jump_to(table,bar->value);
}


/**
  * @brief    表格设置滚动条
  * @param    table : 目标窗体
  * @note     仅可用于设置了边框的已放置文本框
  * @return   成功返回 0
*/
int wg_table_set_scrollbar(struct table *table)
{
	int height,y=1;
	struct scrollbar *bar;

	if (!table || !table->show_border || !table->wg.win) {
		return -1;
	}

	height = table->wg.height-2;
	if (table->show_title){
		height -= 2;
		y += 2;
	}

	/* 创建垂直滚动条,放置于文本框右边边框上 */
	bar = wg_scrollbar_create(height,true);
	wg_scrollbar_put(bar,&table->wg,y,table->wg.width-1);

	/* 更新快捷键，输入键值后返回至文本框 */
	handlers_update(&bar->wg,scrollbar_handlers);

	/* 绑定滚动条值改变时的事件 */
	wg_signal_connect(bar,changed,table_scrollbar_changed,NULL);

	/* 滚动条不可被 TAB 键找到 */
	bar->wg.found_by_tab = false;

	/* 使能滚动条刷新函数 */
	table->scrollbar_wg = &bar->wg;
	table->scrollbar_refresh = scrollbar_refresh;
	return 0;
}

