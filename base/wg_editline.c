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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wg_mutex.h"
#include "stringw.h"
#include "wg_editline.h"

/* Private macro ------------------------------------------------------------*/
/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Global  variables --------------------------------------------------------*/

/* Private function prototypes ----------------------------------------------*/
static int editline_destroy(struct nwidget *wg_entry);
static int editline_preclose(struct nwidget *wg_entry);
static wg_state_t editline_curs_right(struct nwidget *wg_entry,long key);
static wg_state_t editline_curs_left(struct nwidget *wg_entry,long key);
static wg_state_t editline_curs_end(struct nwidget *wg_entry,long key);
static wg_state_t editline_curs_home(struct nwidget *wg_entry,long key);
static wg_state_t editline_backspace(struct nwidget *wg_entry,long key);
static wg_state_t editline_selected(struct nwidget *wg_entry,long key);
/* Gorgeous Split-line ------------------------------------------------------*/

const struct wghandler editline_default_handlers[] = {
	{KEY_RIGHT    ,editline_curs_right},
	{KEY_LEFT     ,editline_curs_left},
	{KEY_HOME     ,editline_curs_home},
	{KEY_END      ,editline_curs_end },
	{KEY_DC       ,editline_backspace},
	{0x08         ,editline_backspace},
	{0x7f         ,editline_backspace},
	{KEY_BACKSPACE,editline_backspace},
	{KEY_DOWN     ,widget_exit_right},
	{KEY_UP       ,widget_exit_left},
	{'\n'         ,editline_selected},
	{0,0},
};


/*
	  |  Unicode符号范围      |  UTF-8编码方式  
	n |  (十六进制)            | (二进制)  
	--+-----------------------+------------------------------------------------------  
	1 | 0000 0000 - 0000 007F |                                              0xxxxxxx  
	2 | 0000 0080 - 0000 07FF |                                     110xxxxx 10xxxxxx  
	3 | 0000 0800 - 0000 FFFF |                            1110xxxx 10xxxxxx 10xxxxxx  
	4 | 0001 0000 - 0010 FFFF |                   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  
	5 | 0020 0000 - 03FF FFFF |          111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
	6 | 0400 0000 - 7FFF FFFF | 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
*/
int unicode_by_utf8(long key,unsigned char utf8[8])
{
	int mask;
	int len = 0;
	if (key < 241) {
		utf8[0] = key;
		utf8[1] = '\0';
		return 1;
	}

	if (key > 0x0000FFFF) {
		if (key > 0x03FFFFFF) {
			len = 6;
		} else if (key > 0x0010FFFF) {
			len = 5;
		} else {
			len = 4;
		}
	} else {
		if (key > 0x000007FF) {
			len = 3;
		} else {
			len = 2;
		}
	}
	utf8[len] = 0;
	for (int tmp = len ; tmp > 0 ; ) {
		tmp--;
		utf8[tmp] = (0x3f & key) | 0x80;
		key >>= 6;
	}
	mask = (1 << (8 - len)) - 1;
	utf8[0] |= (~mask);

	return len;
}




/**
  * @brief    输入控件响应  backspace 删除字符
  * @param    wg_entry   : 输入控件所在对象句柄
  * @return   成功返回 0
*/
static wg_state_t editline_backspace(struct nwidget *wg_entry,long key)
{
	int tail,len,edit,cursor,start;
	char *data;
	struct editline *line ;
	line = container_of(wg_entry,struct editline,wg);
	if (!line->edit) {
		beep();
		return WG_OK;
	}

	NWIDGET_MUTEX_LOCK(line->mutex);
	data = line->value;
	edit = line->edit-1;
	while((data[edit] & 0xC0) == 0x80 ) {
		edit--;
	}

	memcpy(&data[edit],&data[line->edit],line->tail - line->edit);
	len = line->edit - edit;
	tail = line->tail - len;
	start = line->start_display_at;
	data[tail] = '\0';

	cursor = wstrnlen(&data[start],edit-start) + line->labelen;
	if (start && cursor < line->wg.width - 1) {
		len = 1 ;
		while(start > len && (data[start - len] & 0xC0) == 0x80 ) {
			len++;
		}
		start -= len;
		cursor = wstrnlen(&data[start],edit-start) + line->labelen;
	}

	if (edit < start) {
		start = edit;
	}

	desktop_lock();
	curs_set(0);
	wattron(line->wg.win,A_UNDERLINE);
	mvwhline(line->wg.win,0,line->labelen,' ',line->available);
	mvwaddstr(line->wg.win,0,line->labelen,&data[start]);
	wattroff(line->wg.win,A_UNDERLINE);
	CURSOR_MOVE(&line->wg,0,cursor);
	curs_set(2);
	desktop_refresh();
	desktop_unlock();

	line->start_display_at = start;
	line->cursor = cursor;
	line->edit = edit;
	line->tail = tail;
	NWIDGET_MUTEX_UNLOCK(line->mutex);
	if (line->sig.changed)
		line->sig.changed(line,line->sig.changed_arg);

	return WG_OK;
}


