/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       输入控件对象及其管理
  ******************************************************************************
  *
  * COPYRIGHT(c) 2021 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#ifndef __CURSES_EDITLINE_
#define __CURSES_EDITLINE_

/* Includes -----------------------------------------------------------------*/
#include "nwidget.h"

/* Global macro -------------------------------------------------------------*/
#define MAX_LINE_LENGTH 256
#define EDITLINE_LABAL_MAX 32

/* Global type  -------------------------------------------------------------*/

/** 编辑输入框 */
typedef struct editline {
	struct nwidget wg;
	struct nwidget_signal sig;

	void *created_by;
	void *mutex;
	char label[EDITLINE_LABAL_MAX * 3] ;
	char value[MAX_LINE_LENGTH];/**< 输入缓存区 */
	int start_display_at;/**< 输入信息开始显示的位置 */
	int tail;/**< 输入长度，末尾 */
	int cursor;/**< 当前光标位置 */
	int available;/**< 除去标签宽度后的可输入长度 */
	int edit;
	int labelen;
}
wg_editline_t;


/* Global variables  --------------------------------------------------------*/
extern const struct wghandler editline_default_handlers[];
/* Global function prototypes -----------------------------------------------*/


/**
  * @brief    创建输入控件
  * @param    label : 输入标签
  * @param    width : 控件宽度
  * @return   输入控件
*/
wg_editline_t *wg_editline_create(const char *label,int width);


/**
  * @brief    输入控件放置函数，显示
  * @param    line : 输入控件
  * @param    parent : 父窗体
  * @param    y : 父窗体坐标
  * @param    x : 父窗体坐标
  * @return   成功返回 0
*/
int wg_editline_put(struct editline *line,struct nwidget *parent,int y,int x);


/**
  * @brief    设置输入控件值
  * @param    line     : 输入控件
  * @param    text     : 值，字符串
  * @return   成功返回0
*/
int wg_editline_set_text(wg_editline_t *line,const char *text);


/**
  * @brief    设置输入控件值
  * @param    line     : 输入控件
  * @param    text     : 值，字符串
  * @return   成功返回0
*/
int wg_editline_append(wg_editline_t *line,const char *text,int len);


/**
  * @brief    控件编辑点移动至末尾
  * @param    line     : 输入控件
  * @return   成功返回0
*/
int wg_editline_edit_end(wg_editline_t *line);

/**
  * @brief    控件编辑点移动至末尾
  * @param    line     : 输入控件
  * @return   成功返回0
*/
int wg_editline_edit_home(wg_editline_t *line);


/**
  * @brief    输入控件当前输入值
  * @param    line : 输入控件
  * @return   值，字符串
*/
static inline const char *wg_editline_value(wg_editline_t *line)
{
	return line->value;
}

#endif /* __CURSES_INPUTBOX_ */
