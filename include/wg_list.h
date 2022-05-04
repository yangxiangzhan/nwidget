/**
  ******************************************************************************
  * @file        
  * @author      
  * @brief       链表文件，实现相关数据结构及其操作
  ******************************************************************************
  *
  * COPYRIGHT(c) GoodMorning
  *
  ******************************************************************************
  */
#ifndef _WG_LIST_H_
#define _WG_LIST_H_

/* Global  types ------------------------------------------------------------*/
/** 双向链表节点 */
struct wg_list {
	struct wg_list *next, *prev;
};


/* Global  macro ------------------------------------------------------------*/
/* Global  variables --------------------------------------------------------*/
/* Global  function prototypes ----------------------------------------------*/

/* 初始化节点：将list节点的前继节点和后继节点都是指向list本身。 */
static inline void wg_list_init(struct wg_list *list)
{
	list->next = list;
	list->prev = list;
}


/* 添加节点：将_new插入到prev和next之间。 */
static inline void __wg_list_add(struct wg_list *_new,
                  struct wg_list *prev,
                  struct wg_list *next)
{
	next->prev = _new;
	_new->next = next;
	_new->prev = prev;
	prev->next = _new;
}

/* 添加_new节点：将_new添加到head之后，是_new称为head的后继节点。 */
static inline void wg_list_add_head(struct wg_list *_new, struct wg_list *head)
{
	__wg_list_add(_new, head, head->next);
}

/* 添加_new节点：将_new添加到head之前，即将_new添加到双链表的末尾。 */
static inline void wg_list_add_tail(struct wg_list *_new, struct wg_list *head)
{
	__wg_list_add(_new, head->prev, head);
}

/* 从双链表中删除entry节点。 */
static inline void __wg_list_del(struct wg_list * prev, struct wg_list * next)
{
	next->prev = prev;
	prev->next = next;
}

/* 从双链表中删除entry节点。 */
static inline void wg_list_del(struct wg_list *entry)
{
    __wg_list_del(entry->prev, entry->next);
}

/* 从双链表中删除entry节点。 */
static inline void __wg_list_del_entry(struct wg_list *entry)
{
	__wg_list_del(entry->prev, entry->next);
}

/* 从双链表中删除entry节点，并将entry节点的前继节点和后继节点都指向entry本身。 */
static inline void wg_list_del_init(struct wg_list *entry)
{
	__wg_list_del_entry(entry);
	wg_list_init(entry);
}

/* 用_new节点取代old节点 */
static inline void wg_list_replace(struct wg_list *old,
                struct wg_list *_new)
{
	_new->next = old->next;
	_new->next->prev = _new;
	_new->prev = old->prev;
	_new->prev->next = _new;
}

/* 双链表是否为空 */
static inline int wg_list_empty(const struct wg_list *head)
{
	return head->next == head;
}



#endif /* _WG_LIST_H_ */
