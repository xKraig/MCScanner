#ifndef H_SCANNER
#define H_SCANNER

#include <sys/time.h>
#include <stdint.h>

struct server_info
{
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
    int id;

    struct
    {
        uint8_t* buffer;
        unsigned int size;
    } buffer;

    struct
    {
        char* begin;
        char* end;
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
    } control;
};


void free_server_info(struct server_info info);
const char * format_server_info(struct server_info);

#endif