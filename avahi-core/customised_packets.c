#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h> 
#include </usr/include/mysql/mysql.h>
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

//main function to obtain customised packets
void customized_packets_formation(AvahiResponseJob *begin, AvahiResponseJob *end, AvahiResponseJob *rj_copy, AvahiResponseScheduler *s){

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
    
    //Getting mysql connection and trying to execute a statemtnt
    //mysql objects

    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    //mysql server and database definitions
    char *server = "172.22.0.2";
    char *user = "mysql";
    char *password = "supersecret";
    char *database = "tgen";

    conn = mysql_init(NULL);

    /* Connect to database */
    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)){
        printf("Failed to connect to the MYSQL database\n");
    }

    res = mysql_use_result(conn);
    
    //Send SQL Query
    if (mysql_query(conn, "select * from announce where byte_buffer IS NULL")){
        printf("Failed to execute query. Error: %s\n", mysql_error(conn));
    }
    res = mysql_store_result(conn);
    
    int columns = mysql_num_fields(res);
    
    int i = 0;
    
    while(row = mysql_fetch_row(res))
    {
	//checking the values
	FILE *tempfp = fopen("log.txt", "a");
    	fprintf(tempfp, "1: %s \n", row[1]);
	fprintf(tempfp, "2: %s \n", row[2]);
	fprintf(tempfp, "3: %s \n", row[3]);
	fprintf(tempfp, "4: %s \n", row[4]);
	fprintf(tempfp, "5: %s \n", row[5]);
	fprintf(tempfp, "6: %s \n", row[6]);
	fprintf(tempfp, "7: %s \n", row[7]);
	fprintf(tempfp, "8: %s \n", row[8]);
    	fclose(tempfp);

        AvahiDnsPacket *p;
        unsigned n;		
        char *name = row[1];
        char *type = row[2];
        char *domain = "local"; //always
        char *host = "snoopsxox-VirtualBot.local"; // Also taken from CSV I believe
        char *txt_name = row[3];
        char *macadd = row[7];
        char ptr_name[AVAHI_DOMAIN_NAME_MAX], svc_name[AVAHI_DOMAIN_NAME_MAX], enum_ptr[AVAHI_DOMAIN_NAME_MAX];
        int mode = (strcmp(row[8], "4500") == 0) ? 1 : 0;
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
        char *ipv6 = row[5];
        ipv6_address_converter(ipv6, rj[0]->record->data.aaaa.address.address);

        // For A Records
        char *ipv4 = row[4];
        rj[1]->record->data.a.address.address = ipv4_address_converter(ipv4);
    
        //For enumeration record
        rj[5]->record->key->name = avahi_normalize_name_strdup(enum_ptr);
        rj[5]->record->data.ptr.name = avahi_normalize_name_strdup(ptr_name);
        if(mode == 0){
            rj[5]->record->ttl = 0;
        }    
        
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

        }
        
        //Number of records added	
        n+=5;
 
        avahi_dns_packet_set_field(p, AVAHI_DNS_FIELD_ANCOUNT, n);
	tempfp = fopen("log.txt","a");
	fprintf(tempfp, "\nAlmost there");
	fclose(tempfp);
        //mode = 1 -> announce, 0 -> ttl0
	char hexval[1024];
        avahi_hexstring(hexval, AVAHI_DNS_PACKET_DATA(p), p->size, mode);
        avahi_dns_packet_free(p);
        //free(tmp);
	tempfp = fopen("log.txt","a");
	fprintf(tempfp, "\ninserting");
	fclose(tempfp);
	
	//Inserting hex into the database
	char buf[1024] = {};
	char query[] = {"insert into announce (byte_buffer) values (%s)"}; 
	sprintf(buf, query, hexval);
	if (mysql_query(conn, buf)){
        	printf("Failed to execute query. Error: %s\n", mysql_error(conn));
    	}
	tempfp = fopen("log.txt","a");
	fprintf(tempfp, "\ninserting completed \n");
	fclose(tempfp);

    }
    
    //Closing the connection to sql and freeing the sql resultts
    mysql_free_result(res);
    mysql_close(conn);
}


void customised_query_packets(AvahiQueryJob *qj, AvahiQueryScheduler *s){
    
    FILE *brw;
    AvahiDnsPacket *p;
    unsigned n;


    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    char *server = "`172.22.0.2";
    char *user = "mysql";
    char *password = "supersecret";
    char *database = "tgen";

    conn = mysql_init(NULL);

    /* Connect to database */
    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)){
        printf("Failed to connect to the MYSQL database\n");
    }

    res = mysql_use_result(conn);
    /* Execute SQL query to fetch all table names.*/
    if (mysql_query(conn, "show tables"))
    {
        printf(("Failed to execute MYSQL quesry. Error: %s\n", mysql_error(conn)));
    }
    
    res = mysql_use_result(conn);
    
    /* Output table name */
    
    printf("MYSQL Tables in mydb database:\n");
    while ((row = mysql_fetch_row(res)) != NULL){
        
        printf("%s \n", row[0]);
    }
    
    
    // free results
    mysql_free_result(res);
    
    //Send SQL Query
    if (mysql_query(conn, "select * from browse_service"))
    {
        printf("Failed to execute quesry. Error: %s\n", mysql_error(conn));
    }
    res = mysql_store_result(conn);
    
    int columns = mysql_num_fields(res);
    
    int i = 0;
    
    printf("Entries in the table my_table:\n");
    while(row = mysql_fetch_row(res))
    {
        char *name = row[0];
        char *domain = ".local";
        char *mac = row[1];
        qj->key->name = strcat(name, domain);
        
        if (!(p = avahi_dns_packet_new_query(s->interface->hardware->mtu)))
            return; /* OOM */

        if (!avahi_dns_packet_append_key(p, qj->key, 0))
            return 0;

        n = 1;
        
        brw = fopen("browse.csv", "a");
        fprintf(brw, "%s;%s;",  name, mac);
        fclose(brw);

        avahi_dns_packet_set_field(p, AVAHI_DNS_FIELD_QDCOUNT, n);
        avahi_hexstring(AVAHI_DNS_PACKET_DATA(p), p->size, 2);
        avahi_hexdump_file(AVAHI_DNS_PACKET_DATA(p), p->size);
        avahi_dns_packet_free(p);

    }
    
}
