// test-010

// creating and destroying couple of VdpDevice from different threads
// caused deadlocks and crashes

#include "vdpau-init.h"
#include <stdio.h>
#include <pthread.h>

VdpDevice device1;
VdpDevice device2;

void *
thread_1(void *param)
{
    ASSERT_OK(vdp_device_destroy(device1));
    return NULL;
}

int main(void)
{
    ASSERT_OK(vdpau_init_functions(&device1, NULL, 0));
    ASSERT_OK(vdpau_init_functions(&device2, NULL, 0));

    pthread_t thread_id_1;
    pthread_create(&thread_id_1, NULL, thread_1, NULL);
    pthread_join(thread_id_1, NULL);

    ASSERT_OK(vdp_device_destroy(device2));
    printf("pass\n");
    return 0;
}
