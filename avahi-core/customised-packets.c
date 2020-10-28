
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include "customised-packets.h"
#include "query-sched.h"
#include "log.h"



struct AvahiQueryJob {
    unsigned id;
    int n_posted;

    AvahiQueryScheduler *scheduler;
    AvahiTimeEvent *time_event;


    int done;
    struct timeval delivery;

    AvahiKey *key;

    AVAHI_LLIST_FIELDS(AvahiQueryJob, jobs);
};

struct AvahiKnownAnswer {
    AvahiQueryScheduler *scheduler;
    AvahiRecord *record;

    AVAHI_LLIST_FIELDS(AvahiKnownAnswer, known_answer);
};

struct AvahiQueryScheduler {
    AvahiInterface *interface;
    AvahiTimeEventQueue *time_event_queue;

    unsigned next_id;

    AVAHI_LLIST_HEAD(AvahiQueryJob, jobs);
    AVAHI_LLIST_HEAD(AvahiQueryJob, history);
    AVAHI_LLIST_HEAD(AvahiKnownAnswer, known_answers);
};

char* csv_get_field(char* line, int num)
{
    char* tok;
    for (tok = strtok(line, ";");
            tok && *tok;
            tok = strtok(NULL, ";\n"))
    {
        if (!--num)
            return tok;
    }
    return NULL;
}

void customised_query_packets(AvahiQueryJob *qj, AvahiQueryScheduler *s){
	
	FILE *fp = fopen("browse_service.csv","r");
    char line[1024];

    AvahiDnsPacket *p;
    unsigned n;
    

    while (fgets(line, 1024, fp)){	
    	char *tmp = line;
        char *name = csv_get_field(tmp,1);
        char*domain = ".local";
        qj->key->name = strcat(line, domain);
		//qj->key->name = line;
        if (!(p = avahi_dns_packet_new_query(s->interface->hardware->mtu)))
            return; /* OOM */

        if (!avahi_dns_packet_append_key(p, qj->key, 0))
            return 0;

        n = 1;

        avahi_dns_packet_set_field(p, AVAHI_DNS_FIELD_QDCOUNT, n);
        avahi_hexstring(AVAHI_DNS_PACKET_DATA(p), p->size);
        avahi_hexdump_file(AVAHI_DNS_PACKET_DATA(p), p->size);
        avahi_dns_packet_free(p);  

    }
}
