#ifndef H_NETWORK
#define H_NETWORK

#include <stdint.h>

unsigned int recv_data(int fd, void* buffer, unsigned int buffer_size);
int connect_to(uint32_t ip, unsigned short port);
int send_data(int fd, uint8_t* data, unsigned int size);
uint32_t ip_range_size(const char* begin, const char* end);
char* get_next_ip(const char* ip);
#endif