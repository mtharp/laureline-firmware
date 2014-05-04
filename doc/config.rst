Configuration
*************

Laureline can be configured via its USB port.
When connected to a PC it will enumerate as a serial port.
If required, drivers can be downloaded from `FTDI`_.
You will need a terminal emulator to connect to the serial port.
On Windows, PuTTY is recommended.
On Linux and BSD "screen" works well.

How to connect
==============

Parameters
----------
Any serial terminal emulator will work as long as it can connect to the USB serial port.
The general parameters are 115200 baud, 8 data bits, no parity, 1 stop bit.
This is sometimes written as ``115200 8n1``.
Make sure flow control (RTS/CTS) is disabled.

Windows - using PuTTY
----------------------
Download `PuTTY`_, either the standalone executable ``putty.exe`` (no install required), or the full installer ``putty-installer.exe``.
After starting PuTTY, a window titled "PuTTY Configuration" will appear.
In the right-side pane, click "Serial".
In the text box under "Serial line", type the name of the COM port.
You can find a list of COM ports in the Windows Device Manager, where Laureline will be named "USB Serial Port" with the COM port name following it in parentheses.
In the "Speed" text box type 115200. Click "Open".
See below for instructions on how to use the command-line.
When you are finished, close the window.

Linux/BSD - using screen
------------------------
screen can be used to interact with a serial port with minimal fuss.
To start a screen session with a serial port, or to open a new window from within an existing screen session, type::

    screen /dev/ttyUSB0 115200

Where ``/dev/ttyUSB0`` is the device that appears when you connected Laureline.
Consult ``dmesg`` or ``/var/log/messages`` or ``journalctl`` for logging that indicates which serial port was assigned.
When you are finished, type ``Ctrl-a``, then ``k`` (for "kill"), then ``y`` to confirm.

Command-line interface
======================

Normally, Laureline will print status and logging info to the serial port.
When you connect you will see a periodic output indicating the health of the GPS and timing loop, as well as possibly raw readings from the PPS input.
To configure the system or query its configuration, you must first enter command-line mode by pressing ``Enter``.
You should see a prompt, starting with a solitary ``#`` character.
You can now enter commands, followed by the ``Enter`` key.
When you are finished, type ``exit`` and press ``Enter``, or press ``Ctrl-d``, to return to the status display.

Commands
========

defaults
--------
Resets the internal EEPROM to factory defaults and reboots immediately.

exit
----
Leaves command-line mode and returns to status mode.

help
----
Lists available command names with a short description.

.. _info:

info
----
Lists system information including hardware and software version, serial
number, MAC address, LAN status, IP address, and system uptime.

.. _save:

save
----
Saves changes made by the :ref:`set` command to internal EEPROM and reboots immediately.
Until saved, setting changes have no effect.

.. _set:

set
---
Displays and modifies settings.
With no arguments, it displays the available settings with their current value.
Otherwise it takes the form ``set param = value``.
Until saved with the :ref:`save` command, changes have no effect.

uptime
------
Displays the elapsed time since the system was powered on.

version
-------
Displays hardware, software, and bootloader version information.

Settings
========
Settings are viewed and modified with the :ref:`set` command.
Setting changes must be saved with the :ref:`save` command before they will have any effect.
Saving causes the system to reboot, which will cause a 2 minute period where NTP service is not available while the PLL settles.

admin_key
---------
| **Format**: 16 hexadecimal digits
| **Default**: 0

Currently unimplemented, this feature may be added at a later date.
Used to authenticate remote configuration access.
You must supply the same key to the configuration client tool in order to discover and configure this server remotely.
If set to all zeroes (the default), remote access is not possible.

.. _gps_baud_rate:

gps_baud_rate
-------------
| **Format**: integer
| **Default**: 0

Baud rate of the GPS serial port.
If using the internal GPS, leave this at the default of 0.
Set it to a non-zero value only in combination with :ref:`gps_ext_in`.

.. _gps_ext_in:

gps_ext_in
----------
| **Format**: boolean (true or false)
| **Default**: false

If true, instead of using the builtin GPS module Laureline will receive GPS data and pulse-per-second from the :ref:`Data In/Out port <dataio>`.
In this mode, the PPS connector is used as an input.
Use :ref:`gps_baud_rate` to configure the baud rate.
At present, supported protocols include NMEA, u-blox UBX, Trimble TSIP, and Motorola Oncore.
However, only UBX is well-tested and many receivers require special configuration to output all of the data required. Use at your own risk.
Not compatible with the :ref:`gps_ext_out` or :ref:`pps_out` settings.

.. _gps_ext_out:

gps_ext_out
-----------
| **Format**: boolean (true or false)
| **Default**: false

If true, Laureline will copy raw GPS data from the internal module to the external :ref:`Data In/Out port <dataio>` at 57600 baud.
Do not change :ref:`gps_baud_rate` as it affects the internal serial port as well and will prevent the GPS from functioning correctly.
May be used in combination with :ref:`pps_out`. Not compatible with :ref:`gps_ext_in`.

gps_listen_port
---------------
| **Format**: integer
| **Default**: 0

