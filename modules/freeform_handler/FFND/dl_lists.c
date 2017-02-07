/*
 * FILENAME: dl_lists.c
 * 
 * CONTAINS:
 *		DLL_NODE_PTR 	dll_init
 *		DLL_NODE_PTR 	dll_insert
 *		DLL_NODE_PTR 	dll_add
 *				void 	dll_delete
 *				int		dll_free
 *		DLL_NODE_PTR 	dll_init_contiguous
 *				int		make_dll_title_list
 *
 * CAVEAT:
 * No claims are made as to the suitability of the accompanying
 * source code for any purpose.  Although this source code has been
 * used by the NOAA, no warranty, expressed or implied, is made by
 * NOAA or the United States Government as to the accuracy and
 * functioning of this source code, nor shall the fact of distribution
 * constitute any such endorsement, and no responsibility is assumed
 * by NOAA in connection therewith.  The source code contained
 * within was developed by an agency of the U.S. Government.
 * NOAA's National Geophysical Data Center has no objection to the
 * use of this source code for any purpose since it is not subject to
 * copyright protection in the U.S.  If this source code is incorporated
 * into other software, a statement identifying this source code may be
 * required under 17 U.S.C. 403 to appear with any copyright notice.
 */

#ifdef DLL_CHK
#ifdef NDEBUG
#undef NDEBUG
#endif
#endif

#include <freeform.h>

/* Private */

#ifdef DLL_CHK
#define HEAD_NODE 0x8000
#define FREED     0x4000

#define DLL_COUNT(head) ((head)->count)

#define DLL_SET_HEAD_NODE(h) ((h)->status|=HEAD_NODE)
#define DLL_MARK_FREED(n) ((n)->status|=FREED)
#define DLL_IS_FREED(n) ((n)->status&FREED)
#define DLL_SET_NEW(n) ((n)->status=0)

#define DLL_IS_HEAD_NODE(n) ((n)->data.type == DLL_HEAD_NODE && (n)->status&HEAD_NODE)

static DLL_NODE_PTR dll_node_create(DLL_NODE_PTR link_node);
static DLL_NODE_PTR find_head_node(DLL_NODE_PTR head);
#else
#define DLL_IS_HEAD_NODE(n) ((n)->data.type == DLL_HEAD_NODE)

static DLL_NODE_PTR dll_node_create(void);	
#endif

/*
 * NAME: dll_init
 *		
 * PURPOSE: To Initialize a generic linked list
 *
 * AUTHOR:	T. Habermann, NGDC, (303) 497 - 6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	NODE_PTR dll_init()
 *
 * DESCRIPTION: Memory is allocated for the header node (of type NODE).
 *				No memory is allocated for a data element, because the
 * 				data-element pointer in the header node points to NULL.
 *				The previous and next node pointers are set to point to
 *				the header node.
 *
 * DLL_COUNT(head) is initialized to zero.  Every time a new node is added
 * (inserted) into the list, DLL_COUNT() is incremented.  Every time a node is
 * removed from the list, DLL_COUNT() is decremented. DLL_IS_HEAD_NODE(head)
 * is set to 1 (DLL_YES).
 *
 * RETURNS:	If successful: A pointer to header node.
 *			If Error: NULL
 *
 * SYSTEM DEPENDENT FUNCTIONS: None
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "dll_init"

DLL_NODE_PTR dll_init(void)
{
	/*  Allocate the header node */
#ifdef DLL_CHK
	DLL_NODE_PTR head = (DLL_NODE_PTR)dll_node_create(NULL);
#else
	DLL_NODE_PTR head = (DLL_NODE_PTR)dll_node_create();
#endif

	if (head)
	{	/* Successful Allocation */
#ifdef DLL_CHK
		DLL_COUNT(head) = 0;
		DLL_SET_HEAD_NODE(head);
#endif
		head->data.type = DLL_HEAD_NODE;

		head->next = head->previous = head;
	}
	return(head);
}

