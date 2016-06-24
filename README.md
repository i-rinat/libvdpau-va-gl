About
=====

Briefly, this is the [VDPAU](http://en.wikipedia.org/wiki/VDPAU) driver with
[VA-API](http://en.wikipedia.org/wiki/Video_Acceleration_API)/OpenGL backend.

There are applications exists that can use VDPAU. Amongst them are Adobe Flash Player
and Mplayer. They both can use VDPAU, but since there is no VDPAU available on Intel
chips, they fall back to different drawing techniques. And while Mplayer can use
XVideo extension to offload scaling to GPU, Flash Player can not and does all
scaling in software. If there was VDPAU available, CPU usage could be significantly
lower.

VDPAU is not vendor-locked technology. Even official documentation mentions
possibility of other drivers. They should be named as `libvdpau_drivername.so.1` and
placed where linker could find them. `/usr/lib` usually works fine.
Which driver to use is determined by asking X server about current driver name or by
using `VDPAU_DRIVER` environment variable.

Here is one. Named libvdpau_va_gl.so.1, it uses OpenGL under the hood to
accelerate drawing and scaling and VA-API (if available) to accelerate video
decoding. For now VA-API is available on some Intel chips, and on some AMD video
adapters with help of [xvba-va-driver](http://cgit.freedesktop.org/vaapi/xvba-driver/).
OpenGL is available, you know, on systems with OpenGL available.


Install
=======
   1. `sudo apt-get install cmake libva-dev libgl1-mesa-dev`
   2. `mkdir build; cd build`
   3. `cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..`
   4. `sudo make install`
   5. Add `VDPAU_DRIVER=va_gl` to your environment

Commands above should work for any Debian-based distro. Fedora names packages in a
different way, so package installation step will look like:
`sudo yum install cmake libva-devel mesa-libGL-devel`.

Run time configuration
======================
Besides `VDPAU_DRIVER` variable which selects which driver to use there are other
variables that control runtime behavior of va_gl driver.

`VDPAU_QUIRKS` contains comma-separated list of enabled quirks. Here is the list:

   * `XCloseDisplay`	Disables calling of XCloseDisplay which may segfault on some video drivers
   * `ShowWatermark`	Enables displaying string "va_gl" in bottom-right corner of window
   * `AvoidVA`          Makes libvdpau-va-gl NOT use VA-API

Parameters of VDPAU_QUIRKS are case-insensetive.

Copying
=======
libvdpau-va-gl is distributed under the terms of the MIT license. See
LICENSE file for details.

Contact
=======
Author can be reached at email
`ibragimovrinat-at-mail.ru` or at github: https://github.com/i-rinat/
