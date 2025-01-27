#ifndef H_SCANNER
#define H_SCANNER

#include <sys/time.h>
#include <stdint.h>
#include <pthread.h>

struct server_info
{
    char* ip;

    struct version
    {
        char* name;
        int protocol;
    }version;
    
    struct players
    {
        int max;
        int online;
    }players;

    char* description;
};

struct worker_info
{
    pthread_t id;

    struct
    {
        uint8_t* buffer;
        unsigned int size;
    } buffer;

    struct
    {
        char* begin;
        uint32_t amount;
    } ip_range;

    struct
    {
        unsigned int nscanned;
        unsigned int nconnected;
        unsigned int nhits;
        time_t time_start;
        time_t time_end;
    } stats;

    struct
    {
        uint8_t stop_requested;
        uint8_t running;
        int log_file;
    } control;
};


void free_server_info(struct server_info info);
void log_server_info(int fd, struct server_info info);
struct worker_info* start_worker(const char* ip_begin, long int amount);
void stop_worker(struct worker_info* info);
void free_worker_info(struct worker_info* info);
struct server_info parse_data(const char* raw, unsigned int data_size);
#endif