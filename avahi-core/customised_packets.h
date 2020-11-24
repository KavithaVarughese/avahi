#include <avahi-common/address.h>
#include "iface.h"

typedef struct AvahiResponseJob AvahiResponseJob;
typedef struct AvahiQueryJob AvahiQueryJob;
typedef struct AvahiKnownAnswer AvahiKnownAnswer;

//int file_is_empty(FILE *fp);

long int ipv4_address_converter(char *s);

void ipv6_address_converter(char *s, uint8_t *address);

char* csv_get_field(char* line, int num);

void customized_packets_formation(AvahiResponseJob *begin, AvahiResponseJob *end, AvahiResponseJob *rj_copy, AvahiResponseScheduler *s);
void customised_query_packets(AvahiQueryJob *qj, AvahiQueryScheduler *s);
