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
