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

//check if the file is empty
/*int file_is_empty(FILE *fp){*/

/*	int size;*/
/*	if (NULL != fp) {*/
/*    		fseek (fp, 0, SEEK_END);*/
/*    		size = ftell(fp);*/

/*    		if (0 == size) {*/
/*        		return 1;*/
/*    		}*/
/*	}*/
/*	return 0;	*/
/*}*/
//Converts records into byte buffers and appends into the packet
static int add_record_to_packet(AvahiResponseScheduler *s, AvahiDnsPacket *p, AvahiResponseJob *rj) {
    assert(s);
    assert(p);
    assert(rj);

    /* Try to add this record to the packet */
    if (!avahi_dns_packet_append_record(p, rj->record, rj->flush_cache, 0))
        return 0;

    return 1;
}

//converts ipv4 string (10.0.2.16) to long integer
long int ipv4_address_converter(char *s){

    struct in_addr result;

    if(inet_pton(AF_INET, s, &result))
        {
        return(result.s_addr);
        }
    return 0;
}

//converts ipv6 string (2001:0db8:85a3:0000:0000:8a2e:0370:7334) to array of integers of size 16
void ipv6_address_converter(char *s, uint8_t *address){

    struct in6_addr result;

    if(inet_pton(AF_INET6, s, &result))
        {
        memcpy(address, result.s6_addr, 16*sizeof(uint8_t));
        }
    return;
}

//gets field from the csv
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