/**
  * @brief    输入控件响应  回车键 确认
  * @param    wg_entry   : 输入控件所在对象句柄
  * @return   成功返回 0
*/
static wg_state_t editline_selected(struct nwidget *wg_entry,long key)
{
	int res = WG_EXIT_NEXT;
	struct editline *line ;
	line = container_of(wg_entry,struct editline,wg);
	if (line->sig.selected)
		res = line->sig.selected(line,line->sig.selected_arg);
	return res;
}

/**
  * @brief    输入控件响应  KEY_LEFT 向左移动光标
  * @param    wg_entry   : 输入控件所在对象句柄
  * @return   成功返回 0
*/
static wg_state_t editline_curs_left(struct nwidget *wg_entry,long key)
{
	int edit,cursor,start;
	char *data;
	struct editline *line ;
	line = container_of(wg_entry,struct editline,wg);
	if (!line->edit) {
		beep();
		return WG_OK;
	}

	start = line->start_display_at;
	data = line->value;
	edit = line->edit-1;
	while((data[edit] & 0xC0) == 0x80 ) {
		edit--;
	}

	desktop_lock();
	curs_set(0);
	if (edit < start) {
		start = edit;
		wattron(line->wg.win,A_UNDERLINE);
		mvwhline(line->wg.win,0,line->labelen,' ',line->available);
		mvwaddstr(line->wg.win,0,line->labelen,&data[start]);
		wattroff(line->wg.win,A_UNDERLINE);
	}

	cursor = wstrnlen(&data[start],edit-start) + line->labelen;
	CURSOR_MOVE(&line->wg,0,cursor);
	curs_set(2);
	desktop_refresh();
	desktop_unlock();

	line->start_display_at = start;
	line->cursor = cursor;
	line->edit = edit;
	return WG_OK;
}


/**
  * @brief    输入控件响应  KEY_RIGHT 向右移动光标
  * @param    wg_entry   : 输入控件所在对象句柄
  * @return   成功返回 0
*/
static wg_state_t editline_curs_right(struct nwidget *wg_entry,long key)
{
	int edit,cursor,start;
	char *data;
	struct editline *line ;
	line = container_of(wg_entry,struct editline,wg);
	if (line->tail == line->edit) {
		beep();
		return WG_OK;
	}

	start = line->start_display_at;
	data = line->value;
	edit = line->edit ;
	while((data[++edit] & 0xC0) == 0x80);

	desktop_lock();
	curs_set(0);
	if (wstrnlen(&data[start],edit-start) >= line->available) {
		do {
			while((data[++start] & 0xC0) == 0x80);
		} while(wstrnlen(&data[start],edit-start) >= line->available);
		wattron(line->wg.win,A_UNDERLINE);
		mvwhline(line->wg.win,0,line->labelen,' ',line->available);
		mvwaddstr(line->wg.win,0,line->labelen,&data[start]);
		wattroff(line->wg.win,A_UNDERLINE);
	}

	cursor = wstrnlen(&data[start],edit-start) + line->labelen;
	CURSOR_MOVE(&line->wg,0,cursor);
	curs_set(2);
	desktop_refresh();
	desktop_unlock();

	line->edit = edit;
	line->start_display_at = start;
	line->cursor = cursor;
	return WG_OK;
}

