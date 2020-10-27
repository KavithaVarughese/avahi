#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h> 

#include <string.h>
#include <sys/types.h> 
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <avahi-common/timeval.h>
#include <avahi-common/malloc.h>
#include <avahi-common/domain.h>
#include <avahi-common/strlst.h>

#include "customised_packets.h"
#include "log.h"
#include "rr-util.h"



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

static int add_record_to_packet(AvahiResponseScheduler *s, AvahiDnsPacket *p, AvahiResponseJob *rj) {
    assert(s);
    assert(p);
    assert(rj);

    /* Try to add this record to the packet */
    if (!avahi_dns_packet_append_record(p, rj->record, rj->flush_cache, 0))
        return 0;

    return 1;
}

long int ipv4_address_converter(char *s){

    struct in_addr result;

    if(inet_pton(AF_INET, s, &result))
        {
        return(result.s_addr);
        }
    return 0;
}

void ipv6_address_converter(char *s, uint8_t *address){

    struct in6_addr result;

    if(inet_pton(AF_INET6, s, &result))
        {
        memcpy(address, result.s6_addr, 16*sizeof(uint8_t));
        }
    return;
}

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

void customized_packets_formation(AvahiResponseJob *begin, AvahiResponseJob *end, AvahiResponseJob *rj_copy, AvahiResponseScheduler *s){
	//int Ip_Array[12] = {268566538, 285343754, 302120970, 268566538, 285343754, 302120970, 268566538, 285343754, 302120970,268566538, 285343754, 302120970 };
	FILE *fp = fopen("ip.csv","r");
    	char line[1024];
	printf("\nSuccessfully in customised packets\n");


	AVAHI_LLIST_HEAD(AvahiResponseJob,llcopy);
	llcopy = begin;

	AvahiResponseJob *rj[6];
	int j = 0;
	while(llcopy){
		rj[j] = llcopy;
		j++;
		llcopy = llcopy->jobs_next;
	}
	printf("\nJJJJJJJJJJJJJJJJJJ %d \n", j);
	
	AvahiResponseJob *tmp = rj[0];
	rj[0] = rj[2];
	rj[2] = tmp;
	tmp = rj[1];
	rj[1] = rj[3];
	rj[3] = tmp;
	rj[4] = rj[5];
	
	while (fgets(line, 1024, fp))
	{	
		AvahiDnsPacket *p;
   		unsigned n;
		//Related to CSV
        	//char* tmp = strdup(line);
		char *name = csv_get_field(strdup(line),2); // Get name of service from CSV
		char *type = csv_get_field(strdup(line),3); //Get service type from CSV
		char *domain = "local"; //always
		char *host = "snoopsxox-VirtualBot.local"; // Also taken from CSV I believe
		char *txt_name = csv_get_field(strdup(line),4); //Hopefully from CSV
		char ptr_name[AVAHI_DOMAIN_NAME_MAX], svc_name[AVAHI_DOMAIN_NAME_MAX], enum_ptr[AVAHI_DOMAIN_NAME_MAX];
		//Related to CSV

		//Formation of the required names
		int ret;
		if ((ret = avahi_service_name_join(svc_name, sizeof(svc_name), name, type, domain)) < 0 ||
        	(ret = avahi_service_name_join(ptr_name, sizeof(ptr_name), NULL, type, domain)) < 0 ||
        	(ret = avahi_service_name_join(enum_ptr, sizeof(enum_ptr), NULL, "_services._dns-sd._udp", domain)) < 0) {
        	exit(0);
    	}
    	
				
		//For TXT Records
		
		//changing strlst
		AvahiStringList *strlst = NULL;
    	strlst = avahi_string_list_add(strlst,txt_name);
		
		while(rj_copy->record->data.txt.string_list)
		{
			printf("\nStrlst value\n");
			rj_copy->record->data.txt.string_list = rj_copy->record->data.txt.string_list->next;
		}
		//changing txt
		rj_copy->record->key->name = avahi_normalize_name_strdup(svc_name);
		rj_copy->record->data.txt.string_list = strlst;
		
		if (!(p = avahi_dns_packet_new_response(s->interface->hardware->mtu, 1)))
        		return; /* OOM */
    		n = 1;
		
		if (add_record_to_packet(s, p, rj_copy)) {

        	/* Try to fill up packet with more responses, if available */
        	for(j = 0; j < 5 ; j++) {

			//For PTR Records
			if(j==0){
				rj[j]->record->key->name = avahi_normalize_name_strdup(ptr_name);
				rj[j]->record->data.ptr.name = avahi_normalize_name_strdup(svc_name);
			}
			//For SRV Records
			if(j==1){
				rj[j]->record->key->name = avahi_normalize_name_strdup(svc_name);
				rj[j]->record->data.srv.name = host;	
			}
			//For AAAA Records 
			
			// For A Records
        	if(j == 3){
        		rj[j]->record->data.a.address.address = ipv4_address_converter(csv_get_field(strdup(line), 1));
        	}
        	
        	//For enumeration record
        	if(j==4)
        	{
        		rj[j]->record->key->name = avahi_normalize_name_strdup(enum_ptr);
        		rj[j]->record->data.ptr.name = avahi_normalize_name_strdup(ptr_name);
        		
        	}
        	
            	if (!add_record_to_packet(s, p, rj[j]))
            	    break;
            	

            	n++;
       		}

    	}
    	avahi_dns_packet_set_field(p, AVAHI_DNS_FIELD_ANCOUNT, n);
    	avahi_hexstring(AVAHI_DNS_PACKET_DATA(p), p->size);
    	avahi_hexdump_file(AVAHI_DNS_PACKET_DATA(p), p->size);
    	avahi_dns_packet_free(p);
        //free(tmp);
		 
	}
}