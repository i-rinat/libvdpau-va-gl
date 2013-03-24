#ifndef __GLOBALS_H
#define __GLOBALS_H

#include <pthread.h>

struct global_data {
    pthread_mutex_t     mutex;
};

#endif /* __GLOBALS_H */
