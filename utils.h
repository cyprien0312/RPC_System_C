
#ifndef UTILS
#define UTILS
#include <stddef.h>
#include <stdint.h>

int create_listening_socket(char* service);
int create_commute_socket(char *addr, int port);
uint64_t htonll(uint64_t value);
uint64_t ntohll(uint64_t value);
#endif