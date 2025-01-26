#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "network.h"
#include "scanner.h"
#include "cJSON.h"

 
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
        printf("Parsing failed!\n");
        cJSON_Delete(json);
        return info;
    }
  
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
        json = json->next;
    }

    cJSON_Delete(json);
    return info;
}

void scan_worker(struct worker_info* info)
{
    struct in_addr addr_begin = {0};
    struct in_addr addr_end = {0};

    inet_pton(AF_INET, info->ip_range.begin, &addr_begin);
    inet_pton(AF_INET, info->ip_range.end, &addr_end);

    uint32_t ip = ntohl(addr_begin.s_addr);
    uint32_t ip_end = ntohl(addr_end.s_addr);

    while(ip <= ip_end)
    {
        const unsigned short port = 25565;

        struct in_addr tmp;
        tmp.s_addr = ntohl(ip);
        char ipstr[200] = {0};
        printf("Connecting to %s... ", inet_ntop(AF_INET, &tmp, ipstr, 200));

        int s = connect_to(ip++, port);
        
        if(s == -1)
        {
            printf("Connection failed!\n");
            continue;
        }

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
        close(s);

        printf("Received %i bytes!\n", data_size);

        struct server_info server = parse_data(info->buffer.buffer, data_size);

        printf("Description: %s"
                " Version: %s %i"
                " Players: %i/%i\n", server.description, server.version.name, server.version.protocol, server.players.online, server.players.max);


        free_server_info(server);
    }
}

struct worker_info* start_worker(const char* ip_begin, const char* ip_end)
{

    unsigned int buffer_size = 100000;
    
    struct worker_info* info = calloc(sizeof(struct worker_info), 1);

    info->ip_range.begin = malloc(strlen(ip_begin));
    info->ip_range.end = malloc(strlen(ip_end));
    strcpy(info->ip_range.begin , ip_begin);
    strcpy(info->ip_range.end, ip_end);

    info->buffer.buffer = malloc(buffer_size);
    info->buffer.size = buffer_size;

    scan_worker(info);
}


void free_server_info(struct server_info info)
{
    free(info.description);
    free(info.version.name);
}

