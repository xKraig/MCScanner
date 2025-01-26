#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "network.h"


unsigned int recv_data(int fd, void* buffer, unsigned int buffer_size)
{

    memset(buffer, 0, buffer_size);
    struct pollfd poll_info = {.fd = fd, .events = POLLIN | POLLHUP};

    int bytes_received = 0;

    while( bytes_received < buffer_size )
    {
        if (poll(&poll_info, 1, 500) == -1)
        {
            printf("poll() failed! Error: %i\n", errno);
            printf("%s\n", strerror(errno));
            return 0;
        }

        if( poll_info.revents & POLLIN )
        {
            int n = recv(fd, buffer + bytes_received, buffer_size - bytes_received, 0);
            bytes_received += n;

            if( n == 0)
            {
                return bytes_received;
            }
        }
        else return bytes_received;
    }

    printf("Buffer overflow!\n");
    return bytes_received;
}


int send_data(int fd, uint8_t* data, unsigned int size)
{
    int sent = 0;
    do
    {
        int amnt = send(fd, data + sent, size - sent, 0);
        if(amnt == -1)
        {
            printf("Error while sending! %i\n", errno);
            printf("%s\n", strerror(errno));
            return errno;
        }
        sent += amnt;
    }
    while(sent < size);

    return 0;
}



int connect_to(uint32_t ip, unsigned short port)
{

    const int tcp_protocol_id = 6;

    int s = socket(AF_INET, SOCK_STREAM, tcp_protocol_id);
    if(s == -1)
    {
        printf("socket() failed! Error: %i\n", errno);
        printf("%s\n", strerror(errno));
        return -1;
    }

    struct timeval connect_time = { .tv_sec = 0, .tv_usec = 500*1000}; //500ms
    struct timeval send_time = { .tv_sec = 2, .tv_usec = 0}; //2s
    struct timeval recv_time = { .tv_sec = 2, .tv_usec = 0}; //2s

    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &connect_time, sizeof(connect_time));
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &recv_time, sizeof(recv_time));

    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ip);

    if(connect(s, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        return -1;
    }
    printf("Connect success!\n");
    return s;
}

uint32_t ip_range_size(const char* begin, const char* end)
{
    struct in_addr addr_begin = {0};
    struct in_addr addr_end = {0};

    inet_pton(AF_INET, begin, &addr_begin);
    inet_pton(AF_INET, end, &addr_end);

    uint32_t ip = ntohl(addr_begin.s_addr);
    uint32_t ip_end = ntohl(addr_end.s_addr);

    return ip_end - ip;
}

char* get_next_ip(const char* ip)
{
    struct in_addr addr = {0};

    inet_pton(AF_INET, ip, &addr);

    uint32_t ip_number = ntohl(addr.s_addr);
    ip_number++;

    char* new_ip = calloc(16,1);
    inet_ntop(AF_INET, &addr, new_ip, 16);

    return new_ip;
}