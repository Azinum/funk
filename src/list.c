// list.c

#include <stdlib.h>

#include "list.h"

void* list_init(const u32 size, u32 count) {
	void* list = m_calloc(size, count);
	if (!list) {
		fprintf(stderr, "Allocation failed\n");
		return NULL;
	}
	return list;
}