//main function to obtain customised packets
void customized_packets_formation(AvahiResponseJob *begin, AvahiResponseJob *end, AvahiResponseJob *rj_copy, AvahiResponseScheduler *s){

	FILE *fp = fopen("ip.csv","r");
	FILE *csv_writer;
    char line[1024];
	printf("\nSuccessfully in customised packets\n");
	
	FILE *tempfp = fopen("log.txt", "w");
	fprintf(tempfp, "inside customised packets formation\n");
	fclose(tempfp);


	AVAHI_LLIST_HEAD(AvahiResponseJob,llcopy);
	llcopy = begin;
	
	//copying into an array
	AvahiResponseJob *rj[6];
	int j = 0;
	while(llcopy) {
		rj[j] = llcopy;
		j++;
		llcopy = llcopy->jobs_next;
	}
	
	
	//TRY SETTING FLAGS FOR CURRENT RECORD ORDER
	
	
	
	
	while (fgets(line, 1024, fp)) {

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
		int mode = (strcmp("announce", csv_get_field(strdup(line), 7)) == 0) ? 1 : 0;
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
		
		while(rj_copy->record->data.txt.string_list) {
			rj_copy->record->data.txt.string_list = rj_copy->record->data.txt.string_list->next;
		}
		//changing txt
		rj_copy->record->key->name = avahi_normalize_name_strdup(svc_name);
		rj_copy->record->data.txt.string_list = strlst;
		
		if (!(p = avahi_dns_packet_new_response(s->interface->hardware->mtu, 1)))
        		return; /* OOM */
    	
    	n = 1;

    	/* Try to fill up packet with more responses, if available */
    	
    	//Original Record Order:
		//AAAA    A    PTR    SRV    GARBAGE    PTR(DNS)
		// 0      1     2      3        4          5


		//For PTR Records
		rj[2]->record->key->name = avahi_normalize_name_strdup(ptr_name);
		rj[2]->record->data.ptr.name = avahi_normalize_name_strdup(svc_name);
		if(mode == 0)
			rj[2]->record->ttl = 0;
			
		//For SRV Records
		rj[3]->record->key->name = avahi_normalize_name_strdup(svc_name);
		rj[3]->record->data.srv.name = host;
		if(mode == 0)
			rj[3]->record->ttl = 0;

		//For AAAA Records 
		ipv6_address_converter(csv_get_field(strdup(line), 5), rj[0]->record->data.aaaa.address.address);

		// For A Records
		rj[1]->record->data.a.address.address = ipv4_address_converter(csv_get_field(strdup(line), 1));
	
		//For enumeration record
		rj[5]->record->key->name = avahi_normalize_name_strdup(enum_ptr);
		rj[5]->record->data.ptr.name = avahi_normalize_name_strdup(ptr_name);
		if(mode == 0)
			rj[5]->record->ttl = 0;
		
		
    	    tempfp = fopen("log.txt", "a");
			fprintf(tempfp, "changed values\n");
			fclose(tempfp);
			
		
		//Original Record Order:
		//AAAA    A    PTR    SRV    GARBAGE    PTR(DNS)
		// 0      1     2      3        4          5
		//Announce Packet Record Order:
		//TXT    PTR    SRV     AAAA    A    PTR(DNS)
		//Withdraw Packet Record Order:
		//PTR(DNS)    PTR    SRV    AAAA    A    TXT
		if(mode == 1) {
			if(!add_record_to_packet(s, p, rj_copy))
				break;
			if (!add_record_to_packet(s, p, rj[2]))
    	    	break;
			if (!add_record_to_packet(s, p, rj[3]))
    	    	break;
			if (!add_record_to_packet(s, p, rj[0]))
    	    	break;
			if (!add_record_to_packet(s, p, rj[1]))
    	    	break;
			if (!add_record_to_packet(s, p, rj[5]))
    	    	break;
    	    tempfp = fopen("log.txt", "a");
			fprintf(tempfp, "announce\n");
			fclose(tempfp);
		} else {
			if (!add_record_to_packet(s, p, rj[5]))
    	    	break;
			if (!add_record_to_packet(s, p, rj[2]))
    	    	break;
			if (!add_record_to_packet(s, p, rj[3]))
    	    	break;
			if (!add_record_to_packet(s, p, rj[0]))
    	    	break;
			if (!add_record_to_packet(s, p, rj[1]))
    	    	break;
    	    rj_copy->record->ttl = 0;
			if(!add_record_to_packet(s, p, rj_copy))
				break;
    	    tempfp = fopen("log.txt", "a");
			fprintf(tempfp, "withdraw\n");
			fclose(tempfp);
		}
    	
    	//Number of records added	
    	n+=5;
		
		//writing to announce.csv/withdraw.csv
		csv_writer = (mode == 1) ? fopen("announce.csv", "a") : fopen("withdraw.csv", "a");
		fprintf(csv_writer, "%s;%s;%s;%s;%s;%s;", csv_get_field(strdup(line), 1), csv_get_field(strdup(line), 2), csv_get_field(strdup(line), 3), csv_get_field(strdup(line), 4), csv_get_field(strdup(line), 5), csv_get_field(strdup(line), 6));
		fclose(csv_writer);
		
		avahi_dns_packet_set_field(p, AVAHI_DNS_FIELD_ANCOUNT, n);
		//mode = 1 -> announce, 0 -> ttl0
		avahi_hexstring(AVAHI_DNS_PACKET_DATA(p), p->size, mode);
		avahi_hexdump_file(AVAHI_DNS_PACKET_DATA(p), p->size);
		avahi_dns_packet_free(p);
    	//free(tmp);
		 
	}
	fclose(fp);
	fp = fopen("ip.csv","w");
	fclose(fp);
}


void customised_query_packets(AvahiQueryJob *qj, AvahiQueryScheduler *s){
	
	FILE *fp = fopen("browse_service.csv","r"), *brw;
    char line[1024];

    AvahiDnsPacket *p;
    unsigned n;
    

    while (fgets(line, 1024, fp)){
        char *name = csv_get_field(strdup(line),1);
        char*domain = ".local";
        qj->key->name = strcat(name, domain);
		//qj->key->name = line;
        if (!(p = avahi_dns_packet_new_query(s->interface->hardware->mtu)))
            return; /* OOM */

        if (!avahi_dns_packet_append_key(p, qj->key, 0))
            return 0;

        n = 1;
        
        brw = fopen("browse.csv", "a");
        fprintf(brw, "%s;%s;",  name, csv_get_field(strdup(line), 2));
        fclose(brw);

        avahi_dns_packet_set_field(p, AVAHI_DNS_FIELD_QDCOUNT, n);
        avahi_hexstring(AVAHI_DNS_PACKET_DATA(p), p->size, 2);
        avahi_hexdump_file(AVAHI_DNS_PACKET_DATA(p), p->size);
        avahi_dns_packet_free(p);  

    }
	fclose(fp);
	fp = fopen("browse_service.csv", "w");
	fclose(fp);
}
