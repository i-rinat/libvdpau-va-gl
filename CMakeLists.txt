project (fake-vdpau)

cmake_minimum_required (VERSION 2.6)

add_library (vdpau_fake SHARED
	vdpau_fake.c
)

set_target_properties (vdpau_fake
	PROPERTIES
	SOVERSION 1
)
