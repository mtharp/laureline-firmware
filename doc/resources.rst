Resources
*********

Buy Laureline
=============
You can buy the Laureline GPS NTP server at `Tindie`_

Firmware download
=================
Download the latest firmware here: https://github.com/mtharp/laureline-firmware/releases

See the :doc:`updates` chapter for instructions on uploading new firmware.

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

| This project uses the `SCons`_ build system. It is available in most Linux distributions; just type "scons" to get started.

Acknowledgments
================
Laureline includes and links against the following third-party software:

* `FreeRTOS`_ real-time operating system (`Modified GPL license`_)
* `lwIP`_ - TCP/IP stack (BSD License)
* `ChaN's FatFs`_ - FAT filesystem (permissively licensed)
* `NTPns`_ by Poul-Henning Kamp - PLL math (Beer-ware License)
* `OpenSSL`_ - message digest functions (permissively licensed)

.. _Tindie: https://www.tindie.com/products/gxti/laureline-gps-ntp-server/
.. _FreeRTOS: http://www.freertos.org/
.. _Modified GPL license: http://www.freertos.org/license.txt
.. _lwIP: https://savannah.nongnu.org/projects/lwip/
.. _ChaN's FatFs: http://elm-chan.org/fsw/ff/00index_e.html
.. _NTPns: http://phk.freebsd.dk/phkrel/
.. _OpenSSL: http://www.openssl.org/
.. _SCons: http://scons.org/
