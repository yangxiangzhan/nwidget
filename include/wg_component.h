/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       控件集合
  ******************************************************************************
  *
  * COPYRIGHT(c) 2021 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#ifndef __CURSES_COMPONENT_
#define __CURSES_COMPONENT_

#include "wg_button.h"
#include "wg_form.h"
#include "wg_editline.h"
#include "wg_scrollbar.h"
#include "wg_table.h"
#include "wg_textarea.h"

#include "wg_dropdown.h"
#include "wg_checkbox.h"



/**
  * @brief    状态栏初始化
  * @return   成功返回 0 ，否则返回其他 
*/
int desktop_taskbar_init(void);

#endif