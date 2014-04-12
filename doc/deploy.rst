Recommended Deployments
***********************

Laureline is a standalone NTP server that uses GPS as its only time source.
It does not reference or fall back to other network time sources.
This means that in case of a disruption of GPS reception, hardware failure, or global thermonuclear war, Laureline will not be able to answer NTP queries.
If this would cause hardship for you, you probably do not want a single Laureline server to be the solitary NTP source for your clients.

For low-risk deployments such as a home network or a small business without
stringent redundancy requirements, it is sufficient to simply keep the existing
NTP server pool in your clients.
As long as Laureline is healthy clients will invariably prefer it as a time
source due to the ultra-low latency and jitter of a LAN time source.
But in case of a problem, clients can fallback to the internet pool.

If you have dozens of clients you probably do not want them all polling internet time sources, especially when that data will be decimated during normal operation due to the strenth of Laureline's data.
Consider deploying ntpd on two or more general-purpose servers to act as a Stratum 2 intermediate server.
Give these servers a normal pool of internet time servers, plus your Laureline server or servers.
As before they will prefer Laureline as long as it is available.
You can also configure them to peer with each other for additional confidence.
Then configure your clients to the Stratum 2 servers.
In case of a GPS outage, the Stratum 2 servers will fall back to internet time sources and clients will not be disrupted.
This is a good configuration if you have important SNTP clients that do not support a full ntpd implementation and cannot query multiple servers.
For full ntpd clients, you can continue to point them at the Laureline server(s) as well if you need the lowest possible jitter.
