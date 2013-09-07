#ifndef STORAGE_H
#define STORAGE_H

#include <stdlib.h>

void storage_init(size_t size);
void storage_add(int key, void *data);
void* storage_get(int key);
void storage_del(int key);
void storage_destroy(void);

void storage_print(void);

#endif
