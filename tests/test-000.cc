#undef NDEBUG
#include <stdio.h>
#include <assert.h>
#include <vector>
#include "../src/bitstream.hh"


using std::vector;

static
void
test_get_uev()
{
    const vector<uint8_t> buf{0xa6, 0x42, 0x98, 0xe2, 0x3f};
    vdp::RBSPState st{buf};
    int64_t a;

    a = st.get_uev(); assert(a == 0);
    a = st.get_uev(); assert(a == 1);
    a = st.get_uev(); assert(a == 2);
    a = st.get_uev(); assert(a == 3);
    a = st.get_uev(); assert(a == 4);
    a = st.get_uev(); assert(a == 5);
    a = st.get_uev(); assert(a == 6);
    a = st.get_uev(); assert(a == 7);
    a = st.get_uev(); assert(a == 0);
    a = st.get_uev(); assert(a == 0);
}

static
void
test_get_u()
{
    const vector<uint8_t> buf{0xa6, 0x42, 0x98, 0xe2, 0x3f};
    vdp::RBSPState st{buf};
    int64_t a;

    a = st.get_u(1); assert(a == 1);
    a = st.get_u(3); assert(a == 2);
    a = st.get_u(3); assert(a == 3);
    a = st.get_u(5); assert(a == 4);
    a = st.get_u(5); assert(a == 5);
    a = st.get_u(5); assert(a == 6);
    a = st.get_u(5); assert(a == 7);
    a = st.get_u(7); assert(a == 8);
    a = st.get_u(1); assert(a == 1);
}

static
void
test_get_sev()
{
    const vector<uint8_t> buf{0xa6, 0x42, 0x98, 0xe2, 0x3f};
    vdp::RBSPState st{buf};
    int64_t a;

    a = st.get_sev(); assert(a == 0);
    a = st.get_sev(); assert(a == 1);
    a = st.get_sev(); assert(a == -1);
    a = st.get_sev(); assert(a == 2);
    a = st.get_sev(); assert(a == -2);
    a = st.get_sev(); assert(a == 3);
    a = st.get_sev(); assert(a == -3);
    a = st.get_sev(); assert(a == 4);
    a = st.get_sev(); assert(a == 0);
    a = st.get_sev(); assert(a == 0);
}

static
void
test_emulation_prevention_bytes_1()
{
    const vector<uint8_t> buf{0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00};
    vdp::RBSPState st{buf};

    for (int k = 0; k < 6 * 8; k ++) {
        int64_t a = st.get_u(1);
        assert(a == 0);
    }

    const auto bits_eaten = st.bits_eaten();
    assert(bits_eaten == 6 * 8);
}

static
void
test_emulation_prevention_bytes_2()
{
    const vector<uint8_t> buf{0x00, 0x00, 0x03, 0xff, 0xff};
    vdp::RBSPState st{buf};

    for (int k = 0; k < 16; k ++) {
        const int64_t a = st.get_u(1);
        assert(a == 0);
    }

    for (int k = 0; k < 16; k ++) {
        const int64_t a = st.get_u(1);
        assert(a == 1);
    }
}

static
void
test_emulation_prevention_bytes_3()
{
    const vector<uint8_t> buf{0x00, 0x00, 0x00, 0x03, 0xff};
    vdp::RBSPState st{buf};

    for (int k = 0; k < 24; k ++) {
        const int64_t a = st.get_u(1);
        assert(a == 0);
    }

    for (int k = 0; k < 8; k ++) {
        const int64_t a = st.get_u(1);
        assert(a == 1);
    }
}

static
void
test_navigating_to_nal_element_1()
{
    const vector<uint8_t> buf{0xff, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x01, 0xa3, 0x43};

    vdp::RBSPState st{buf};

    // skip first byte
    st.get_u(8);

    const int64_t a = st.navigate_to_nal_unit();
    assert(a == 7);
}

static
void
test_navigating_to_nal_element_2()
{
    const vector<uint8_t> buf{0xff, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x01, 0xa3, 0x43};

    vdp::RBSPState st{buf};

    // skip part of the first byte
    st.get_u(3);

    const int64_t a = st.navigate_to_nal_unit();
    assert(a == 7);

    // test further data reading
    const uint32_t b = st.get_u(8);
    assert(b == 0xa3);
}

int
main()
{
    test_get_uev();
    test_get_u();
    test_get_sev();

    test_emulation_prevention_bytes_1();
    test_emulation_prevention_bytes_2();
    test_emulation_prevention_bytes_3();

    test_navigating_to_nal_element_1();
    test_navigating_to_nal_element_2();

    printf("pass\n");
}
