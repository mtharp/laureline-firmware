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


Known Issues
============

Leap seconds
------------
Laureline does not currently set the leap second bit in NTP replies.
This is because the Laureline software itself does not support leap seconds.
However, the GPS receiver does.
When a leap second occurs Laureline will immediately step its own internal
clock in response to the time-of-day data from the GPS and continue serving the
correct time.
But because the leap bit was not set in any NTP replies, clients that do not
receive NTP from any other time source will not step in unison and will be
"left behind".
Manual intervention will be required e.g. by running ntpdate to step the clock,
because ntpd will reject any time source that has a large offset.

If any other NTP servers are in use that set the leap bit, or if leap second
data is manually loaded into the client, then the client will step in unison
and continue operating correctly.
SNTP clients such as routers and switches are typically not affected as they
simply periodically set their local clock to whatever the server replies.
Their clock will be off by one second until the next check, then will be
correct.
