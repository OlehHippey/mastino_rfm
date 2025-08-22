/*
 * mlist.c
 *
 *  Created on: 4 трав. 2025 р.
 *      Author: voltekel
 */


/*
 * mlist.c
 *
 * Created: 21.04.2021 16:34:15
 *  Author: Aleks
 */ 


#include "global.h"
#include "mlist.h"
#include <string.h>

pthread_mutex_t GL_mutex=PTHREAD_MUTEX_INITIALIZER;
void m_list_mlock(void)
{
	pthread_mutex_lock(&GL_mutex);
}
void m_list_munlock(void)
{
	pthread_mutex_unlock(&GL_mutex);
}
int Mmalloc(void *al,size_t size)
{
	void **tmp = (void **)al;
	if(*tmp)
	{
		fprintf(stderr,"Allocate pointer not null\r\n");
		return 0;
	}
	*tmp = malloc(size);
	if(*tmp == NULL)
	{
		fprintf(stderr,"Cannot allocate memory\r\n");
		return (0);
	}
	memset(*tmp, 0, size);
	return 1;
}
void Mfree(void *al)
{
	void **tmp = (void **)al;
	if(*tmp)
	{
		free(*tmp);
		*tmp=NULL;
	}
}

MList *m_free(MList *ml)
{
	pthread_mutex_lock(&GL_mutex);
	while (ml)
	{
		void *data=ml->data;
		pthread_mutex_unlock(&GL_mutex);
		ml=m_list_remove(ml, data);
		pthread_mutex_lock(&GL_mutex);
		Mfree(&data);
	}
	pthread_mutex_unlock(&GL_mutex);
	return NULL;
}

MList *m_list_first(MList *ml)
{
	if(!ml) return NULL;
	pthread_mutex_lock(&GL_mutex);
	ml->list->cur=ml;
	pthread_mutex_unlock(&GL_mutex);
	return ml->list->first;
}
MList *m_list_next(MList *ml)
{
	if(!ml) return NULL;
	if(!ml->list->cur->next) return NULL;
	pthread_mutex_lock(&GL_mutex);
	ml->list->cur=ml->list->cur->next;
	pthread_mutex_unlock(&GL_mutex);
	return ml->list->cur;
}
MList *m_list_remove(MList *ml, void *data)
{
	MList	*chkd=ml;
	if(!ml) return NULL;
	pthread_mutex_lock(ml->list->lmut);
	for(;chkd!=NULL;chkd=chkd->next)
	{
		if(chkd->data==data)
		{
			pthread_mutex_lock(&GL_mutex);
			if((chkd->list->first==chkd)&&(chkd->list->last==chkd))
			{
				pthread_mutex_unlock(chkd->list->lmut);
				pthread_mutex_destroy(chkd->list->lmut);
				free(chkd->list->lmut);
				free(chkd->list);
				free(chkd);
				pthread_mutex_unlock(&GL_mutex);
				return NULL;
			}
			pthread_mutex_unlock(&GL_mutex);
			if(chkd->next) chkd->next->prev=chkd->prev;
			if(chkd->prev) chkd->prev->next=chkd->next;
			if(chkd->list->first==chkd)
			{
				chkd->list->first=chkd->next;
				ml=chkd->next;
			}
			if(chkd->list->last==chkd) chkd->list->last=chkd->prev;
			chkd->list->count--;
			free(chkd);
			break;
		}
	}
	ml->list->cur=ml;
	pthread_mutex_unlock(ml->list->lmut);
	return ml;
}
/*
void m_list_realloc_arr(MListL *l)
{
	uint32_t lc=((l->count/1000)+1)*1000;
	l->arr=realloc(l->arr,lc*sizeof(void));
	lc--;
	while(lc>l->count)
	{
		void *lcc=l->arr+lc;
		*lcc=l->arr;
		lc--;
	}
}
void m_llist_inc(MListL *l)
{
	if((l->count%1000)==0)
	{
		l->count++;
		m_list_realloc_arr(l);
		return;
	}
	l->count++;
}
*/
MList *m_list_append(MList *ml, void *data)
{
//	printf("mla\n");
	MList *nml=NULL;
	if(!Mmalloc(&nml,sizeof(MList))) while(1){};
	nml->data=data;
	pthread_mutex_lock(&GL_mutex);
	if(!ml)
	{
		MListL *l=NULL;
		Mmalloc(&l,sizeof(MListL));
		l->lmut=malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init((l->lmut), NULL);
		pthread_mutex_lock(l->lmut);
		pthread_mutex_unlock(&GL_mutex);
		ml=nml;
//		l->arr=NULL;
		ml->list=l;
		l->count=1;
		l->first=ml;
		l->last=ml;
//		m_llist_inc(l);
		ml->list->cur=ml;
	}else
	{
		pthread_mutex_lock(ml->list->lmut);
		pthread_mutex_unlock(&GL_mutex);
		MListL *l=ml->list;
		nml->list=l;
		nml->prev=l->last;
		l->last->next=nml;
		l->last=nml;
		l->count++;
		ml->list->cur=nml;
	}
	ml->list->cur=ml->list->last;
	pthread_mutex_unlock(ml->list->lmut);
//	printf("mlae\n");
	return ml;
}


