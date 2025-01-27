#ifndef H_NETWORK
#define H_NETWORK

#include <stdint.h>

unsigned int recv_data(int fd, uint8_t* buffer, unsigned int buffer_size);
int connect_to(uint32_t ip, unsigned short port);
int send_data(int fd, uint8_t* data, unsigned int size);
uint32_t ip_range_size(const char* begin, const char* end);
uint8_t get_ip_by_offset(const char* src, char* dst, long int offset);
uint32_t ip_to_hostbytes(const char* ip);
char* hostbytes_to_ip(uint32_t hostbytes, char* dst, unsigned int buffer_size);
#endif