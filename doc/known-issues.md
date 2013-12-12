Known issues
============

Flash Player is slow
--------------------

The issue consists of two: hw decoding (D in VDPAU) and hw
presentation (P in VDPAU, mostly scaling).  See below.


No hardware accelerated decoding in Flash Player
------------------------------------------------

Flash Player have hardware accelerated decoding turned off by
default. To enable, add line `EnableLinuxHWVideoDecode=1` to file
`/etc/adobe/mms.cfg`. Create that file if necessary. You must reload plugin,
easiest way to reload plugin is to restart browser.


No hardware accelerated presentation in Flash
---------------------------------------------

First, you may check whenever application uses VDPAU via libvdpau-va-gl,
by adding `ShowWatermark` to `VDPAU_QUIRKS` environment variable. That
will display "va_gl" at bottom right corner of video. If you see it,
you are fine. Otherwise, you can try user script [doc/flash-wmode.js](flash-wmode.js)
which will force wmode parameter value to be 'direct'. Here is how and why
it works.

Flash Player is an NPAPI plugin. Such plugins are separate binaries which
output is embedded in a web page by one of two different ways. Plugin can ask browser
either windowed or windowless operation. First way browser creates a
window and passes it to a plugin. Then plugin can draw on that window
when and how it wants to. Second way plugin does content display only on
browser demand by filling data buffer. VDPAU requires an X drawable
to display on, so it can be used only in windowed plugin mode.
Usually nothing can be displayed over that drawable. VDPAU will
overwrite everything else. On the other hand, browser plugins have
`wmode` parameter which controls how their content is managed by browser.
You can search for exact `wmode` semantics on the Internet. But here is
the crucial part: if `wmode` set to anything but `direct`, plugin can not
use hardware acceleration, since it forces windowless operation which
in turn prevents VDPAU usage.

Script above forces all plugin instances to have `wmode=direct`. That
solves some problems, but has own drawbacks. If web page was
desined to have something to be displayed over Flash movie, that will
become hidden. That may be subtitles, or video player controls. They
may become unusable.

If you know any better working solution for this problem, please let me know.


Flash is still slow
-------------------

Flash movies (.swf) must use StageVideo to make use of hardware acceleration.
If author for some reason have not used it, there is nothing can be done
on our side. For example, Vimeo player does use hardware decoding, but then it
downloads decoded frames back to CPU, where they scaled with their own
scaler implemented in ActionScript.


Mplayer have higher CPU usage with VDPAU than with Xv
-----------------------------------------------------

If you omit `-vc ffh264vdpau`, Mplayer will use software decoder, then
then output YCbCr images via VDPAU. At the moment YCbCr to RGB conversion
is done with help of libswscale, which can eat decent amount of CPU time.
Ensure you have hardware accelerated codecs enabled.


Mplayer shows weird errors for 10-bit H.264
---------------------------------------------

VDPAU at the moment has no support for Hi10P, so 10bit videos will fail. There is
nothing can be done in libvdpau-va-gl to fix this.
