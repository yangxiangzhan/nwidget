/**
  ******************************************************************************
  * @file        
  * @author      GoodMorning
  * @brief       基本控件对象及其管理
  ******************************************************************************
  *
  * COPYRIGHT(c) 2021 GoodMorning
  *
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#ifndef __CURSES_WIDGET_
#define __CURSES_WIDGET_

/* Includes -----------------------------------------------------------------*/

#include <stdio.h>  // for sprintf
#include <stddef.h> /* for offsetof  获取"MEMBER成员"在"结构体TYPE"中的位置偏移 */
#include <curses.h>
#include <panel.h>

/* Global macro -------------------------------------------------------------*/
#ifndef container_of
	#ifndef __GNUC__
		/* 根据"结构体(type)变量"中的"域成员变量(member)的指针(ptr)"来获取指向整个结构体变量的指针 */
		#define container_of(ptr, type, member)  ((type*)((char*)ptr - offsetof(type, member)))
		/* 此宏定义原文为 GNU C 所写，如下，有些编译器只支持 ANSI C /C99 的，所以作以上修改 */
	#else
		#define container_of(ptr, type, member) ({          \
			const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
			(type *)( (char *)__mptr - offsetof(type,member) );})
	#endif
#endif

#if 0
	/** ctrl + 方向键，这几个键值不在 ncurses 的列表上，未必是标准的 */
	#define KEY_CTRL_UP    (567)
	#define KEY_CTRL_DOWN  (526)
	#define KEY_CTRL_LEFT  (546)
	#define KEY_CTRL_RIGHT (561)
#else
	/** shift + 方向键*/
	#define KEY_CTRL_UP    KEY_SR
	#define KEY_CTRL_DOWN  KEY_SF
	#define KEY_CTRL_LEFT  KEY_SLEFT
	#define KEY_CTRL_RIGHT KEY_SRIGHT
#endif

#define __DEBUG_MSG(y,...)\
do{\
	char msg[256];\
	snprintf(msg,256-1,__VA_ARGS__);\
	mvwhline(stdscr,y,2,' ',COLS-4);\
	mvaddstr(y,2,msg);\
	\
}while(0)


#ifdef UNIT_DEBUG
	extern int debug_msg(const char *msg);
	#define DEBUG_MSG(...)\
	do{\
		char msg[256] ={0};\
		snprintf(msg,256-1,__VA_ARGS__);\
		debug_msg(msg);\
	}while(0)

	#define DEBUG_LOG(y,x,...)\
	do{\
		char msg[256] ={0};\
		snprintf(msg,256-1,__VA_ARGS__);\
		mvaddstr(y,x,msg);\
	}while(0)
#else
	#define DEBUG_MSG(...)
	#define DEBUG_LOG(...)
#endif

#ifdef __PDCURSES__
	/** 好像 pdcurses 无法通过 wmove 移动物理光标，被迫用 move */
	#define CURSOR_MOVE(_widget,_y,_x) move((_widget)->rely + (_y) ,(_widget)->relx + (_x))
#else
	#define CURSOR_MOVE(_widget,_y,_x) wmove((_widget)->win,_y,_x);
#endif


/* 控件最大快捷键数目 */
#define MAX_HANDLERS  (32)

/** 桌面动态刷新间隔，单位 ms */
#define REFRESH_DELAY_MS 150

/** 标识值 */
#define HAS_BEEN_PLACED (0x20210610)

/** 摆放控件 */
#define wg_put_on(_wg,_form,_y,_x) (((_wg)->put_on) ? (_wg)->put_on(_wg,_form,_y,_x) : -1)

/** 窗口显示边框 */
#define wg_show_border(win) \
wborder(win,wgtheme.ls,wgtheme.rs,wgtheme.ts,wgtheme.bs,wgtheme.tl,wgtheme.tr,wgtheme.bl,wgtheme.br)


/** 连接两个宏定义 */
#define _MARCOCAT(x,y) x##y
#define MARCOCAT(x,y)  _MARCOCAT(x,y)

/**
  * @brief    控件连接一个信号
  * @param    _wg : 指定控件，如 button
  * @param    _signal : 信号,clicked/selected/changed/closed
  * @param    _callback : 回调函数
  * @param    _callback_arg : 回调函数参数
*/
#define wg_signal_connect(_wg,_signal,_callback,_callback_arg) \
do{ \
	(_wg)->sig._signal = _callback;\
	(_wg)->sig.MARCOCAT(_signal,_arg) = _callback_arg;\
}while(0)


/** 颜色枚举定义 */
enum wg_color{
	WG_BLACK = 1,
	WG_RED,
	WG_GREEN,
	WG_YELLOW,
	WG_BLUE,
	WG_MAGENTA,
	WG_CYAN,
	WG_WHITE
};

/* Global type  -------------------------------------------------------------*/

