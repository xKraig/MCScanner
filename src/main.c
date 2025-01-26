#include <stdio.h>

#include "cJSON.h"
#include "network.h"



struct worker_info* start_worker(const char* ip_begin, const char* ip_end);

void log_msg(int id, const char* message);


void stop_worker(struct worker_info* info);


void print_buffer(const void* buffer, unsigned int size)
{
    for(unsigned int i = 0; i < size; i++)
    {
        printf("%c ", ((uint8_t*)buffer)[i]);
    }
    printf("\n");
}


int main()
{

   start_worker("49.13.113.15", "49.13.113.22");
   //start_worker("127.0.0.1", "127.0.0.1");

    printf("Finished!\n");
}




