#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>

#include "network.h"
#include "scanner.h"
#include "cJSON.h"

 


void *scan_worker(void* info_arg)
{
    struct worker_info* info = info_arg;

    info->control.running = 1;

    info->stats.time_start = time(NULL);

    uint32_t current_ip = ip_to_hostbytes(info->ip_range.begin);

    for(uint32_t i = 0; i < info->ip_range.amount && !info->control.stop_requested; i++) 
    {
        info->stats.nscanned++;
        const unsigned short port = 25565;

        char tmp[17] = {0};
        hostbytes_to_ip(current_ip, tmp, 16);

        //printf("[%lX]connecting to %s\n", info->id, tmp);
        int s = connect_to(current_ip, port);
        
        if(s == -1)
        {
            current_ip++;
            continue;
        }

        info->stats.nconnected++;

        //printf("[%lX]connected to %s\n", info->id, tmp);

        uint8_t pl_handshake[] = { 
            0x7,
            0x00,
            0xF3, 0x05,
            0x00, 
            0x63, 0xDD,
            0x01
        };

        uint8_t pl_status_request[] = {0x01, 0x00};

        send_data(s, pl_handshake, sizeof(pl_handshake));
        send_data(s, pl_status_request, sizeof(pl_status_request));

        unsigned int data_size = recv_data(s, info->buffer.buffer, info->buffer.size);

        if(data_size)
        {
            struct server_info server = parse_data(info->buffer.buffer, data_size);
            if(server.description != NULL)
            {
                info->stats.nhits++;

                server.ip = calloc(INET_ADDRSTRLEN, 1);
                hostbytes_to_ip(current_ip, server.ip, INET_ADDRSTRLEN);

                log_server_info(info->control.log_file, server);
            }
            free_server_info(server);
        }

        current_ip++;
        close(s);
    }

    info->stats.time_end = time(NULL);
    info->control.running = 0;
}

struct worker_info* start_worker(const char* ip_begin, long int amount)
{

    static int file = 0;

    if(file == 0)
    {
        char* log_path = calloc(PATH_MAX, 1);
        getcwd(log_path, PATH_MAX);
        strcat(log_path, "/log.txt");

        file = open(log_path, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        free(log_path);

        if(file == -1)
        {
            printf("Error while opening file! %i\n", errno);
            printf("%s\n", strerror(errno));
            return NULL;
        }
    }

    unsigned int buffer_size = 100000;
    
    struct worker_info* info = calloc(sizeof(struct worker_info), 1);

    info->ip_range.begin = calloc(strlen(ip_begin) + 1, 1);
    strcpy(info->ip_range.begin , ip_begin);
    info->ip_range.amount = amount;

    info->buffer.buffer = malloc(buffer_size);
    info->buffer.size = buffer_size;
    info->control.log_file = file;

    //printf("Starting worker with address %s\n", info->ip_range.begin);

    pthread_create(&info->id, NULL, scan_worker, info);

    time_t time_start = time(NULL);

    while(time(NULL) - time_start <= 30)
    {
        if(info->control.running) return info;
    }

    printf("Thread did not start within 5 seconds!\n");
    free_worker_info(info);
    return NULL;
}


void free_server_info(struct server_info info)
{
    free(info.description);
    free(info.version.name);
    free(info.ip);
}

void log_server_info(int fd, struct server_info info)
{
    char buffer[100000] = {0};
    sprintf(buffer,"[%s][%s] %s [%i/%i]\n", info.ip, info.version.name,info.description, info.players.online, info.players.max);

    printf("%s",buffer);
    write(fd, buffer, strlen(buffer));
}


void free_worker_info(struct worker_info* info)
{
    if(info == NULL)return;
    free(info->buffer.buffer);
    free(info->ip_range.begin);
    free(info);
}


struct server_info parse_data(const char* raw, unsigned int data_size)
{

    struct server_info info = {0};

    uint8_t offset = 0;
    for(; raw[offset] != '{' && data_size - offset;offset++);

    raw += offset;
    data_size -= offset;


    cJSON* json = cJSON_ParseWithLength(raw, data_size);


    if(cJSON_IsInvalid(json) || json == NULL)
    {

        char tmp[10000] = {0};
        for(int i = 0; i < data_size && i < 10000; i++)
        {
            sprintf(tmp, "%c",raw[i]);
        }
        if(strlen(tmp) > 0)
            printf("Parsing failed! Data: [%s]\n",tmp);

        cJSON_Delete(json);
        return info;
    }
  
    cJSON* parrent = json;
    json = json->child;

    while(json)
    {

        if(strcmp(json->string, "version") == 0)
        {
            cJSON* version = json->child;

            while(version)
            {
                if(strcmp(version->string, "name") == 0)
                {
                    char* name = calloc(strlen(version->valuestring) + 1, 1);
                    strcpy(name, version->valuestring);
                    info.version.name = name;
                }

                else if(strcmp(version->string, "protocol") == 0)
                {
                    info.version.protocol = version->valueint;
                }

                version = version->next;
            }
        }

        else if(strcmp(json->string, "players") == 0)
        {
            cJSON* players = json->child;

            while(players)
            {
                if(strcmp(players->string, "max") == 0)
                {
                    info.players.max = players->valueint;
                }

                else if(strcmp(players->string, "online") == 0)
                {
                    info.players.online = players->valueint;
                }

                players = players->next;
            }
        }

        else if(strcmp(json->string, "description") == 0)
        {

            if(cJSON_IsString(json))
            {
                unsigned int len = strlen(json->valuestring);
                info.description = calloc(len + 1, 1);
                strcpy( info.description, json->valuestring);
            }
            else
            {
                cJSON* description = json->child;

                char* description_str = NULL;
                unsigned int description_len = 0;

                while(description)
                {
                    if(strcmp(description->string, "text") == 0)
                    {
                        unsigned int len = strlen(description->valuestring);
                        description_str = realloc(description_str, description_len + len);
                        strcpy(description_str + description_len, description->valuestring);
                        description_len += len;
                    }
                    else if(cJSON_IsArray(description))
                    {
                        cJSON* entry = description->child;

                        while(entry)
                        {
                            cJSON* member = entry->child;
                            while(member)
                            {
                                if(strcmp(member->string, "text") == 0)
                                {
                                    unsigned int len = strlen(member->valuestring);
                                    description_str = realloc(description_str, description_len + len);
                                    strcpy(description_str + description_len, member->valuestring);
                                    description_len += len;
                                }
                                member = member->next;
                            }
                            entry = entry->next;
                        }
                    }
                    description = description->next;
                }

                description_str = realloc(description_str, description_len + 1);
                description_str[description_len] = 0;

                for(unsigned int i = 0; i < description_len; i++)
                {
                    if(description_str[i] == '\n')
                    {
                        description_str[i] = '*';
                    }
                }

                info.description = description_str;
            }
        }
        json = json->next;
    }

    cJSON_Delete(parrent);
    return info;
}