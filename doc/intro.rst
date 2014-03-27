Introduction
************

The Laureline GPS NTP Server is a small, performant, and low-power device that
serves time to network clients from a built-in GPS receiver. Given only a GPS
antenna, a power source, and a clear view of the sky it can track UTC to within
200 nanoseconds and make precise time available to your entire LAN, WAN, or to
the Internet. Laureline interoperates seamlessly with any NTP or SNTP client
and can sustain thousands of queries per second. Even under high throughput
timekeeping operations are never disrupted or perturbed.

Laureline is Open Source Hardware and Software. Hardware design files and user
documentation are provided under the Creative Commons Attribution 3.0 License.
Software source code is provided under the MIT license.

Specifications
==============

* Performance

  * 5000 NTP queries per second
  * 200 µs round-trip latency
  * Locked to UTC to ±200ns < 75ns typical

* Power Supply

  * Voltage: 4.0-6.0VDC
  * Consumption: 150mA (0.75W) - not including antenna (typical < 1W)
  * Connector: USB Type B receptacle

* Network

  * 100Base-TX Ethernet
  * Full- or half-duplex
  * Auto-MDIX (auto-crossover)
  * Dedicated Ethernet MAC with minimal jitter and latency

* GPS

  * u-blox GPS receiver module
  * Sensitivity: -161 dBm (tracking)
  * PPS accuracy: 30ns RMS
  * Antenna: SMA receptacle; 5V DC power provided

* NTP and Timing

  * Supports normal queries from NTP and SNTP clients
  * Input capture resolution of 15ns
  * Onboard TCXO
  * Software PLL keeps time in native NTP units

* Other features

  * Command-line configuration via USB
  * External GPS data and PPS input, 3.3V and 5V compatible
  * External GPS data and PPS output, 3.3V
  * Field upgrade of software via MicroSD card
