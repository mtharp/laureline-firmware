Resources
*********

Buy Laureline
=============
You can buy the Laureline GPS NTP server at `Tindie`_

Firmware download
=================
Download the latest firmware here: http://partiallystapled.com/pub/laureline/

For hardware revisions 6 and 7, use software version 3.
Look for files starting with ``laureline_v3_`` and ending in ``.hex``. Don't
forget to rename to ``ll.hex`` when writing to the MicroSD card (two lowercase 'L's).

Source code and Design files
============================
| Software, and the source for the documentation you are currently reading, is published on GitHub:
| https://github.com/mtharp/laureline-firmware

| Hardware design files are maintained in a separate Mercurial repository:
| http://hg.partiallystapled.com/circuits/laureline/

| If you just want a ZIP file to download, you can find them here:
| Software: https://github.com/mtharp/laureline-firmware/archive/master.zip
| Hardware: http://hg.partiallystapled.com/circuits/laureline/archive/tip.zip

| If you are not sure where to start with building the software, we highly recommend the *GCC ARM Embedded toolchain*, which is available in binary form for Windows, Linux, and Mac OS X:
| https://launchpad.net/gcc-arm-embedded/+download

Acknowledgments
================
Laureline includes and links against the following third-party software:

* `CooCox CoOS`_ real-time operating system (BSD License)
* `lwIP`_ - TCP/IP stack (BSD License)
* `ChaN's FatFs`_ - FAT filesystem (permissively licensed)

.. _Tindie: https://www.tindie.com/products/gxti/laureline-gps-ntp-server/
.. _CooCox CoOS: http://www.coocox.org/CoOS.htm
.. _lwIP: https://savannah.nongnu.org/projects/lwip/
.. _ChaN's FatFs: http://elm-chan.org/fsw/ff/00index_e.html
