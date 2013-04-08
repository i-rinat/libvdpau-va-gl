About
=====

VAAPI/OpenGL backend for VDPAU. Work in progress.


Install
=======
   1. `sudo apt-get install libvdpau-dev libva-dev libglib2.0-dev libswscale-dev libgl1-mesa-dev libglu1-mesa-dev`
   2. `mkdir build; cd build`
   3. `cmake -DCMAKE_BUILD_TYPE=Release ..`
   4. `sudo make install`
   5. Add `VDPAU_DRIVER=va_gl` to your environment


Copying
=======
libvdpau-va-gl is distributed under the terms of the LGPLv3. See files
COPYING, COPYING.GPLv3, and COPYING.LGPLv3 for details.
