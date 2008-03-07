/*
** dlist.h
**
** Copyright (C) 1996-2008 Robert William Fuller <hydrologiccycle@gmail.com>
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DLIST_H_INCLUDED
#define DLIST_H_INCLUDED

/*  define INLINE appropriately
*/
#ifndef INLINE
#if defined(__cplusplus) || defined (__GNUC__)
#define INLINE inline
#elif defined(_MSC_VER)
#define INLINE __inline
#else
#define INLINE
#endif
#endif


#define DLIST_DECLARE(name) \
		d_list_node_t name = { &name, &name };

#define	FIELD_TO_STRUCT(p, f, t) \
		((t *) ((char *) (p) -  (char *) &((t *) 0)->f))


typedef struct _d_list_node_t {

	struct _d_list_node_t *next, *prev;

} d_list_node_t, d_list_head_t;


static INLINE
void dListInitHead(d_list_head_t *listHead)
{
	listHead->next = listHead->prev = listHead;
}

static INLINE
int dListIsEmpty(d_list_head_t *listHead)
{
	return (listHead->next == listHead);
}

static INLINE
void dListInsertHead(d_list_head_t *listHead, d_list_node_t *listNode)
{
	d_list_node_t *next = listHead->next;

	listNode->next = next;
	listHead->next = listNode;
	next->prev = listNode;
	listNode->prev = listHead;	
}

static INLINE
void dListInsertTail(d_list_head_t *listHead, d_list_node_t *listNode)
{
	d_list_node_t *prev = listHead->prev;

	listNode->prev = prev;
	listHead->prev = listNode;
	prev->next = listNode;
	listNode->next = listHead;
}

static INLINE
void dListRemoveNode(d_list_node_t *listNode)
{
	d_list_node_t *next = listNode->next;
	d_list_node_t *prev = listNode->prev;

	next->prev = prev;
	prev->next = next;
}

static INLINE
d_list_node_t *dListRemoveHead(d_list_head_t *listHead)
{
	d_list_node_t *victim = listHead->next;

	dListRemoveNode(victim);

	return victim;
}

static INLINE
d_list_node_t *dListRemoveTail(d_list_head_t *listHead)
{
	d_list_node_t *victim = listHead->prev;

	dListRemoveNode(victim);

	return victim;
}


#endif /* DLIST_H_INCLUDED */