/** 控件编辑后状态集 */
typedef enum widget_state {
	WG_EXIT_PREV  = -1, /**< 当前控件退出选中状态，并选中当前控件的上一个控件 */
	WG_OK = 0,
	WG_EXIT_NEXT  = 1, /**< 当前控件退出选中状态，并选中当前控件的下一个控件 */
	WG_EXIT_MOUSE = 130, /**< 当前控件退出选中状态，并选中鼠标点击的控件 */
} wg_state_t ;


/** 基础控件 */
typedef struct nwidget {
	struct nwidget *parent;/**< 当前控件的父节点 */
	struct nwidget *sub;/**< 当前控件的子节点 */
	struct nwidget *next;/**< 当前控件的下一个兄弟节点 */
	struct nwidget *prev;/**< 当前控件的上一个兄弟节点 */
	
	WINDOW *win; /**< 当前控件的基础窗口 */
	PANEL *panel; /**< 由 win 生成的基础面板 */

	int relx,rely; /** 绝对值坐标 */
	int width,height; /** 控件大小尺寸 */

	int bkg;/**< 背景色，基础色调 */
	int focus_time; /**< 最近一次被聚焦的时间 */
	int has_been_placed; /**< == HAS_BEEN_PLACED where called widget_init */

	int editing;/**< 当前控件被选中，焦点在当前控件上 */
	int redraw_requested; /**< 重绘制请求 */

	int always_top; /**< for popup form which always on top */
	int delete_onblur;/**< 失去焦点后自动销毁 */
	int found_by_tab;
	int hidden;/**< 当前控件被隐藏，一般在 this->hide() 中被设置 */

	const char *tips;/**< 被聚焦时的提示性文字 */

	/* 控件关闭前执行函数，用于销毁前通知用户，默认为 NULL,
	   called by do_delete() */
	int (*preclose)(struct nwidget *self);

	/** 销毁控件函数，释放具体控件所申请的其他资源,每个控件应作重载，
	   called by do_delete() */
	int (*destroy)(struct nwidget *self);

	/** 当控件失焦时调用的函数，默认为 NULL */
	int (*blur)(struct nwidget *self);

	/** 当控件聚焦时调用的函数，默认为 NULL */
	int (*focus)(struct nwidget *self);

	/** 当窗体移动时控件进行相对移动的动作函数。如果一个控件存在多个子面板，
	    需重载这个函数,默认为 widget_move */
	int (*move)(struct nwidget *self,int y_offset,int x_offset);

	/* 隐藏/显示动作函数。如果一个控件存在多个子面板，
	    需重载这个函数，默认为 widget_hide */
	int (*hide)(struct nwidget *self,int hide);

	/* 请求在桌面刷新之后重绘制控件 */
	int (*redraw)(struct nwidget *self);

	/** 当前控件所支持的快捷键 */
	long shortcuts[MAX_HANDLERS];

	/** 当前控件所支持的快捷键处理函数。函数的返回值 @see wg_state_t */
	wg_state_t (*handlers[MAX_HANDLERS])(struct nwidget *self,long shortcut);

	/** 如果控件对鼠标事件感兴趣，重载这个函数。默认为 NULL。函数的返回值 @see wg_state_t */
	wg_state_t (*handle_mouse_event)(struct nwidget *self);

	/** 控件接收除 shortcut 以外的字符处理，默认为 NULL。函数的返回值 @see wg_state_t */
	wg_state_t (*edit)(struct nwidget *self,long key);
} widget_t;


/** 基础控件状态事件 */
typedef struct nwidget_signal {
	/** 被选择后执行的回调函数 */
	int (*selected)(void *self,void *selected_arg);
	void *selected_arg;

	/** 被点击后执行的回调函数 */
	int (*clicked)(void *self,void *clicked_arg);
	void *clicked_arg;

	/** 选项改变后执行的回调函数 */
	int (*changed)(void *self,void *changed_arg);
	void *changed_arg;

	#if 0
	/** 被聚焦时的回调函数 */
	int (*focus)(void *self,void *focus_arg);
	void *focus_arg;

	/** 失焦时的回调函数 */
	int (*blur)(void *self,void *blur_arg);
	void *blur_arg;
	#endif

	/** 模块被销毁后的执行函数 */
	int (*closed)(void *self,void *closed_arg);
	void *closed_arg;
}wgsig_t;


/** 快捷键定义 */
struct wghandler {
	long shortcut ;
	wg_state_t (*func)(struct nwidget *self,long shortcut);
};

/** 被选中时所显示的颜色主题 */
struct theme {
	enum wg_color title; /**< 标题颜色，默认为 WG_RED */
	enum wg_color desktop_bkg; /**< 桌面背景颜色，默认为 WG_BLUE */
	enum wg_color desktop_fg; /**< 桌面前景色(即字体色)，默认为 WG_WHITE */
	enum wg_color form_bkg; /**< 窗体背景颜色，默认为 WG_WHITE */
	enum wg_color form_fg; /**< 窗体前景色(即字体色)，默认为 WG_BLACK */
	enum wg_color shadow; /**< 阴影颜色，默认为 WG_BLACK */


