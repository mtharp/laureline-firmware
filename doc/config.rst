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

Baud rate of the GPS serial port, when :ref:`gps_ext_in` or :ref:`gps_ext_out` is true.
When :ref:`gps_ext_out` is true, this must be 57600 or greater.

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

If true, Laureline will copy raw GPS data from the internal module to the external :ref:`Data In/Out port <dataio>` at the baud rate configured by :ref:`gps_baud_rate`.
Setting :ref:`gps_baud_rate` to less than 57600 baud will cause the output to become corrupted.
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

.. _holdover_time:

holdover_time
-------------
| **Format**: integer
| **Default**: 60

In case of loss of GPS reception or GPS receiver failure, Laureline will
continue serving time normally for this many seconds after cessation of
pulse-per-second (PPS) input. After this time has elapsed, all NTP responses
will be marked with a Stratum value of 16, indicating loss of synchronization.
During holdover the status LED will flash red, and after holdover expires the
pulse LED will extinguish. It takes up to 5 seconds after loss of PPS to
transition from regular operation to holdover mode.

holdover_test
-------------
| **Format**: boolean (true or false)
| **Default**: false

If set, then Laureline will measure and log the pulse-per-second signal but
will not feed it into the main timing loop. This is useful for testing holdover
performance. Set it after the PLL has locked and observe the phase drift over
time. This setting will revert to false on powerup.

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
-----------
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

.. _ip6_manycast:

ip6_manycast
------------
| **Format**: IPv6 address (long form)
| **Default**: 0:0:0:0:0:0:0:0

If set to a non-zero value, the NTP server will listen on the specified
IPv6 multicast group for queries. Use this with the ``manycastclient`` option to
ntpd. Note that ntpd requires authentication be working in order to receive
manycast replies, see :ref:`ntp_key`.
Shortened IPv6 addresses -- those with a "::" in them -- are not parsed correctly, and must be expanded to the full 8 segments.

.. _loopstats_interval:

loopstats_interval
------------------
| **Format**: integer
| **Default**: 60

Sets the interval, in seconds, at which loop statistics will be logged to the console.

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
--------------
| **Format**: boolean (true or false)
| **Default**: false

If true, then :ref:`ntp_key` is a 40 digit hexadecimal key for use with the MD5
authentication scheme. Note that ntpd's keys file specifies MD5 keys as 20
plaintext bytes; this must be converted to 40 hexadecimal digits here.

.. _ntp_key_is_sha1:

ntp_key_is_sha1
---------------
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

.. _logging:

Logging
=======

When not in command-line mode, log messages are printed to the serial console.
These messages show changes in status as well as periodic statistical reports about the performance of the NTP server.
For example, shortly after startup and when networking is online, a line like this will appear:

    2014-07-12T02:17:16.789433Z kernel NOTE GPS NTP Server version v3.0-0-ge9f578d started

Logging can also be forwarded to a remote server using the standard `syslog`_
UDP protocol, see :ref:`syslog_ip`.

In addition to state change messages, there is also a periodic "loopstats"
report that indicates the state of the internal clock, similar to this:

    2014-07-12T03:48:00.899535Z loopstats INFO off:-1ns freq:1433ppb jit:19ns fjit:559ppt looptc:117s state:4 flags:PPS,ToD,PLL,QUANT

The meaning of these fields is as follows:

off
    Time offset between the internal clock and GPS time in nanoseconds (averaged)
freq
    Frequency offset between the oscillator and its nominal frequency in parts-per-billion
jit
    Root-mean-square (RMS) jitter of the time offset in nanoseconds
fjit
    RMS jitter of the frequency offset in parts-per-trillion
looptc
    Loop time constant of the internal clock in seconds
state
    Operating mode of the internal clock PLL.
    1 and 2 are starting up.
    3 is stable enough to count as "locked".
    4 and 5 are very stable and are the normal operating modes.
flags
    A comma-seperated list of status flags.
    Each flag is prefixed with an exclamation point "!" when it is in an error or non-asserted state.
    The ToD and PLL flags must all be asserted before the server will respond to NTP queries.
flags - PPS
    Indicates that the pulse-per-second signal is currently valid.
    It is cleared about 5 seconds after the signal ceases.
flags - ToD
    Indicates that valid time-of-day data was received from the GPS.
    This flag is only set when the UTC offset portion of the GPS almanac has been received, which can take up to 30 minutes after a cold start.
    Once it is set, it will remain set even if GPS signal is lost.
flags - PLL
    Indicates that the internal clock (phase-locked-loop) is settled and tracking UTC time closely.
flags - QUANT
    Indicates that quantization error correction was applied during the last second.
    This flag should be asserted when using the onboard GPS receiver.
    When using an external GPS, it may not be asserted depending on the receiver and how it is configured.


.. _FTDI: http://www.ftdichip.com/Drivers/VCP.htm
.. _PuTTY: http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html
.. _syslog: http://tools.ietf.org/html/rfc5424
