/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       弹出式窗体对象及其管理
  ******************************************************************************
  *
  * COPYRIGHT(c) 2021 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#ifndef __CURSES_FORM_
#define __CURSES_FORM_

/* Includes -----------------------------------------------------------------*/
#include "nwidget.h"

/* Global macro -------------------------------------------------------------*/
#define WG_SHADOW    (0x80)  /**< 3D form */
#define WG_UNMOVABLE (0x100) /**< unmovable form */

/* Global type  -------------------------------------------------------------*/

/** 基础窗体 */
typedef struct form {
	struct nwidget wg;
	void *created_by;
	char tips[128];
	char title[80];
	int titlew;
	int width,height;
	int shadow;
	int movable;
	WINDOW *wr,*wb;/**< 右侧阴影和底部阴影所在的窗口 */
	PANEL *pr,*pb;/**< 右侧阴影和底部阴影所在的面板 */
} wg_form_t;

/* Global variables  --------------------------------------------------------*/
/* Global function prototypes -----------------------------------------------*/

/**
  * @brief    创建一个基础窗体
  * @param    title : 窗体标题
  * @param    height : 窗体高度
  * @param    width : 窗体宽度
  * @param    flags : WG_SHADOW/WG_UNMOVABLE
  * @return   成功窗体按钮句柄
*/
struct form *wg_form_create(const char *title,int height,int width,int flags);

/**
  * @brief    窗体放置函数，窗体显示
  * @param    form : 窗体
  * @param    parent : 母窗口/母控件
  * @param    y : 母窗口的相对位置
  * @param    x : 母窗口的相对位置
  * @return   成功返回 0
*/
int wg_form_put(struct form *form,struct nwidget *parent,int y,int x);

/**
  * @brief    创建一个弹出式的窗体
  * @param    height : 窗体高度
  * @param    width : 窗体宽度
  * @param    rely : 窗体坐标
  * @param    relx : 窗体坐标
  * @note     创建的窗体失去焦点时会自动删除
  * @return   成功窗体按钮句柄
*/
struct form *wg_popup_create(int height,int width,int rely,int relx);



int wg_form_hide(struct form *form,int hide);


int wg_form_set(struct form *form,int minimize,int closable);


int wg_msgbox(const char *msg);

#endif /* __CURSES_POPUP_ */
