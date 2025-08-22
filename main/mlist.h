/*
 * mlist.h
 *
 *  Created on: 4 трав. 2025 р.
 *      Author: voltekel
 */

#ifndef MAIN_MLIST_H_
#define MAIN_MLIST_H_

#include "global.h"



typedef struct MLists
{
	void			*data;
	struct MLists	*next;
	struct MLists	*prev;
	struct MListLs	*list;
}MList;

typedef struct MListLs
{
	int				count;
	struct	MLists	*first;
	struct	MLists	*last;
	struct	MLists	*cur;
//	void	*arr;
	pthread_mutex_t *lmut;
}MListL;




void m_list_mlock(void);
void m_list_munlock(void);
MList *m_free(MList *ml);
MList *m_list_first(MList *ml);
MList *m_list_next(MList *ml);
MList *m_list_append(MList *ml, void *data);
MList *m_list_insert_after(MList *ml, void *data);
MList *m_list_add_sort(MList *ml, void *data, int (*compFunc)(void*, void*));
MList *m_list_insert_before(MList *ml, void *data);
MList *m_list_remove(MList *ml, void *data);
MList *m_list_cur_set_first(MList *ml);
MList *m_list_cur_set_next(MList *ml);
MList *m_list_get_cur(MList *ml);
int m_list_foreach(MList *ml, void *data, int (*forFunc)(void*,void*));



#endif /* MAIN_MLIST_H_ */
