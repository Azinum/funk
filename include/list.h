// list.h

#ifndef _LIST_H
#define _LIST_H

#include <assert.h>

#include "memory.h"

#define list_push(list, count, value) do { \
	if (list == NULL) { list = list_init(sizeof(value), 1); list[0] = value; count = 1; break; } \
	void* new_list = m_realloc(list, count * sizeof(*list), (1 + count) * (sizeof(value))); \
	if (new_list) { \
		list = new_list; \
		list[(count)++] = value; \
	} \
} while (0); \

#define list_realloc(list, count, new_size) do { \
  if (list == NULL) break; \
  if (new_size == 0) { list_free(list, count); break; } \
	void* new_list = m_realloc(list, count * sizeof(*list), (new_size) * (sizeof(*list))); \
  list = new_list; \
  count = new_size; \
} while(0); \

#define list_shrink(list, count, num) { \
  if ((count - num) >= 0) { \
	  list_realloc(list, count, count - num); \
  } \
}
 
#define list_assign(list, count, index, value) { \
	assert(list != NULL); \
	if (index < count) { \
		list[index] = value; \
	} else { assert(0); } \
} \

#define list_free(list, count) { \
	if ((list) != NULL) { \
		m_free(list, count * sizeof(*list)); \
		count = 0; \
    list = NULL; \
	}\
}

// size of type, count of elements to allocate
void* list_init(const u32 size, u32 count);

#endif
