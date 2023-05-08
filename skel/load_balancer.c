/* Copyright 2023 <> */
#include <stdlib.h>
#include <string.h>

#include "load_balancer.h"
#include "server.h"
#include "list_ht.h"

#define MIN_HASH 0
#define MAX_HASH 360

struct eticheta {
    int nr_eticheta;
    int hash;
    server_memory* server;
    int id;
};

typedef struct eticheta eticheta;

struct load_balancer {
    /* TODO 0 */
    eticheta** servers_et;
    int nr_servers;
};

unsigned int hash_function_servers(void *a) {
    unsigned int uint_a = *((unsigned int *)a);

    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = (uint_a >> 16u) ^ uint_a;
    return uint_a;
}

unsigned int hash_function_key(void *a) {
    unsigned char *puchar_a = (unsigned char *)a;
    unsigned int hash = 5381;
    int c;

    while ((c = *puchar_a++))
        hash = ((hash << 5u) + hash) + c;

    return hash;
}

load_balancer *init_load_balancer() {
    /* TODO 1 */
    load_balancer* load_b = malloc(sizeof(load_balancer));
    load_b->nr_servers = 0;
    load_b->servers_et = NULL;

    return load_b;
}

void loader_add_server(load_balancer *main, int server_id) {
    /* TODO 2 */
    if(!main)
        return;

    // Server init
    server_memory* new_server = init_server_memory();
    new_server->id = server_id;


    // ===== ETICHETE =====
    if(main->nr_servers == 0){ // First server

        main->nr_servers = 1;

        main->servers_et = malloc(sizeof(eticheta*) * 3);

        eticheta* new_et = malloc(sizeof(eticheta));
        new_et->id = server_id;
        new_et->server = new_server;

        for(int i = 0 ; i <= 2 ; i++){
            new_et->nr_eticheta = i * 100000 + new_server->id;
            new_et->hash = hash_function_servers(&new_et->nr_eticheta);
            memcpy(main->servers_et[i],new_et,sizeof(eticheta*));
        }
        free(new_et);

    }
    else
    {
        main->nr_servers++;
        
        //REDIM
        eticheta **aux;
        aux = realloc(main->servers_et, sizeof(eticheta*) * main->nr_servers * 3);
        if(aux)
            main->servers_et = aux;

        
        //Put the new "etichete"
        eticheta* new_et = malloc(sizeof(eticheta));
        new_et->id = server_id;
        new_et->server = new_server;

        for(int i=0;i<=2;i++){
            new_et->nr_eticheta = i * 100000 + new_server->id;
            new_et->hash = hash_function_servers(&new_et->nr_eticheta);

            int j,k;
            for(j = 0; j < (main->nr_servers - 1) * 3; j++){ // pos to put eticheta
                if (new_et->hash == main->servers_et[j]->hash) {
                    if(new_et->id > main->servers_et[j]->id)
                        j++;
                    break;
                }
                if(new_et->hash < main->servers_et[j]->hash)
                    break;
            }

            for (k = (main->nr_servers - 1) * 3 + i - 1 ; k >= j; j--)
                main->servers_et[k + 1] = main->servers_et[k];

            memcpy(main->servers_et[j],new_et,sizeof(eticheta*));
        }
        free(new_et);

    }

    // ===== ELEMENTE =====

    
}

void loader_remove_server(load_balancer *main, int server_id) {
    /* TODO 3 */
}

void loader_store(load_balancer *main, char *key, char *value, int *server_id) {
    /* TODO 4 */
    if(!main)
        return;

    int hash_obj = hash_function_key(key);
    for(int i=0;i<main->nr_servers * 3;i++)
        if(main->servers_et[i]<hash_obj && hash_obj<main->servers_et[i+1])
            {
                *server_id = main->servers_et[i]->id;
                server_store(main->servers_et[i]->server,key,value);
            }
}

char *loader_retrieve(load_balancer *main, char *key, int *server_id) {
    /* TODO 5 */
    if(!main)
        return NULL;

    int hash_obj = hash_function_key(key);
    for(int i=0;i<main->nr_servers * 3;i++)
        if(main->servers_et[i]<hash_obj && hash_obj<main->servers_et[i+1])
            {
                *server_id = main->servers_et[i]->id;
                char* value = server_retrieve(main->servers_et[i]->server,key);
                return value;
            }
}

void free_load_balancer(load_balancer *main) {
    /* TODO 6 */
    
}