static wg_state_t editline_curs_end(struct nwidget *wg_entry,long key)
{
	struct editline *line = container_of(wg_entry,struct editline,wg) ;
	if (line->edit == line->tail) {
		beep();
	} else {
		wg_editline_edit_end(line);
	}
	return WG_OK;
}

static wg_state_t editline_curs_home(struct nwidget *wg_entry,long key)
{
	struct editline *line = container_of(wg_entry,struct editline,wg) ;
	if (!line->edit) {
		beep();
	} else {
		wg_editline_edit_home(line);
	}
	return WG_OK;
}

/**
  * @brief    输入控件插入字符
  * @param    line : 输入控件
  * @param    text : 插入文本
  * @param    len : 插入文本长度
  * @return   成功返回 0
*/
static int editline_insert(struct editline *line,const char *text,int len)
{
	char *dest;
	int start,edit,tail,mv,cursor;
	assert(line && text && len);

	dest = line->value;
	edit = line->edit;
	tail = line->tail;
	start = line->start_display_at;
	cursor = line->cursor;

	if (len + tail > sizeof(line->value) - 1) {
		len = sizeof(line->value) - 1 - tail;
		if ((text[len-1] & 0xC0) == 0xC0)
			len--;
		else
			for ( ; (text[len-1] & 0xC0) == 0x80;len--);
	}

	if ( (mv = tail - edit) )
		memmove(&dest[edit+len],&dest[edit],mv);
	memcpy(&dest[edit],text,len);
	edit += len;
	tail += len;
	dest[tail] = '\0';

	cursor += wstrnlen(text,len);
	while (cursor >= line->wg.width) {
		/* 此处假设字符长度 utf8 > 3 (如汉字)时占用两个显示空间 */
		len = utf8len(dest[start]);
		start += len;
		cursor -= (len > 2) ? 2 : 1;
	}

	line->edit = edit;
	line->tail = tail;
	line->cursor = cursor;
	line->start_display_at = start;
	return 0;
}

/**
  * @brief    输入控件接收字符输入处理函数
  * @param    wg_entry : 输入控件所在对象句柄
  * @return   成功返回 0
*/
static wg_state_t editline_getchar(struct nwidget *wg_entry,long key)
{
	int len;
	char utf8[8];
	struct editline *line ;

	if (key < ' ') {
		beep();
		return WG_OK;
	}

	line = container_of(wg_entry,struct editline,wg);
	len = unicode_by_utf8(key,(unsigned char*)utf8);
	if (wg_editline_append(line,utf8,len) < 0) {
		beep();
	} else if (line->sig.changed){
		line->sig.changed(line,line->sig.changed_arg);
	}
	return WG_OK;
}

/**
  * @brief    输入控件聚焦回调
  * @param    wg : 输入控件所在对象句柄
  * @return   成功返回 0
*/
static int editline_onfocus(struct nwidget *wg)
{
	int color;
	struct editline *line = container_of(wg,struct editline,wg);
	
	if (line->labelen && wg->bkg) {
		if (wg->bkg == wgtheme.desktop_bkg)
			color =  wgtheme.desktop_fg;
		else 
			color = wgtheme.form_fg;
		
		desktop_lock();
		wattron(wg->win,color);
		mvwaddstr(wg->win,0,0,line->label);
		wattroff(wg->win,color);
		desktop_refresh();
		desktop_unlock();
	}
	CURSOR_MOVE(wg,0,line->cursor);
	curs_set(2);
	return 0;
}


/**
  * @brief    输入控件失焦回调
  * @param    wg_entry : 输入控件所在对象句柄
  * @return   成功返回 0
*/
static int editline_blur(struct nwidget *wg_entry)
{
	struct editline *line = container_of(wg_entry,struct editline,wg);
	desktop_lock();
	curs_set(0);
	//wmove(line->wg.win,0,0);
	mvwaddstr(wg_entry->win,0,0,line->label);
	desktop_unlock();
	return 0;
}


