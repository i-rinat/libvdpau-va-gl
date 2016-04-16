// test-010

// creating and destroying couple of VdpDevice from different threads
// caused deadlocks and crashes

#include <stdio.h>
#include <pthread.h>
#include "tests-common.h"

VdpDevice device1;
VdpDevice device2;

void *
thread_1(void *param)
{
    (void)param;
    ASSERT_OK(vdpDeviceDestroy(device1));
    return NULL;
}

int main(void)
{
    device1 = create_vdp_device();
    device2 = create_vdp_device();

    pthread_t thread_id_1;
    pthread_create(&thread_id_1, NULL, thread_1, NULL);
    pthread_join(thread_id_1, NULL);

    ASSERT_OK(vdpDeviceDestroy(device2));
    printf("pass\n");
    return 0;
}