	/* 重定义颜色的RGB */
	#define WG_RGB(r,g,b) (((r)<<16)|((g)<<8)|((b)<<0))
	size_t black;
	size_t red;
	size_t green;
	size_t yellow;
	size_t blue;
	size_t magenta;
	size_t cyan;
	size_t white;

	/** 边框元素,note:默认状态下 putty 显示不出这部分符号，可进行重定义 */
	chtype ls; /**< 左边框，为 0 时默认为 <curses.h>:ACS_VLINE */
	chtype rs; /**< 右边框，为 0 时默认为 <curses.h>:ACS_VLINE */
	chtype ts; /**< 上边框，为 0 时默认为 <curses.h>:ACS_HLINE */
	chtype bs; /**< 下边框，为 0 时默认为 <curses.h>:ACS_HLINE */
	chtype tl; /**< 左上角，为 0 时默认为 <curses.h>:ACS_ULCORNER */
	chtype tr; /**< 右上角，为 0 时默认为 <curses.h>:ACS_URCORNER */
	chtype bl; /**< 左下角，为 0 时默认为 <curses.h>:ACS_LLCORNER */
	chtype br; /**< 右下角，为 0 时默认为 <curses.h>:ACS_LRCORNER */

	char btn_normal[2]; /**< 按键常态样式，默认为 "[]"  */
	char btn_clicked[2]; /**< 按键点击时的样式，默认为 "><"  */
	char btn_focus[2]; /**< 按键被聚焦时的样式，默认为 "  "  */
	char chkbox_enabled[4]; /**< 复选框未选中状态，默认为 "( )" */
	char chkbox_disabled[4]; /**< 复选框未选中状态，默认为 "(*)" */
	char radio_enabled[4]; /**< 复选框未选中状态，默认为 "[*]" */
	char radio_disabled[4]; /**< 复选框未选中状态，默认为 "[*]" */
};

/* Global variables  --------------------------------------------------------*/
extern MEVENT mouse;
extern struct nwidget desktop;
extern struct theme wgtheme;

/* Global function prototypes -----------------------------------------------*/

/**
  * @brief    控件初始化
  * @param    widget : 控件
  * @param    form   : 控件所在的窗体
  * @param    rows   : 控件长度
  * @param    cols   : 控件宽度
  * @param    y      : Y 轴位置
  * @param    x      : X 轴位置
  * @return   成功返回 0
*/
int widget_init(struct nwidget *widget,struct nwidget *parent,int rows,int cols,int y,int x);


/**
  * @brief    从桌面删除一个控件及其子控件
  * @param    widget : 控件
  * @return   成功返回 0
*/
int widget_delete(struct nwidget *widget);
#define desktop_delete widget_delete


/**
  * @brief    更新对象的按键响应
  * @param    object   : 控件对象
  * @param    handlers : 键值-处理函数结构体数组，必须以 {0,0} 结尾
  * @return   0 
*/
int handlers_update(struct nwidget *object,const struct wghandler *handlers);


/**
  * @brief    当前控件退出编辑，焦点移动至下一个控件
  * @param    object  : 控件对象
  * @param    key     : 按键键值
  * @return   WG_EXIT_NEXT 
*/
static inline int widget_exit_right(struct nwidget *object,long key)
{
	return WG_EXIT_NEXT;
}

/**
  * @brief    当前控件退出编辑，焦点移动至上一个控件
  * @param    object  : 控件对象
  * @param    key     : 按键键值
  * @return   WG_EXIT_PREV 
*/
static inline int widget_exit_left(struct nwidget *object,long key)
{
	return WG_EXIT_PREV;
}


/**
  * @brief    请求刷新桌面
  * @note     实际上并未立即刷新桌面，一般是定时刷新或处理完键盘输入后刷新
*/
static inline void desktop_refresh(void)
{
	desktop.editing++;
}


/**
  * @brief 请求桌面在刷新后重绘某个控件
  * @param wg : 指定控件
  * @note  实际上并未立即刷新桌面，一般是定时刷新或处理完键盘输入后刷新
*/
static inline void desktop_redraw(struct nwidget *wg)
{
	wg->redraw_requested++;
	desktop.redraw_requested++;
	desktop.editing++;
}


/**
  * @brief    移动焦点至指定控件，控件至于顶层并执行聚焦回调
  * @param    new_focus : 目标对象
  * @return   0
*/
int desktop_focus_on(struct nwidget *new_focus);


/**
  * @brief    初始化 curses 相关组件
  * @param    user : 用户主题定义，为 NULL 即采用默认;@see truct theme
  * @return   0 
*/
int desktop_init(struct theme *user);


// 上锁整个桌面，原则上任何修改屏幕内容前都应对整个屏幕上锁
void desktop_lock(void);

// 解锁桌面
void desktop_unlock(void);

/**
  * @brief    接收输入，交由 curses 托管
  * @return   0 
*/
int desktop_editing(void);

/**
  * @brief    update desktop tips
  * @param    tips : message
  * @return   0
*/
int desktop_tips(const char *tips);

#endif /* __CURSES_WIDGET_ */