/*
 * NAME: dll_insert
 *		
 *
 * PURPOSE: Allocate memory for and install a new node in a linked list.
 *
 * AUTHOR:	T. Habermann, NGDC, (303) 497 - 6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE: DLL_NODE_PTR dll_ins(DLL_NODE_PTR next_node, unsigned dsize)
 *
 * DESCRIPTION: dll_insert() inserts a new node in a generic linked
 *	list before next_node.  A new node is allocated and linked into the node list.
 *
 * RETURNS:	If successful: A pointer to the new node.
 *			If Error: NULL
 *
 * SYSTEM DEPENDENT FUNCTIONS: None
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "dll_insert"

DLL_NODE_PTR dll_insert(DLL_NODE_PTR next_node)
{
#ifdef DLL_CHK
	DLL_NODE_PTR new_node = dll_node_create(next_node);
#else
	DLL_NODE_PTR new_node = dll_node_create();
#endif

	FF_VALIDATE(next_node);
	
	if (new_node == NULL)
		return(NULL);

	new_node->next = next_node;
	new_node->previous = dll_previous(next_node);

	next_node->previous = new_node;
	new_node->previous->next = new_node;
	
	return(new_node);
}

/*
 * NAME: dll_add
 *		
 *
 * PURPOSE: Allocate memory for and install a new node in a linked list.
 *
 * AUTHOR:	T. Habermann, NGDC, (303) 497 - 6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE: DLL_NODE_PTR dll_add(DLL_NODE_PTR previous_node, unsigned dsize)
 *
 * DESCRIPTION: dll_add() adds a new node to the end of a generic linked list.
 *
 * RETURNS:	If successful: A pointer to the new node.
 *			If Error: NULL
 *
 * SYSTEM DEPENDENT FUNCTIONS: None
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "dll_add"

DLL_NODE_PTR dll_add(DLL_NODE_PTR head_node)
{
	dll_rewind(&head_node);

	return(dll_insert(head_node));
/*
#ifdef DLL_CHK
	DLL_NODE_PTR new_node = dll_node_create(head_node);
#else
	DLL_NODE_PTR new_node = dll_node_create();
#endif
	
	FF_VALIDATE(head_node);
	
	if (new_node == NULL)
		return(NULL);

	dll_previous(new_node) = head_node;
	dll_next(new_node) = dll_next(head_node);

	dll_previous(dll_next(new_node)) = new_node;
	dll_next(head_node) = new_node;

	return(new_node);
*/
}

static void dll_disconnect_node(DLL_NODE_PTR node)
{
	FF_VALIDATE(node);

#ifdef DLL_CHK
	assert(!DLL_IS_HEAD_NODE(node));
	assert(!DLL_IS_FREED(node));
	
	DLL_MARK_FREED(node);
	DLL_COUNT(find_head_node(node))--;
#endif

	node->previous->next = dll_next(node);     /* set next_node of prev_node */
	node->next->previous = dll_previous(node);     /* Set prev_node of next_node */

	node->previous = node->next = NULL;

#ifdef FF_CHK_ADDR
	node->check_address = NULL;
#endif
}

static void dll_destroy_data(DLL_DATA_PTR data)
{
	switch (data->type)
	{
		case DLL_VAR:
			FF_VALIDATE(data->u.var);
			ff_destroy_variable(data->u.var);
			data->u.var = NULL;
		break;

		case DLL_FMT:
			FF_VALIDATE(data->u.fmt);
			ff_destroy_format(data->u.fmt);
			data->u.fmt = NULL;
		break;

		case DLL_FD:
			FF_VALIDATE(data->u.fd);
			fd_destroy_format_data(data->u.fd);
			data->u.fd = NULL;
		break;

		case DLL_AC:
			FF_VALIDATE(data->u.ac);
			ff_destroy_array_conduit(data->u.ac);
			data->u.ac = NULL;
		break;

		case DLL_PI:
			FF_VALIDATE(data->u.pi);
			ff_destroy_process_info(data->u.pi);
			data->u.pi = NULL;
		break;

		case DLL_ERR:
			FF_VALIDATE(data->u.err);
			ff_destroy_error(data->u.err);
			data->u.err = NULL;
		break;

		case DLL_DF:
			FF_VALIDATE(data->u.err);
			ff_destroy_data_flag(data->u.df);
			data->u.df = NULL;
		break;

		default:
			assert(0);
		break;
	}
}

