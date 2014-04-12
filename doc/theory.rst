Theory of Operation
*******************

Laureline's main hardware elements are a STM32F1 main processor, a NEO-6M GPS
receiver, a temperature-compensated crystal oscillator (TCXO), and a dedicated
Ethernet transceiver (PHY).
The main processor is clocked by an internal PLL which is fed from the TCXO,
ensuring a stable time-base.

A dedicated hardware timer-counter in the main processor is used to capture
pulse-per-second events from the GPS, which have a rising edge coinciding with
the start of each UTC or GPS second.
This rising edge is captured and timestamped against an internal "monotonic"
timebase which runs at a steady 65-70MHz (depending on the TCXO used).

Concurrently, a virtual timebase ("vtimer") ticks at an adjustable rate
relative to the fixed monotonic timebase.
Each second, the elapsed number of monotonic ticks since the previous
adjustment is divided by the adjusted rate output from the phase-locked loop in
order to get the true time elapsed since the previous adjustment.
This true time is added to an accumulator to track what the current UTC time
is.
The accumulated vtimer and monotonic values serve as a base for further UTC
time calculations until the next adjustment 1 second later.

Each time a NTP query is performed, the number of monotonic ticks since the
last vtimer adjustment is sampled and divided by the PLL output rate.
This is added to the accumulator "base" value to get the final UTC time.
The vtimer accumulator is in NTP's native timestamp format, so the calculated
time is simply copied into the response packet directly and no further
precision is lost due to conversion.
The Ethernet stack is implemented using dedicated hardware MAC and PHY and is
interrupt-driven and DMA-enabled so minimal latency is incurred during a query.

Finally, each second the captured PPS value is compared against the vtimer's
calculated start-of-second.
The difference between measured and expected value is fed into a software PLL
which then produces an adjustment which feeds back into the vtimer to correct
its rate of advancement.
