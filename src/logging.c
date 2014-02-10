/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "eeprom.h"
#include "epoch.h"
#include "logging.h"
#include "net/tcpapi.h"
#include "net/tcpip.h"
#include "vtimer.h"
#include "lwip/udp.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char log_fmtbuf[256];
static char log_hostname[16];
static uint8_t log_facility;
static OS_MutexID log_mutex;
static serial_t *log_serial;
static struct udp_pcb *syslog_pcb;

static const char *const level_names[] = {
	"EMERG",
	"ALERT",
	"CRIT",
	"ERROR",
	"WARN",
	"NOTE",
	"INFO",
	"DEBUG"};

static void syslog_send(const char *data, uint16_t len);


void
log_start(serial_t *serial) {
	ASSERT((log_mutex = CoCreateMutex()) != E_CREATE_FAIL);
	log_serial = serial;
	log_facility = LOG_KERN;
	log_sethostname("-");
}


void
log_sethostname(const char *hostname) {
	CoEnterMutexSection(log_mutex);
	strncpy(log_hostname, hostname, sizeof(log_hostname) - 1);
	CoLeaveMutexSection(log_mutex);
}


void
log_write(int priority, const char *appname, const char *format, ...) {
	va_list ap;
	int size, used;
	uint64_t tstamp;
	int microseconds;
	struct tm tm;
	char *ptr;
	ASSERT(log_serial != NULL);
	tstamp = vtimer_now();
	CoEnterMutexSection(log_mutex);

	microseconds = (tstamp & NTP_MASK_FRAC) / NTP_TO_US;
	epoch_to_datetime(tstamp >> 32, &tm);

	/* Format syslog */
	ptr = log_fmtbuf;
	size = sizeof(log_fmtbuf);
	used = snprintf(ptr, size - 1,
			"<%d>1 %04d-%02d-%02dT%02d:%02d:%02d.%06dZ %s %s - - - ",
			priority | log_facility,
			tm.tm_year, tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			microseconds, log_hostname, appname);
	ptr += used;
	size -= used;
	va_start(ap, format);
	used = vsnprintf(ptr, size - 1, format, ap);
	va_end(ap);
	ptr += used;
	*ptr = 0;
	syslog_send(log_fmtbuf, ptr - log_fmtbuf);

	/* Format for serial terminal */
	if (!cl_enabled) {
		ptr = log_fmtbuf;
		size = sizeof(log_fmtbuf);
		used = snprintf(ptr, size - 3,
				"%04d-%02d-%02dT%02d:%02d:%02d.%06dZ %s %s ",
				tm.tm_year, tm.tm_mon, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec,
				microseconds, appname, level_names[priority]);
		ptr += used;
		size -= used;
		va_start(ap, format);
		used = vsnprintf(ptr, size - 3, format, ap);
		va_end(ap);
		ptr += used;
		*ptr++ = '\r';
		*ptr++ = '\n';
		*ptr = 0;
		serial_puts(log_serial, log_fmtbuf);
	}
	CoLeaveMutexSection(log_mutex);
}


void
syslog_start(uint32_t addr) {
	ip_addr_t ip;
	ip.addr = addr;
	syslog_pcb = udp_new();
	udp_bind(syslog_pcb, IP_ADDR_ANY, 514);
	udp_connect(syslog_pcb, &ip, 514);
}


static void
syslog_send(const char *data, uint16_t len) {
	if (!syslog_pcb || !(thisif.flags & NETIF_FLAG_UP)) {
		return;
	}
	api_udp_send(syslog_pcb, data, len);
}
