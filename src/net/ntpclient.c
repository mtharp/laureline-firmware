/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "logging.h"
#include "lwip/udp.h"
#include "ppscapture.h"
#include "status.h"
#include "vtimer.h"
#include "net/ntpclient.h"
#include "net/ntpserver.h"
#include "net/tcpapi.h"
#include <stdio.h>
#include <string.h>

#define NTPCLIENT_STACK 512
OS_STK ntpclient_stack[NTPCLIENT_STACK];
OS_TID ntpclient_tid;

#define MAX_SERVERS_LOG2 2
#define MAX_SERVERS (1<<(MAX_SERVERS_LOG2))
static ip_addr_t ntp_servers[MAX_SERVERS];
const char *ntp_hosts = "%d.pool.ntp.org";

struct udp_pcb *ntp_cli_pcb;
static uint8_t query_buf[48];

// XXX FIXME
//#define QUERY_INTERVAL 12 /* 4096 seconds, 1.13 hours */
//#define RETRY_INTERVAL 6 /* 64 seconds */
#define I2ST(x) S2ST((1<<(x)))

#define QUERY_INTERVAL 2
#define RETRY_INTERVAL 2


static uint64_t
ntp_query(int index) {
	uint64_t query_time, result;
	uint16_t len;
	memset(query_buf, 0, 48);
	query_buf[0] = VN_4 | MODE_CLIENT;
	query_buf[2] = QUERY_INTERVAL;
	api_udp_connect(ntp_cli_pcb, &ntp_servers[index], NTP_PORT);
	query_time = monotonic_now();
	api_udp_send(ntp_cli_pcb, query_buf, 48);

	len = sizeof(query_buf);
	if (ERR_OK == api_udp_recv(ntp_cli_pcb, query_buf, &len) && len >= 48) {
		query_time = monotonic_now() - query_time; /* RTT */
		result =  ((uint64_t)query_buf[40] << 56)
				| ((uint64_t)query_buf[41] << 48)
				| ((uint64_t)query_buf[42] << 40)
				| ((uint64_t)query_buf[43] << 32)
				| ((uint64_t)query_buf[44] << 24)
				| ((uint64_t)query_buf[45] << 16)
				| ((uint64_t)query_buf[46] <<  8)
				| ((uint64_t)query_buf[47]);
		log_write(LOG_INFO, "ntpclient", "ntp response: rtt=%d hi=%d lo=%d",
				(int)query_time, (int)(result >> 32), (int)result);
	} else {
		log_write(LOG_INFO, "ntpclient", "ntp recv failed");
	}
	return 0;
}


static void
ntpclient_thread(void *p) {
	int i, num_responses;
	char hostname[DNS_MAX_NAME_LENGTH+1];
	uint64_t accum, result;
	hostname[DNS_MAX_NAME_LENGTH] = 0;
	while (1) {
		for (i = 0; i < MAX_SERVERS; i++) {
			snprintf(hostname, DNS_MAX_NAME_LENGTH, ntp_hosts, i);
			if (ERR_OK != api_gethostbyname(hostname, &ntp_servers[i])) {
				ntp_servers[i].addr = 0;
			}
		}

		ntp_servers[0].addr = 0x060018ac;
		ntp_servers[1].addr = 0;
		ntp_servers[2].addr = 0;
		ntp_servers[3].addr = 0;

		num_responses = 0;
		for (i = 0; i < MAX_SERVERS; i++) {
			if (ntp_servers[i].addr == 0) { continue;
			}
			result = ntp_query(i);
			if (result != 0) {
				num_responses++;
				accum += result >> MAX_SERVERS_LOG2;
			}
		}
		if (num_responses == 0) {
			CoTickDelay(I2ST(RETRY_INTERVAL));
			continue;
		}
		accum /= num_responses;
		accum <<= MAX_SERVERS_LOG2;
		CoTickDelay(I2ST(QUERY_INTERVAL));
	}
}

void
ntp_client_start(void) {
	ntpclient_tid = CoCreateTask(ntpclient_thread, NULL, THREAD_PRIO_NTPCLIENT,
			&ntpclient_stack[NTPCLIENT_STACK-1], NTPCLIENT_STACK, "ntpclient");
	ASSERT(ntpclient_tid != E_CREATE_FAIL);
	ntp_cli_pcb = udp_new();
	udp_bind(ntp_cli_pcb, IP_ADDR_ANY, 0);
}
