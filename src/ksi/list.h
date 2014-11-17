#ifndef KSI_LIST_H_
#define KSI_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup lists Lists
 * @{
 */

/**
 * Generic list type for storing void* pointers.
 */
typedef struct KSI_List_st KSI_List;

/**
 * Macro to get the list type name for a given type.
 * \param[in]	type	Type of the list.
 */
#define KSI_LIST(type) type##List

/**
 * TODO!
 */
#define KSI_NEW_LIST(type, list) KSI_List_new(type##_free, (list))

/**
 * TODO!
 */
#define KSI_NEW_REFLIST(type, list) KSI_RefList_new(type##_free, type##_ref, (list))

/**
 * Generates the function name for a list with a given type.
 * \param[in]	type	Type of the list.
 * \param[in]	name	Name of the function.
 */
#define KSI_LIST_FN_NAME(type, name) type##List_##name

/**
 * This macro defines a new list of given type.
 * \param[in]	type	Type of the elements stored in the list.
 */
#define KSI_DEFINE_LIST(type) 													\
/*!
 List of \ref type.
*/ 														\
typedef struct type##_list_st KSI_LIST(type);									\
/*! Frees the memory allocated by the list and performes \ref type##_free() on
	all the elements.
	\param[in]	list		Pointer to the list.
 */																				\
void KSI_LIST_FN_NAME(type, free)(KSI_LIST(type) *list);						\
/*! Creates a new list of \ref type.
	\param[in]	ctx		KSI context.
	\param[out]	list	Pointer ot the receiving pointer.
	\return status code (#KSI_OK, when operation succeeded, otherwise an error code).
 */																				\
int KSI_LIST_FN_NAME(type, new)(KSI_CTX *ctx, KSI_LIST(type) **list);					\
/*! Appends the element to the list.
	\param[in]	list	Pointer to the list.
	\param[in]	el		Pointer to the element being added.
	\return status code (#KSI_OK, when operation succeeded, otherwise an error code).
	\note After appending the element to the list, the element belongs to the list
		and it will be freed if the list is freed.
	\see \ref type##_free
 */																				\
int KSI_LIST_FN_NAME(type, append)(KSI_LIST(type) *list, type *el);				\
/*! This function finds the index of a given element.
	\param[in]	list	Pointer to the list.
	\param[in]	el		Pointer to the element.
	\param[out]	pos		Pointer to the receiving ponter.
	\return status code (#KSI_OK, when operation succeeded, otherwise an error code).
 */																				\
int KSI_LIST_FN_NAME(type, indexOf)(KSI_LIST(type) *list, type *el, size_t **pos);		\
/*! Add the element to the given position in the list. All elements with
	equal or greater indices are shifted.
	\param[in]	list	Pointer to the list.
	\param[in]	pos		Position where to insert the element.
	\param[in]	el		Pointer to the element being added.
	\return status code (#KSI_OK, when operation succeeded, otherwise an error code).
	\note After add the element to the list, the element belongs to the list
		and it will be freed if the list is freed.
	\see \ref type##_free
 */																				\
int KSI_LIST_FN_NAME(type, insertAt)(KSI_LIST(type) *list, size_t pos, type *el);			\
/*! Replace the element at the given position in the list. The old element
	will be freed.
	\param[in]	list	Pointer to the list.
	\param[in]	pos		Position where to insert the element.
	\param[in]	el		Pointer to the element being added.
	\return status code (#KSI_OK, when operation succeeded, otherwise an error code).
	\note After add the element to the list, the element belongs to the list
		and it will be freed if the list is freed.
	\see \ref type##_free
 */																				\
int KSI_LIST_FN_NAME(type, replaceAt)(KSI_LIST(type) *, size_t, type *);		\
/*! Removes an element at the given position. If the out parameter is set to
	NULL, the removed element is freed implicitly with type##_free.
	\param[in]	list	Pointer to the list.
	\param[in]	pos		Position of the element to be removed.
	\param[out]	el		Pointer to the receiving pointer.
	\return status code (#KSI_OK, when operation succeeded, otherwise an error code).
	\note If the element is removed from the list and returned via output parameter
		to the caller, the caller is responsible for freeing the element.
	\see type##_free
*/ \
int KSI_LIST_FN_NAME(type, remove)(KSI_LIST(type) *list, size_t pos, type **el);			\
/*! Returns the list of the element.
	\param[in]	list	Pointer to the list.
	\return Returns the lenght of the list or 0 if the list is \c NULL.
*/ \
size_t KSI_LIST_FN_NAME(type, length)(KSI_LIST(type) *list);						\
/*! Method for accessing an element at any given position.
	\param[in]	list	Pointer to the list.
	\param[in]	pos		Position of the element.
	\param[out]	el		Pointer to the receiving pointer.
	\return status code (#KSI_OK, when operation succeeded, otherwise an error code).
	\note The returned element still belongs to the list and may not be freed
		by the caller.
*/ \
int KSI_LIST_FN_NAME(type, elementAt)(KSI_LIST(type) *list, size_t pos, type **el);	\
/*! Function to sort the elements in the list.
	\param[in]	list	Pointer to the list.
	\param[in]	fn		Sort function.
	\return status code (#KSI_OK, when operation succeeded, otherwise an error code).
*/ \
int KSI_LIST_FN_NAME(type, sort)(KSI_LIST(type) *list, int (*fn)(const type **, const type **));	\

void KSI_List_free(KSI_List *list);
int KSI_List_new(void (*obj_free)(void *), KSI_List **list);
int KSI_List_append(KSI_List *list, void *o);
int KSI_List_remove(KSI_List *list, size_t pos, void **o);
int KSI_List_indexOf(KSI_List *list, void *o, size_t **i);
int KSI_List_insertAt(KSI_List *list, size_t pos, void *o);
int KSI_List_replaceAt(KSI_List *list, size_t pos, void *o);
int KSI_List_elementAt(KSI_List *list, size_t pos, void **o);
size_t KSI_List_length(KSI_List *list);
int KSI_List_sort(KSI_List *list, int (*)(const void *, const void *));

/**
 * This macto implements all the functions of a list for a given type.
 * \param[in]	type	The type of the elements stored in the list.
 * \param[in]	free_fn	Function pointer to the free method of stored elements. May be \c NULL
 */
#define KSI_IMPLEMENT_LIST(type, free_fn)											\
struct type##_list_st { 															\
	KSI_List *list;																	\
};																					\
int KSI_LIST_FN_NAME(type, new)(KSI_CTX *ctx, KSI_LIST(type) **list) {				\
	int res = KSI_UNKNOWN_ERROR;													\
	KSI_LIST(type) *l = NULL;														\
	l = KSI_new(KSI_LIST(type));													\
	if (l == NULL) {																\
		res = KSI_OUT_OF_MEMORY;													\
		goto cleanup;																\
	}																				\
	res = KSI_List_new((void (*)(void *))free_fn, &l->list);						\
	if (res != KSI_OK) goto cleanup;												\
	*list = l;																		\
	l = NULL;																		\
	res = KSI_OK;																	\
cleanup:																			\
	KSI_LIST_FN_NAME(type, free)(l);												\
	return res;																		\
}																					\
void KSI_LIST_FN_NAME(type, free)(KSI_LIST(type) *list) {							\
	if (list != NULL) {																\
		KSI_List_free(list->list);													\
		KSI_free(list);																\
	}																				\
} 																					\
int KSI_LIST_FN_NAME(type, append)(KSI_LIST(type) *list, type *o) {					\
	return KSI_List_append(list->list, o);											\
}																					\
int KSI_LIST_FN_NAME(type, indexOf)(KSI_LIST(type) *list, type *o, size_t **pos) {	\
	return KSI_List_indexOf(list->list, o, pos);									\
}																					\
int KSI_LIST_FN_NAME(type, insertAt)(KSI_LIST(type) *list, size_t pos, type *o) {	\
	return KSI_List_insertAt(list->list, pos, o);									\
}																					\
int KSI_LIST_FN_NAME(type, replaceAt)(KSI_LIST(type) *list, size_t pos, type *o) {	\
	return KSI_List_replaceAt(list->list, pos, o);									\
}																					\
size_t KSI_LIST_FN_NAME(type, length)(KSI_LIST(type) *list) {					\
	return list != NULL ? KSI_List_length(list->list): 0;							\
}																					\
int KSI_LIST_FN_NAME(type, remove)(KSI_LIST(type) *list, size_t pos, type **o) {	\
	return KSI_List_remove(list->list, pos, (void **)o);							\
}																					\
int KSI_LIST_FN_NAME(type, elementAt)(KSI_LIST(type) *list, size_t pos, type **o) {	\
	return KSI_List_elementAt(list->list, pos, (void **) o);						\
}																					\
int KSI_LIST_FN_NAME(type, sort)(KSI_LIST(type) *list, int (*cmp)(const type **a, const type **b)) {	\
	return KSI_List_sort(list->list, (int (*)(const void *, const void *)) cmp);	\
}																					\

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* KSI_LIST_H_ */
