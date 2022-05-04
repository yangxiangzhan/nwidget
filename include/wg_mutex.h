/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       wg_mutex_t
  ******************************************************************************
  *
  * COPYRIGHT(c) 2022 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#ifndef __CURSES_MUTEX_
#define __CURSES_MUTEX_


void *wg_mutex_create(void);
void wg_mutex_destroy(void *mutex);
void wg_mutex_lock(void *mutex);
void wg_mutex_unlock(void *mutex);


#define NWIDGET_MUTEX_INIT(x)   do{ (x) = wg_mutex_create();}while(0)
#define NWIDGET_MUTEX_LOCK(x)   do{if (x) wg_mutex_lock(x);}while(0)
#define NWIDGET_MUTEX_UNLOCK(x) do{if (x) wg_mutex_unlock(x);}while(0)
#define NWIDGET_MUTEX_DEINIT(x) do{if (x) wg_mutex_destroy(x);(x)=NULL;}while(0)


#endif