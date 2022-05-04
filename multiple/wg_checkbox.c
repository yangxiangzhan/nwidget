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
#include "wg_checkbox.h"
#include "stringw.h"

/* Private macro ------------------------------------------------------------*/
/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Global  variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Gorgeous Split-line ------------------------------------------------------*/


/**
  * @brief    控件摆放函数
  * @param    chkbox : 指定控件
  * @param    parent : 母窗口/母控件
  * @param    y : 母窗口的相对位置
  * @param    x : 母窗口的相对位置
  * @return   成功返回 0
*/
int wg_checkbox_put(wg_checkbox_t *chkbox,struct nwidget *parent,int y,int x)
{
	struct choice *choices = chkbox->choices;
	char *values[2] = {0};
	wg_table_column_add(&chkbox->table,"check",3);
	wg_table_column_add(&chkbox->table,"items",chkbox->wg.width - 3 -2);
	if (0 == wg_table_put(&chkbox->table,parent,y,x)) {
		chkbox->wg.tips = "checkbox:<Up/Down>select | <Space/Enter>confirm";
		while(choices->name) {
			values[0] = choices->selected ? wgtheme.chkbox_enabled : wgtheme.chkbox_disabled;
			values[1] = (char *)choices->name;
			choices++;
			wg_table_item_add(&chkbox->table,values);
		}
	}
	return 0;
}


/**
  * @brief 表格选项被点击回调函数
  * @param _table : 指定控件
  * @param arg : wg_signal_connect() 时所指定的参数
  * @return   0
*/
static int checkbox_choice_change(void *_table,void *arg)
{
	int id;
	char *value;
	struct table *table = (struct table *)_table;
	wg_checkbox_t *chkbox = container_of(table,wg_checkbox_t,table);
	struct choice *choices = chkbox->choices;

	id = wg_checkbox_current(chkbox);
	if (id < chkbox->items) {
		choices[id].selected = !choices[id].selected;
		value = choices[id].selected ? wgtheme.chkbox_enabled:wgtheme.chkbox_disabled;
		wg_table_cell_update(table,id,0,value);
		if (chkbox->sig.changed)
			chkbox->sig.changed(chkbox,chkbox->sig.changed_arg);
	}
	return 0;
}


/**
  * @brief 创建一个复选框
  * @param height : 高度
  * @param width : 宽度
  * @param flags : TABLE_BORDER
  * @param choices : 选项值
  * @note choices 需以 {0,0} 结尾，且应为静态变量。因为状态改变时会修改对应的 selected 值
  * @return 控件句柄
*/
wg_checkbox_t *wg_checkbox_create(int height,int width,int flags,struct choice *choices)
{
	wg_checkbox_t *chkbox;
	chkbox = (wg_checkbox_t *)malloc(sizeof(wg_checkbox_t));
	if (NULL == chkbox) {
		return NULL;
	}

	memset(chkbox, 0, sizeof(wg_checkbox_t));

	/* 此处初始化表格并把创建者标记为 wg_table_create,以便在 destroy 的时候释放内存 */
	extern int wg_table_init(struct table *table,int height,int width,int flags);
	wg_table_init(&chkbox->table,height,width,flags & TABLE_BORDER);
	chkbox->table.created_by = wg_table_create;
	
	wg_signal_connect(&chkbox->table,selected,checkbox_choice_change,NULL);
	wg_signal_connect(&chkbox->table,clicked,checkbox_choice_change,NULL);

	chkbox->choices = choices;
	while(choices->name) {
		chkbox->items++;
		choices++;
	}
	return chkbox;
}


/**
  * @brief 单选框的表格选项被点击回调函数
  * @param _table : 单选框的表格控件
  * @param arg : wg_signal_connect() 时所指定的参数
  * @return   0
*/
static int radio_choice_change(void *_table,void *arg)
{
	int id;
	struct table *table = (struct table *)_table;
	wg_radio_t *radio = container_of(table,wg_checkbox_t,table);
	struct choice *choices = radio->choices;

	id = wg_checkbox_current(radio);
	if (id < radio->items && !choices[id].selected) {
		for (int i = 0; i < radio->items; i++) {
			if (choices[i].selected) {
				choices[i].selected = false;
				wg_table_cell_update(table,i,0,wgtheme.radio_disabled);
			}
		}

		choices[id].selected = true;
		wg_table_cell_update(table,id,0,wgtheme.radio_enabled);
		if (radio->sig.changed)
			radio->sig.changed(radio,radio->sig.changed_arg);
	}
	return 0;
}


/**
  * @brief    控件摆放函数
  * @param    radio : 指定控件
  * @param    parent : 母窗口/母控件
  * @param    y : 母窗口的相对位置
  * @param    x : 母窗口的相对位置
  * @return   成功返回 0
*/
int wg_radio_put(wg_radio_t *radio,struct nwidget *parent,int y,int x)
{
	struct choice *choices = radio->choices;
	char *values[2] = {0};
	wg_table_column_add(&radio->table,"check",3);
	wg_table_column_add(&radio->table,"items",radio->wg.width - 3 -2);
	if (0 == wg_table_put(&radio->table,parent,y,x)) {
		radio->wg.tips = "radio:<Up/Down>select | <Space/Enter>confirm";
		while(choices->name) {
			values[0] = choices->selected ? wgtheme.radio_enabled : wgtheme.radio_disabled;
			values[1] = (char *)choices->name;
			choices++;
			wg_table_item_add(&radio->table,values);
		}
	}
	return 0;
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
wg_radio_t *wg_radio_create(int height,int width,int flags,struct choice *choices)
{
	wg_radio_t *radio;
	int has_selected = false;
	radio = wg_checkbox_create(height,width,flags,choices);
	if (radio) {
		while(choices->name) {
			if (!has_selected) {
				has_selected = choices->selected != 0;
			} else if (choices->selected) {
				choices->selected = false;
			}
			choices++;
		}

		wg_signal_connect(&radio->table,selected,radio_choice_change,NULL);
		wg_signal_connect(&radio->table,clicked,radio_choice_change,NULL);
	}
	return radio;
}