/*
 * NAME: dll_delete
 *		
 * PURPOSE: To Delete a node from a standard list.
 *
 * AUTHOR:	T. Habermann, NGDC, (303) 497 - 6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE: void dll_delete(DLL_NODE_PTR)
 *
 * DESCRIPTION:	dll_delete() deletes the node from the linked list and resets
 *				the next and previous node pointers for both the previous and next
 *				nodes.  dll_delete() releases the memory allocated for the
 *				node.  If the data storage had been allocated in dll_ins(),
 *				then this storage is freed as well. If the data element
 *				pointed to by the node contains pointers to memory allocated
 *				by the user, this memory should be freed before deleting the
 *				node.
 *
 * The header node's DLL_COUNT() is decremented.  Assertions are used to verify
 * that the deletion node is not the header node, and that if the data block
 * associated with the deletion node is to be free()'d, it was allocated
 * separate from the node itself.  This is based on the dll_is_data_trojan()
 * bitfield being set to 0 (DLL_NO) for separate allocations.
 *
 * Note:  Cannot perform last assertion, thanks to strdb abuse.
 *
 * RETURNS:	None
 *
 * SYSTEM DEPENDENT FUNCTIONS: None
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "dll_delete"

void dll_delete(DLL_NODE_PTR node)
{
	FF_VALIDATE(node);
	
	dll_disconnect_node(node);

	dll_destroy_data(&(node->data));

	memFree(node, "List Node");

	return;
}

void dll_delete_node(DLL_NODE_PTR node)
{
	FF_VALIDATE(node);
	
	dll_disconnect_node(node);

	memFree(node, "List Node");

	return;
}

/*
 * NAME:		dll_free
 *		
 * PURPOSE:		To delete all nodes of a doublely linked list and the data
 *				the list points to [optionally].
 *
 * USAGE:    	int dll_free(DLL_NODE_PTR node, short delete_data)
 *
 * RETURNS:		number of nodes deleted
 *
 * DESCRIPTION:	dll_free() deletes all nodes from a standard doubly
 *				linked list. If the delete_data flag is TRUE,
 *				then this storage is freed as well. If the data element
 *				pointed to by the node contains pointers to memory allocated
 *				by the user, this memory should be freed before deleting the
 *				node.
 *
 * Assertions are used to verify that if delete_data is FALSE, that the
 * dll_data(node) is the NULL pointer.  Assertions are used to verify that
 * if the dll_data(node) is to be free()'d, it was separately allocated from
 * the node.
 *
 * SYSTEM DEPENDENT FUNCTIONS:	none
 *
 * GLOBAL:	none
 *
 * AUTHOR:	Ted Habermann, NGDC, 303-497-6284, haber@mail.ngdc.noaa.gov
 * 			Liping Di, NGDC, 303-497-6284, lpd@mail.ngdc.noaa.gov
 *
 * COMMENTS:	
 *
 * KEYWORDS:	dll
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "dll_free"

/* Was the header node of the list passed into dll_free?  If not, find the header node
   and change the DLL_NODE_PTR "head" to point to it.
*/

void dll_rewind
	(
	 DLL_NODE_HANDLE head_h
	)
{
	while (!DLL_IS_HEAD_NODE(*head_h))
		*head_h = dll_next(*head_h);
}

int dll_free_holdings(DLL_NODE_PTR head)
{
	DLL_NODE_PTR node = NULL;
	int count = 0;

	FF_VALIDATE(head);
	if (head == NULL)
		return(0);
	
	dll_rewind(&head);

	node = dll_first(head);
	while (!DLL_IS_HEAD_NODE(node))
	{
		dll_delete(node);

		count++;
		node = dll_first(head);
	}

#ifdef DLL_CHK
	assert(DLL_COUNT(head) == 0);
#endif

	head->next = head->previous = NULL;

#ifdef FF_CHK_ADDR
	head->check_address = NULL;
#endif

	memFree(head, "Header Node");          

	return(count);
}
		
