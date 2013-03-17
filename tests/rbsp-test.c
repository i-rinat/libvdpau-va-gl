#ifdef NDEBUG
#undef NDEBUG
#endif

#include "bitstream.h"
#include <stdio.h>
#include <assert.h>

int main(void)
{
	unsigned char buf[] = {0xa6, 0x42, 0x98, 0xe2, 0x3f};
    rbsp_state_t st;
    rbsp_attach_buffer(&st, buf, 5);
    assert (0 == rbsp_get_uev(&st));
    assert (1 == rbsp_get_uev(&st));
    assert (2 == rbsp_get_uev(&st));
    assert (3 == rbsp_get_uev(&st));
    assert (4 == rbsp_get_uev(&st));
    assert (5 == rbsp_get_uev(&st));
    assert (6 == rbsp_get_uev(&st));
    assert (7 == rbsp_get_uev(&st));
    assert (0 == rbsp_get_uev(&st));
    assert (0 == rbsp_get_uev(&st));

    rbsp_attach_buffer(&st, buf, 5);
    assert (1 == rbsp_get_u(&st, 1));
    assert (2 == rbsp_get_u(&st, 3));
    assert (3 == rbsp_get_u(&st, 3));
    assert (4 == rbsp_get_u(&st, 5));
    assert (5 == rbsp_get_u(&st, 5));
    assert (6 == rbsp_get_u(&st, 5));
    assert (7 == rbsp_get_u(&st, 5));
    assert (8 == rbsp_get_u(&st, 7));
    assert (1 == rbsp_get_u(&st, 1));

    rbsp_attach_buffer(&st, buf, 5);
    assert ( 0 == rbsp_get_sev(&st));
    assert ( 1 == rbsp_get_sev(&st));
    assert (-1 == rbsp_get_sev(&st));
    assert ( 2 == rbsp_get_sev(&st));
    assert (-2 == rbsp_get_sev(&st));
    assert ( 3 == rbsp_get_sev(&st));
    assert (-3 == rbsp_get_sev(&st));
    assert ( 4 == rbsp_get_sev(&st));
    assert ( 0 == rbsp_get_sev(&st));
    assert ( 0 == rbsp_get_sev(&st));

    unsigned char buf2[] = {0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00};
    rbsp_attach_buffer(&st, buf2, 8);
    for (int k = 0; k < 6 * 8; k ++) {
        assert (0 == rbsp_get_u(&st, 1));
    }

    unsigned char buf3[] = {0x00, 0x00, 0x03, 0xff, 0xff};
    rbsp_attach_buffer(&st, buf3, 5);
    for (int k = 0; k < 16; k ++) assert (0 == rbsp_get_u(&st, 1));
    for (int k = 0; k < 16; k ++) assert (1 == rbsp_get_u(&st, 1));

    unsigned char buf4[] = {0x00, 0x00, 0x00, 0x03, 0xff};
    rbsp_attach_buffer(&st, buf4, 5);
    for (int k = 0; k < 24; k ++) assert (0 == rbsp_get_u(&st, 1));
    for (int k = 0; k < 8; k ++) assert (1 == rbsp_get_u(&st, 1));

    printf ("pass\n");
}
