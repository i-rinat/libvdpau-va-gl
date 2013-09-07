#include "storage.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct sNode{
    void* data;
    int key;
    struct sNode *next;
} Node;

typedef Node* pNode; 

static pNode* _map = NULL;
static size_t _size = 0;

static pNode node_create()
{
    pNode node = malloc(sizeof(Node));
    node->data = NULL;
    node->key = 0;
    node->next = NULL;
    
    return node;
}

static size_t hash(int key)
{
    return key % _size;
}

void storage_init(size_t size)
{
    _map = calloc(size, sizeof(pNode));
    _size = size;
}

void storage_add(int key, void *data)
{
    size_t i = hash(key);
    pNode list = _map[i];
    bool done = false;
    
    while(list != NULL  && !done)
    {
        if (list->key == key)
        {
            list->data = data;
            done = true;
        }
        else
            list = list->next;
    }
    
    if(!done)
    {
        pNode node = node_create();
        node->data = data;
        node->key = key;
        node->next = _map[i];
    
        _map[i] = node;
    }
}

void* storage_get(int key)
{
    void *data = NULL;
    pNode list = _map[hash(key)];
    bool done = false;
    
    while(list != NULL  && !done)
    {
        if (list->key == key)
        {
            data = list->data;
            done = true;
        }
        else
            list = list->next;
    }
    
    return data;
}
void storage_destroy();

static void node_list_print(pNode node)
{
    while(node != NULL)
    {
        printf("(%d; %p)->",node->key, node->data);
        node= node->next;
    }
}

void storage_print(void)
{
    size_t i = 0;
    for(i = 0; i < _size; i++)
    {
        printf("[%d]: ",i);
        node_list_print(_map[i]);
        printf("\n");
    }
}
