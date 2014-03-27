Troubleshooting
***************

If you are having difficulty using Laureline, the following may be of some use.
Don't hesitate to contact us with questions.

Bottom status LED stays solid red
    The GPS is not tracking satellites and is thus not outputting a
    pulse-per-second signal. Check that the antenna is firmly connected, that
    it is compatible with the 5V power Laureline supplies, and that it has a
    clear view of the sky.

Bottom status LED flashes green but top LED is orange and NTP does not respond
    The GPS is not sending valid UTC time-of-day data. Before it can do so it
    must receive a complete almanac from the GPS satellites. This process can
    take up to 30 minutes.

NTP reports "stratum 16", "Server dropped: no data", or "no server suitable for synchronization found"
    The GPS receiver has lost satellite lock and is not outputting a
    pulse-per-second signal. Check that the antenna connection is secure and
    that it has a clear view of the sky.

Status LEDs and Ethernet LEDs do not illuminate or are dim
    Disconnect the GPS antenna and see if the problem persists.
    If it is resolved then your antenna is drawing too much power.
    If it is not due to a fault then you may need to use a DC block and
    external power source.

    If you have recently flashed a firmware upgrade, the firmware may be
    corrupt. Try re-downloading the firmware.