/**
  * @brief    输入控件放置函数，显示
  * @param    line : 输入控件
  * @param    parent : 父窗体
  * @param    y : 父窗体坐标
  * @param    x : 父窗体坐标
  * @return   成功返回 0
*/
int wg_editline_put(struct editline *line,struct nwidget *parent,int y,int x)
{
	static const char tips[] = "editline:TAB:<switch>";

	/* widget_init 会清空 wg */
	int width = line->wg.width;

	if (widget_init(&line->wg,parent,1,width,y,x)) {
		return -1;
	}

	handlers_update(&line->wg,editline_default_handlers);
	line->wg.edit = editline_getchar;
	line->wg.focus = editline_onfocus;
	line->wg.blur = editline_blur;
	line->wg.preclose = editline_preclose;
	line->wg.destroy = editline_destroy;
	line->wg.tips = tips;

	/* display */
	desktop_lock();
	if (line->wg.bkg)
		wbkgd(line->wg.win,line->wg.bkg);
	waddstr(line->wg.win,line->label);
	wattron(line->wg.win,A_UNDERLINE);
	whline(line->wg.win,' ',line->available);
	if (line->value[0]) {
		char *text = &line->value[line->start_display_at];
		mvwaddstr(line->wg.win,0,line->labelen,text);
	}
	wattroff(line->wg.win,A_UNDERLINE);
	desktop_unlock();
	return 0;
}


/**
  * @brief    输入控件初始化
  * @param    line : 输入控件
  * @param    label : 输入标签
  * @param    width : 控件宽度
  * @return   成功返回 0
*/
static int wg_editline_init(struct editline *line,const char *label,int width)
{
	int available,label_len = 0;

	memset(line,0,sizeof(struct editline));
	if (label){
		/* label 最大显示宽度为 24 ，即最多显示 12 个汉字 */
		if (width > EDITLINE_LABAL_MAX)
			label_len = EDITLINE_LABAL_MAX ;
		else
			label_len = width - 2 ;
		wstrncpy(line->label,label,label_len);
		label_len = wstrlen(line->label);
	}

	NWIDGET_MUTEX_INIT(line->mutex);
	available = width - label_len;
	line->available = available;
	line->labelen = label_len;
	line->cursor = label_len;
	line->wg.width = width;
	return 0;
}


/**
  * @brief    创建输入控件
  * @param    label : 输入标签
  * @param    width : 控件宽度
  * @return   输入控件
*/
struct editline *wg_editline_create(const char *label,int width)
{
	struct editline *line;
	assert(width > 2);
	line = malloc(sizeof(struct editline));
	if (NULL == line) {
		return NULL;
	}

	if (wg_editline_init(line,label,width)) {
		free(line);
		return NULL;
	}
	line->created_by = wg_editline_create;
	DEBUG_MSG("%s(%p)",__FUNCTION__,line);
	return line;
}


static int editline_preclose(struct nwidget *self)
{
	struct editline *line = container_of(self,struct editline,wg);
	if (line->sig.closed)
		line->sig.closed(line,line->sig.changed_arg);
	return 0;
}



static int editline_destroy(struct nwidget *self)
{
	struct editline *line = container_of(self,struct editline,wg);
	NWIDGET_MUTEX_DEINIT(line->mutex);
	if (line->created_by == wg_editline_create){
		DEBUG_MSG("%s(%p)",__FUNCTION__,line);
		free(container_of(self,struct editline,wg));
	}
	return 0;
}


