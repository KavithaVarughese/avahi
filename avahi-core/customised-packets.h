#include <avahi-common/address.h>
#include "iface.h"

typedef struct AvahiQueryJob AvahiQueryJob;
typedef struct AvahiKnownAnswer AvahiKnownAnswer;

void customised_query_packets(AvahiQueryJob *qj, AvahiQueryScheduler *s);