int dll_free_list(DLL_NODE_PTR head)
{
	DLL_NODE_PTR node = NULL;
	int count = 0;

	FF_VALIDATE(head);
	if (head == NULL)
		return(0);
	
	dll_rewind(&head);

	node = dll_first(head);
	while (!DLL_IS_HEAD_NODE(node))
	{
		dll_delete_node(node);

		count++;
		node = dll_first(head);
	}

#ifdef DLL_CHK
	assert(DLL_COUNT(head) == 0);
#endif

	head->next = head->previous = NULL;

#ifdef FF_CHK_ADDR
	head->check_address = NULL;
#endif

	memFree(head, "Header Node");          

	return(count);
}
		
#undef ROUTINE_NAME
#define ROUTINE_NAME "dll_node_create"

#ifdef DLL_CHK
static DLL_NODE_PTR dll_node_create(DLL_NODE_PTR link_node)
#else
static DLL_NODE_PTR dll_node_create(void)
#endif
/*****************************************************************************
 * NAME: dll_node_create()
 *
 * PURPOSE:  Allocate a memory block for a new DLL_NODE, plus dsize for data
 *
 * USAGE:  node = dll_node_create();
 *
 * RETURNS:  NULL if operation fails, otherwise a pointer to an allocated
 * node/data block
 *
 * DESCRIPTION:  Allocates space for DLL_NODE.
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/
{
	DLL_NODE_PTR node;

	/* Get memory for data element and set dll_data(node)
	to point to it */

	node = (DLL_NODE_PTR)memMalloc(sizeof(DLL_NODE), "List Node");

	if (node == NULL)
	{
		err_push(ERR_MEM_LACK, "Allocating list node");
		return(NULL);
	}

#ifdef FF_CHK_ADDR
	node->check_address = node;
#endif

#ifdef DLL_CHK
	DLL_SET_NEW(node);
#endif
	
	node->next = node->previous = NULL;
#ifdef DLL_CHK
	node->count = node->data.type = 0;
#endif
	node->data.u.var = NULL;
	
	/* Find header */
#ifdef DLL_CHK
	if (link_node)
		DLL_COUNT(find_head_node(link_node))++;
#endif

	return(node);
}

#ifdef DLL_CHK
static DLL_NODE_PTR find_head_node(DLL_NODE_PTR head)
{
	unsigned int i = 0;

	assert(head);
	while (!DLL_IS_HEAD_NODE(head))
	{
		if (++i == UINT_MAX)
		{
			assert(i != UINT_MAX);
			break;
		}
		head = dll_previous(head);
	}
	
	return(head);
}
#endif

#ifdef FF_DBG

VARIABLE_PTR           FF_VARIABLE(VARIABLE_LIST variable_list)
{
	FF_VALIDATE(variable_list);

	if (DLL_IS_HEAD_NODE(variable_list))
		return(NULL);

	assert(variable_list->data.type == DLL_VAR);

	return(variable_list->data.u.var);
}

FORMAT_PTR             FF_FORMAT(FORMAT_LIST format_list)
{
	FF_VALIDATE(format_list);

	if (DLL_IS_HEAD_NODE(format_list))
		return(NULL);

	assert(format_list->data.type == DLL_FMT);

	return(format_list->data.u.fmt);
}

FORMAT_DATA_PTR		  FD_FORMAT_DATA(FORMAT_DATA_LIST format_data_list)
{
	FF_VALIDATE(format_data_list);

	if (DLL_IS_HEAD_NODE(format_data_list))
		return(NULL);

	assert(format_data_list->data.type == DLL_FD);

	return(format_data_list->data.u.fd);
}

FF_ARRAY_CONDUIT_PTR	  FF_AC(FF_ARRAY_CONDUIT_LIST array_conduit_list)
{
	FF_VALIDATE(array_conduit_list);

	if (DLL_IS_HEAD_NODE(array_conduit_list))
		return(NULL);

	assert(array_conduit_list->data.type == DLL_AC);

	return(array_conduit_list->data.u.ac);
}