/**
  * @brief    输入控件在当前编辑位置追加字符
  * @param    line     : 输入控件
  * @param    text     : 值，字符串
  * @param    len      : 值长度
  * @return   成功返回0
*/
static int wg_editline_append_raw(struct editline *line,const char *text,int len)
{
	int start,cursor,edit; 
	if (len < 0)
		len = strlen(text);

	if (!len)
		return 0;

	if (len + line->tail > MAX_LINE_LENGTH)
		return -1;

	edit = line->edit;
	cursor = line->cursor;
	start = line->start_display_at;
	editline_insert(line,text,len);
	if (line->wg.win) {
		desktop_lock();
		if (line->wg.editing) {
			curs_set(0);
		}
		wattron(line->wg.win,A_UNDERLINE);
		if (start == line->start_display_at) {
			mvwaddstr(line->wg.win,0,cursor,&line->value[edit]);
		} else {
			start = line->start_display_at;
			mvwhline(line->wg.win,0,line->labelen,' ',line->available);
			mvwaddstr(line->wg.win,0,line->labelen,&line->value[start]);
		}
		wattroff(line->wg.win,A_UNDERLINE);
		if (line->wg.editing) {
			CURSOR_MOVE(&line->wg,0,line->cursor);
			curs_set(2);
		}
		desktop_refresh();
		desktop_unlock();
	}

	return 0;
}



/**
  * @brief    输入控件在当前编辑位置追加字符
  * @param    line     : 输入控件
  * @param    text     : 值，字符串
  * @param    len      : 值长度
  * @return   成功返回0
*/
int wg_editline_append(struct editline *line,const char *text,int len)
{
	int res;
	NWIDGET_MUTEX_LOCK(line->mutex);
	res = wg_editline_append_raw(line,text,len);
	NWIDGET_MUTEX_UNLOCK(line->mutex);
	return res;
}

/**
  * @brief    控件编辑点移动至末尾
  * @param    line     : 输入控件
  * @return   成功返回0
*/
int wg_editline_edit_home(struct editline *line)
{
	int cursor = line->labelen;
	if (!line->edit) {
		return 0;
	}

	if (line->start_display_at) {
		desktop_lock();
		curs_set(0);
		wattron(line->wg.win,A_UNDERLINE);
		mvwhline(line->wg.win,0,line->labelen,' ',line->available);
		mvwaddstr(line->wg.win,0,line->labelen,line->value);
		wattroff(line->wg.win,A_UNDERLINE);
		desktop_refresh();
		desktop_unlock();
	}
	CURSOR_MOVE(&line->wg,0,cursor);
	curs_set(2);

	line->edit = line->start_display_at = 0;
	line->cursor = cursor;
	return 0;
}

/**
  * @brief    控件编辑点移动至末尾
  * @param    line     : 输入控件
  * @return   成功返回0
*/
int wg_editline_edit_end(struct editline *line)
{
	int oldpos,cursor,start,len;
	const char *data;
	if (line->edit == line->tail) {
		return 0;
	}

	oldpos = line->start_display_at;
	data = line->value;
	start = 0;
	cursor = wstrlen(data);
	while(cursor > line->available -1) {
		len = utf8len(data[start]);
		start += len;
		cursor -= (len > 2 ? 2 : 1);
	}

	cursor += line->labelen;
	if (oldpos != start) {
		desktop_lock();
		curs_set(0);
		wattron(line->wg.win,A_UNDERLINE);
		mvwhline(line->wg.win,0,line->labelen,' ',line->available);
		mvwaddstr(line->wg.win,0,line->labelen,&data[start]);
		wattroff(line->wg.win,A_UNDERLINE);
		desktop_refresh();
		desktop_unlock();
	}
	CURSOR_MOVE(&line->wg,0,cursor);
	curs_set(2);

	line->edit = line->tail;
	line->start_display_at = start;
	line->cursor = cursor;
	return 0;
}




/**
  * @brief    设置输入控件值
  * @param    line     : 输入控件
  * @param    text     : 值，字符串
  * @return   成功返回0
*/
int wg_editline_set_text(struct editline *line,const char *text)
{
	assert(line && text);
	NWIDGET_MUTEX_LOCK(line->mutex);
	if (line->wg.win) {
		wattron(line->wg.win,A_UNDERLINE);
		mvwhline(line->wg.win,0,line->labelen,' ',line->available);
		wattroff(line->wg.win,A_UNDERLINE);
	}
	line->tail = 0;
	line->edit = 0;
	line->start_display_at = 0;
	line->cursor = line->labelen;
	wg_editline_append_raw(line,text,-1);
	NWIDGET_MUTEX_UNLOCK(line->mutex);
	return 0;
}