MList *m_list_add_sort(MList *ml, void *data, int (*compFunc)(void*, void*))
{
	if(!ml)
	{
		return m_list_append(ml,data);
	}else
	{
		pthread_mutex_lock(ml->list->lmut);
		MList *mls=ml;
		for (;mls!=NULL;mls=mls->next)
		{
			if(compFunc(data,mls->data)<=0)
			{
				pthread_mutex_unlock(ml->list->lmut);
				return m_list_insert_before(mls, data);
			}
		}
		pthread_mutex_unlock(ml->list->lmut);
		if(!mls)
		{
			return m_list_append(ml,data);
		}
	}
	return ml->list->first;
}

MList *m_list_insert_after(MList *ml, void *data)
{
	MList *nml=NULL;
	if(!Mmalloc(&nml,sizeof(MList))) exit(121);
	nml->data=data;
	if(!ml)
	{
		return m_list_append(ml,data);
	}else
	{
		pthread_mutex_lock(ml->list->lmut);
		MListL *l=ml->list;
		nml->list=l;
		nml->next=ml->next;
		if(nml->next)
		{
			nml->next->prev=nml;
		}else
		{
			l->last=nml;
		}
		ml->next=nml;
		nml->prev=ml;
		l->count++;
		pthread_mutex_unlock(ml->list->lmut);
	}
	ml->list->cur=nml;
	return ml->list->first;
}

MList *m_list_insert_before(MList *ml, void *data)
{
	MList *nml=NULL;
	if(!Mmalloc(&nml,sizeof(MList))) exit(121);
	nml->data=data;
	if(!ml)
	{
		return m_list_append(ml,data);
	}else
	{
		pthread_mutex_lock(ml->list->lmut);
		nml->list=ml->list;
		nml->prev=ml->prev;
		if(nml->prev)
		{
			nml->prev->next=nml;
		}else
		{
			ml->list->first=nml;
		}
		ml->prev=nml;
		nml->next=ml;
		ml->list->count++;
		ml->list->cur=nml;
		pthread_mutex_unlock(ml->list->lmut);
	}
	return ml->list->first;
}

MList *m_list_prepend(MList *ml, void *data)
{
	MList *nml=NULL;
	Mmalloc(&nml,sizeof(MList));
	nml->data=data;
	if(!ml)
	{
		return m_list_append(ml,data);
	}else
	{
		pthread_mutex_lock(ml->list->lmut);
		MListL *l=ml->list;
		nml->list=l;
		nml->next=l->first;
		l->first->prev=nml;
		l->first=nml;
		l->count++;
		ml->list->cur=nml;
		pthread_mutex_unlock(ml->list->lmut);		
	}
	return ml;
}
MList *m_list_cur_set_first(MList *ml)
{
	if(ml)
	{
		ml->list->cur=ml->list->first;
		return ml->list->first;
	}
	return NULL;
}
MList *m_list_cur_set_next(MList *ml)
{
	if(ml)
	{
		ml->list->cur=ml->list->cur->next;
		return ml->list->first;
	}
	return NULL;
}
MList *m_list_get_cur(MList *ml)
{
	if(ml)
	{
		return ml->list->cur;
	}
	return NULL;
}
int m_list_foreach(MList *ml, void *data, int (*forFunc)(void*,void*))
{
	int ret=0;
	if(!ml) return 0;
	pthread_mutex_lock(ml->list->lmut);	
	MList *mlc;
	for (mlc=ml;mlc;mlc=mlc->next) if(forFunc(mlc->data,data)) {ret=1;break;}
	pthread_mutex_unlock(ml->list->lmut);
	return ret;
}

