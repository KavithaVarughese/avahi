/***
  This file is part of avahi.

  avahi is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  avahi is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
  Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with avahi; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h> 
#include <sys/types.h> 
#include <unistd.h> 

#include <avahi-common/timeval.h>
#include <avahi-common/malloc.h>

#include "response-sched_copy.h"
#include "log.h"
#include "rr-util.h"

/* Local packets are supressed this long after sending them */
#define AVAHI_RESPONSE_HISTORY_MSEC 500

/* Local packets are deferred this long before sending them */
#define AVAHI_RESPONSE_DEFER_MSEC 20

/* Additional jitter for deferred packets */
#define AVAHI_RESPONSE_JITTER_MSEC 100

/* Remote packets can suppress local traffic as long as this value */
#define AVAHI_RESPONSE_SUPPRESS_MSEC 700

typedef struct AvahiResponseJob AvahiResponseJob;

typedef enum {
    AVAHI_SCHEDULED,
    AVAHI_DONE,
    AVAHI_SUPPRESSED
} AvahiResponseJobState;

struct AvahiResponseJob {
    AvahiResponseScheduler *scheduler;
    AvahiTimeEvent *time_event;

    AvahiResponseJobState state;
    struct timeval delivery;

    AvahiRecord *record;
    int flush_cache;
    AvahiAddress querier;
    int querier_valid;

    AVAHI_LLIST_FIELDS(AvahiResponseJob, jobs);
};

struct AvahiResponseScheduler {
    AvahiInterface *interface;
    AvahiTimeEventQueue *time_event_queue;

    AVAHI_LLIST_HEAD(AvahiResponseJob, jobs);
    AVAHI_LLIST_HEAD(AvahiResponseJob, history);
    AVAHI_LLIST_HEAD(AvahiResponseJob, suppressed);
};

static void job_mark_done_copy(AvahiResponseScheduler *s, AvahiResponseJob *rj) {
    assert(s);
    assert(rj);

    assert(rj->state == AVAHI_SCHEDULED);

    AVAHI_LLIST_NEXT(AvahiResponseJob, jobs, rj);
}

static int packet_add_response_job_copy(AvahiResponseScheduler *s, AvahiDnsPacket *p, AvahiResponseJob *rj) {
    assert(s);
    assert(p);
    assert(rj);

    /* Try to add this record to the packet */
    if (!avahi_dns_packet_append_record(p, rj->record, rj->flush_cache, 0))
        return 0;

    job_mark_done(s, rj);

    return 1;
}

static void send_response_packet_copy(AvahiResponseScheduler *s, void *username) {
    AvahiResponseJob *rj = username;
    AvahiDnsPacket *p;
    unsigned n;

    assert(s);
    assert(rj);

    AvahiResponseJob *rj_copy = rj;
    AvahiResponseJob *llcopy = s->jobs;
    if (!(p = avahi_dns_packet_new_response(s->interface->hardware->mtu, 1)))
        return; /* OOM */
    n = 1;

    /* Put it in the packet. */
    if (packet_add_response_job(s, p, rj_copy)) {

        /* Try to fill up packet with more responses, if available */
        while (llcopy) {

            if (!packet_add_response_job(s, p, llcopy))
                break;

            n++;
        }

    } else {
        size_t size;

        avahi_dns_packet_free(p);

        /* OK, the packet was too small, so create one that fits */
        size = avahi_record_get_estimate_size(rj->record) + AVAHI_DNS_PACKET_HEADER_SIZE;

        if (!(p = avahi_dns_packet_new_response(size + AVAHI_DNS_PACKET_EXTRA_SIZE, 1)))
            return; /* OOM */

        if (!packet_add_response_job(s, p, rj_copy)) {
            avahi_dns_packet_free(p);

            avahi_log_warn("Record too large, cannot send");
            job_mark_done(s, rj_copy);
            return;
        }
    }

    avahi_dns_packet_set_field(p, AVAHI_DNS_FIELD_ANCOUNT, n);
    avahi_hexdump_file(AVAHI_DNS_PACKET_DATA(p), p->size);
    avahi_dns_packet_free(p);
}
