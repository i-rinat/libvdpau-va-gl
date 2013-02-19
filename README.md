About
=====

OpenGL/software/VAAPI(?) backend for VDPAU. Work in progress.


Install
=======
   1. `sudo apt-get install libvdpau-dev libglib2.0-dev libswscale-dev libgl1-mesa-dev libglu1-mesa-dev`
   2. `mkdir build; cd build`
   3. `cmake -DCMAKE_BUILD_TYPE=Release ..`
   4. `sudo make install`
   5. Add `VDPAU_DRIVER=fake` to your environment
