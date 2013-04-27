#ifndef DNS_H_
#define DNS_H_
// DNS functionality for the client

#include "common.h"

#include <stdint.h>

#include "string.h"

// Packet header field flags
#define DNS_CLASS_IN 1
#define DNS_TYPE_A 1
#define DNS_TYPE_AAAA 28

int dns_send_query(int fd, uint16_t type, const char *name); 

String **dns_receive_answer(int fd); 

#endif