PROCESS_INFO_PTR       FF_PI(PROCESS_INFO_LIST process_info_list)
{
	FF_VALIDATE(process_info_list);

	if (DLL_IS_HEAD_NODE(process_info_list))
		return(NULL);

	assert(process_info_list->data.type == DLL_PI);

	return(process_info_list->data.u.pi);
}

FF_ERROR_PTR           FF_EP(FF_ERROR_LIST error_list)
{
	FF_VALIDATE(error_list);

	if (DLL_IS_HEAD_NODE(error_list))
		return(NULL);

	assert(error_list->data.type == DLL_ERR);

	return(error_list->data.u.err);
}

FF_DATA_FLAG_PTR       FF_DF(FF_DATA_FLAG_LIST data_flag_list)
{
	FF_VALIDATE(data_flag_list);

	if (DLL_IS_HEAD_NODE(data_flag_list))
		return(NULL);

	assert(data_flag_list->data.type == DLL_DF);

	return(data_flag_list->data.u.df);
}

#endif /* FF_DBG */

DLL_NODE_PTR dll_first(DLL_NODE_PTR node)
{
	FF_VALIDATE(node);

	dll_rewind(&node);

	return(node->next);
}

DLL_NODE_PTR dll_last(DLL_NODE_PTR node)
{
	FF_VALIDATE(node);

	dll_rewind(&node);

	return(node->previous);
}

void dll_assign
	(
	 void *data,
	 FF_DLL_DATA_TYPES type,
	 DLL_NODE_PTR node
	)
{
	FF_VALIDATE(node);

	node->data.type = type;

	switch (type)
	{
		case DLL_VAR:
			assert(node->data.u.var == NULL);

			node->data.u.var = data;
			FF_VALIDATE(node->data.u.var);
		break;

		case DLL_FMT:
			assert(node->data.u.fmt == NULL);

			node->data.u.fmt = data;
			FF_VALIDATE(node->data.u.var);
		break;

		case DLL_FD:
			assert(node->data.u.fd == NULL);

			node->data.u.fd = data;
			FF_VALIDATE(node->data.u.var);
		break;

		case DLL_AC:
			assert(node->data.u.ac == NULL);

			node->data.u.ac = data;
			FF_VALIDATE(node->data.u.var);
		break;

		case DLL_PI:
			assert(node->data.u.pi == NULL);

			node->data.u.pi = data;
			FF_VALIDATE(node->data.u.var);
		break;

		case DLL_ERR:
			assert(node->data.u.err == NULL);

			node->data.u.err = data;
			FF_VALIDATE(node->data.u.var);
		break;

		case DLL_DF:
			assert(node->data.u.df == NULL);

			node->data.u.df = data;
			FF_VALIDATE(node->data.u.df);
		break;

		default:
			assert(0);
			node->data.type = 0;
		break;
	}
}

#ifdef FF_DBG

DLL_NODE_PTR dll_next(DLL_NODE_PTR node)
{
	FF_VALIDATE(node);

	return(node->next);
}

DLL_NODE_PTR dll_previous(DLL_NODE_PTR node)
{
	FF_VALIDATE(node);

	return(node->previous);
}

#endif /* FF_DBG */

int list_replace_items(pgenobj_cmp_t lri_cmp, DLL_NODE_PTR list)
{
	int error = 0;

	FF_VALIDATE(list);

	list = dll_first(list);
	while (!DLL_IS_HEAD_NODE(list))
	{
		DLL_NODE_PTR list_walker = NULL;

		list_walker = dll_next(list);

		while (!DLL_IS_HEAD_NODE(list_walker))
		{
			if ((*lri_cmp)(list->data.u.var, list_walker->data.u.var))
			{
				list = dll_previous(list);
				dll_delete(list->next);
				break;
			}

			list_walker = dll_next(list_walker);
		}

		list = dll_next(list);
	}

	return(error);
}
