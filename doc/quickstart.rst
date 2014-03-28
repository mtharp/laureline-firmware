Quick Start
***********

To get started with Laureline you will need:

* Laureline GPS NTP server
* GPS antenna with SMA connector and supporting 5V power
* USB A-to-B cable
* Either a PC with a USB port, or a USB charger
* Ethernet cable and a switch, hub, router to connect it to

Steps
=====

For an illustration of where the following items are located see :doc:`connectors`.

#. Connect the GPS antenna to Laureline's antenna port. The connection should be firm but not more than finger-tight. Position the antenna so that it has a clear view of the sky.
#. Connect the LAN port to a switch, hub, router, or PC where you want to serve time. Connecting to a DHCP-enabled network will get things up and running more quickly as no manual configuration is required. If you need to configure Laureline with a static IP, see the :ref:`ip_addr<ip_addr>` setting under Configuration.
#. Finally, connect the Power/USB port to a PC or USB charger with the USB cable.
#. The status indicators will illuminate steady red at first. The LEDs on the LAN port will also illuminate.
#. After about two minutes, the bottom indicator (PPS) will start flashing red once per second. After another two minutes it will start flashing green. If it does not start flashing after several minutes check the antenna connection and ensure that the antenna has a clear view of the sky.
#. The top indicator (Health) will change to orange when the PPS indicator starts flashing. At this point, the GPS receiver is slowly receiving almanac data from the GPS satellites. After a period of up to 30 minutes this process will complete and the top indicator will change to green.
#. When both indicators are green (steady green on top, flashing green on bottom), Laureline is operating normally and is ready to serve NTP responses.
#. If you are not using a static IP, you will need to discover the IP of the Laureline server. This can be retrieved from the logs of your DHCP server. If your DHCP server is a typical home router, the web interface usually has a list of devices on your network. You can also learn the IP using Laureline's command-line interface, see :ref:`the info command <info>` under Configuration.
#. Once both indicators are green, perform a test query using a NTP client of your choice. For example, from a Linux command-line::

    $ ntpdate -d 192.168.1.200
    26 Mar 20:03:25 ntpdate[641]: ntpdate 4.2.6p5@1.2349-o Mon Dec  9 16:28:27 UTC 2013 (1)
    Looking for host 192.168.1.200 and service ntp
    host found : 192.168.1.200
    transmit(192.168.1.200)
    receive(192.168.1.200)
    transmit(192.168.1.200)
    receive(192.168.1.200)
    transmit(192.168.1.200)
    receive(192.168.1.200)
    transmit(192.168.1.200)
    receive(192.168.1.200)
    server 192.168.1.200, port 123
    stratum 1, precision -24, leap 00, trust 000
    refid [GPS], delay 0.02580, dispersion 0.00002
    transmitted 4, in filter 4
    reference time:    d6dde953.9dbf233a  Wed, Mar 26 2014 20:03:31.616
    originate timestamp: d6dde953.9dbf233a  Wed, Mar 26 2014 20:03:31.616
    transmit timestamp:  d6dde953.9e66ea78  Wed, Mar 26 2014 20:03:31.618
    filter delay:  0.02580  0.02585  0.02583  0.02583 
             0.00000  0.00000  0.00000  0.00000 
    filter offset: -0.00262 -0.00265 -0.00267 -0.00267
             0.000000 0.000000 0.000000 0.000000
    delay 0.02580, dispersion 0.00002
    offset -0.002629

    26 Mar 20:03:31 ntpdate[641]: adjust time server 192.168.1.200 offset -0.002629 sec

Where ``192.168.1.200`` is the IP of the Laureline server. A successful response will indicate ``stratum 1`` and the offset to your local clock.

Finally, add Laureline's IP as a source for your NTP daemon(s). If you connected Laureline to a PC, you can move it to an independent power source now that configuration is complete, or you can leave it connected to a PC.
