Firmware Updates
****************

Laureline can be updated in the field by way of a MicroSD card.

To perform a firmware upgrade:

#. Download `ll.hex` from the `Laureline releases page`_.
#. Format a MicroSD card with a FAT12, FAT16, or FAT32 filesystem. exFAT is not supported.
#. Place the firmware in the root of the MicroSD card and rename it if needed to ``ll.hex``
#. Safely eject the MicroSD card and insert it face-up into the slot on Laureline.
#. Cycle power to Laureline or use the :ref:`save` command to soft-reset the device.
#. Wait for the status LEDs to illuminate. If you are monitoring the command-line interface it will report progress as well.
#. You may now remove the MicroSD card.

Firmware Changelog
==================

Version 4.1
-----------
* Fixed DHCP leases not renewing as often as they should (`#6`_).

Version 4.0
-----------
* Added IPv6 support. Only stateless autoconfiguration is supported; static IPs
  will be added later. The :ref:`ip6_manycast` option can be used to join an
  IPv6 multicast group.
* Added detailed loopstats logging with additional jitter and time constant
  data. Reports are emitted periodically, by default every 60 seconds but
  adjustable using the :ref:`loopstats_interval` option. See :ref:`logging` for
  details.
* Added a :ref:`timescale_gps` option to select the GPS timescale instead of UTC.
* The :ref:`gps_baud_rate` option can now be used in combination with
  :ref:`gps_ext_out` to output data at a faster rate than the internal 57600
  baud. However, it may not be set to less than 57600 when used as an output.
* Fixed log messages being sent that had the "time-of-day OK" status flag set
  before the time-of-day was actually applied (`#2`_).
* Fixed a stability issue that could cause a spontaneous reset, especially
  under extremely high network load.
* Now based on the FreeRTOS kernel; see :doc:`resources`.

Version 3.0
-----------
* Added configurable holdover time. This allows NTP to keep running for a
  period of time after GPS signal is lost. See :ref:`holdover_time`.
* Added optional NTP auth using MD5 or SHA-1 digests. See :ref:`ntp_key`.
* Added NTP "manycast" server support. NTP clients can discover manycast
  servers on the LAN by sending a query to a multicast group and automatically
  establishing relationships with servers that respond. See :ref:`ip_manycast`.
  Note that ntpd clients require authentication in order to use this feature.

Version 2.4
-----------
* Tweaked the monotonic timer's input capture algorithm. The new implementation
  performs identically but is easier to understand.

.. _Laureline releases page: https://github.com/mtharp/laureline-firmware/releases
.. _#2: https://github.com/mtharp/laureline-firmware/issues/2
.. _#6: https://github.com/mtharp/laureline-firmware/issues/6