If set to a non-zero value, Laureline will listen for a TCP connection at this port.
The client that connects can then receive raw data from the GPS module, and can transmit raw packets to the GPS as well.
This feature is experimental and may cause instability or lock-ups.
Even when working correctly it is a security risk if exposed to an untrusted network (i.e. the internet).
Use at your own risk.

.. _ip_addr:

ip_addr
-------
| **Format**: IP address
| **Default**: 0.0.0.0

If set to a non-zero value, Laureline will use this as its IP address.
If set to zero (the default), Laureline will use DHCP to acquire an IP address automatically.
If non-zero, :ref:`ip_netmask` must also be set and :ref:`ip_gateway` should usually be set.

.. _ip_gateway:

ip_gateway
----------
| **Format**: IP address
| **Default**: 0.0.0.0

If :ref:`ip_addr` is set, this should be set to the IP address of the network gateway router.
This is not mandatory, but if not set then computers outside of the local network will not receive responses to NTP queries.
If you are not sure what your network gateway is, use the ``ipconfig`` command on your PC.
If using DHCP this must be set to zero.

.. _ip_manycast:

ip_manycast
----------
| **Format**: IP address
| **Default**: 0.0.0.0

If set to a non-zero value, the NTP server will listen on the specified
multicast group for queries. Use this with the ``manycastclient`` option to
ntpd. Note that ntpd requires authentication be working in order to receive
manycast replies, see :ref:`ntp_key`.

.. _ip_netmask:

ip_netmask
----------
| **Format**: IP address
| **Default**: 0.0.0.0

If :ref:`ip_addr` is set, this must be set to the associated network mask (subnet mask).
The network mask is used to determine whether a given remote IP address is on the same LAN or not.
If you are not sure what your network mask is, use the ``ipconfig`` command on your PC.
If using DHCP this must be set to zero.

.. _ntp_key:

ntp_key
-------
| **Format**: 40 hexadecimal digits
| **Default**: 0

This key is used to validate incoming client queries and to sign outgoing
responses.
One of :ref:`ntp_key_is_md5` or :ref:`ntp_key_is_sha1` must be set in order to
select the key type.
The key must be the raw, 40 hexadecimal digits.
MD5 keys must be converted to hex.
Key IDs do not need to be specified because the server will reply with the same
ID that the client specified if query authentication succeeds.
If the query is not authenticated then the response will also be
unauthenticated.

.. _ntp_key_is_md5:

ntp_key_is_md5
-------
| **Format**: boolean (true or false)
| **Default**: false

If true, then :ref:`ntp_key` is a 40 digit hexadecimal key for use with the MD5
authentication scheme. Note that usually ntpd's keys file specifies MD5 keys as
20 plaintext bytes; this must be converted to 40 hexadecimal digits here.

.. _ntp_key_is_sha1:

ntp_key_is_sha1
-------
| **Format**: boolean (true or false)
| **Default**: false

If true, then :ref:`ntp_key` is a 40 digit hexadecimal key for use with the
SHA1 authentication scheme.

.. _pps_out:

pps_out
-------
| **Format**: boolean (true or false)
| **Default**: false

If true, the PPS pin on the :ref:`Data In/Out connector <dataio>` will output a pulse-per-second signal.
See the pinout description under Connectors for more electrical info.
If false (the default), the PPS pin does not output a signal.
Do not set this to true unless you are sure only compatible equipment is connected to the Data In/Out port.
This setting is not compatible with the :ref:`gps_ext_in` setting.

.. _syslog_ip:

syslog_ip
---------
| **Format**: IP address
| **Default**: 0.0.0.0

If non-zero, Laureline will forward logging data in the `syslog`_ format to the specified IP address.
Log lines will be sent in plain UDP format to port 514.


Hardware Jumpers
================

If you wish to use Laureline with a 3.3V GPS antenna and are comfortable using
a soldering iron, there is a hardware jumper on the PCB that can be changed to
adjust the antenna voltage.
Performing this modification as described will not void your warranty.

#. Open the enclosure by removing both screws on one end of the chassis.
   Remove the end panel and slide out the PCB.
#. Look at the bottom of the PCB near the antenna connector.
   Look for the "J2" designator with the label "ANT PWR".
#. Using a scalpel or hobby knife, carefully slice the copper track between the
   center pad and the pad labeled "+5.0v".
#. Inspect with a jeweler's loupe or microscope to ensure there is no copper
   connecting the pads.
   Optionally, apply power to the board and use a multimeter to confirm that no
   voltage is present on the center pad.
#. Using a soldering iron, apply a blob of solder between the center pad and
   the pad labeled "+3.3v".
#. Before reassembling the enclosure, apply power to the USB connector and use
   a multimeter to check the voltage present on the SMA antenna connector.
#. Reassemble the enclosure by checking that the board is the right way up and
   sliding it into the bottom-most channel, adding the end panel, and screwing
   it into place. Do not over-tighten.

.. _FTDI: http://www.ftdichip.com/Drivers/VCP.htm
.. _PuTTY: http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html
.. _syslog: http://tools.ietf.org/html/rfc5424
