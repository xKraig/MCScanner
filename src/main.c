#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "scanner.h"
#include "network.h"



int main()
{

    unsigned int workers = 0;

    char ip_begin_str[INET_ADDRSTRLEN] = "1.0.0.0";
    char ip_end_str[INET_ADDRSTRLEN] = "9.255.255.255";

    uint32_t range_size = ip_range_size(ip_begin_str, ip_end_str);
    uint32_t worker_target = 20000;
    uint32_t worker_range = range_size / worker_target;
    struct worker_info** worker_list = calloc(sizeof(struct worker_info), worker_target);

    for(int i = 0; i < worker_target; i++)
    {
        char ip_str[INET_ADDRSTRLEN] = {0};
        get_ip_by_offset(ip_begin_str, ip_str, i * worker_range);

        struct worker_info* worker = start_worker(ip_str, worker_range);

        if(worker == NULL)
        {
            printf("Starting worker failed!\n");
            continue;
        }

        worker_list[i] = worker;
        workers++;
    }
    printf("Started %lu workers with %lu ips each. Range is %lu\n", worker_target, worker_range, range_size);

    sleep(1);
    time_t begin = time(NULL);

    sleep(2);
    while(1)
    {
            uint8_t active = 0;

            unsigned int total_connects = 0;
            unsigned int total_hits = 0;
            unsigned int total_scanned = 0;

            for(unsigned int i = 0; i < workers; i++)
            {
                total_connects += worker_list[i]->stats.nconnected;
                total_hits += worker_list[i]->stats.nhits;
                total_scanned += worker_list[i]->stats.nscanned;

                if(worker_list[i]->control.running)
                    active = 1;
            }

            printf("\n\n\n"
                    "Scanned:      %10u/%10u (%10f%%  ) %.2f/s\n"
                    "Connected:    %10u/%10u (%10.2f%%)\n"
                    "Hits:         %10u/%10u (%10.2f%%)\n",
                    "Runtime:      %10u"
                    ,total_scanned, range_size, ((float)total_scanned)*100.f/range_size, ((float)total_scanned)/(time(NULL)-begin)
                    ,total_connects, total_scanned, ((float)total_connects)*100.f/total_scanned
                    ,total_hits, total_connects, total_connects?((float)total_hits)*100.f/total_connects:0,
                    time(NULL) - begin);

            if(active == 0)
                break;

            
            sleep(3);
    }
    
    for(int i = 0; i < worker_target; i++)
    {
        pthread_join(worker_list[i]->id, NULL);
        free_worker_info(worker_list[i]);
    }

    free(worker_list);
    printf("\n\nFinished!\n");
}




