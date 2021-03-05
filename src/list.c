// list.c

#include <stdlib.h>

#include "list.h"

void* list_init(const unsigned int size, unsigned int count) {
	void* list = m_calloc(size, count);
	if (!list) {
		fprintf(stderr, "Allocation failed\n");
		return NULL;
	}
	return list;
}
