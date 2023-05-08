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
    printf("server_id: %d\n", server_id);
    server_memory* new_server = init_server_memory();
    new_server->id = server_id;


    // ===== ETICHETE =====
    if(main->nr_servers == 0){ // First server

        main->nr_servers = 1;

        main->servers_et = malloc(sizeof(eticheta*) * 3);

        

        for(int i = 0 ; i <= 2 ; i++){
            eticheta* new_et = malloc(sizeof(eticheta));
            new_et->server = new_server;
            new_et->nr_eticheta = i * 100000 + new_server->id;
            new_et->hash = hash_function_servers(&new_et->nr_eticheta);
            main->servers_et[i] = new_et;
            //free(new_et);
            //memcpy(main->servers_et[i], new_et, sizeof(eticheta*));
        }
        

    }
    else
    {
        main->nr_servers++;
        
        //REDIM
        eticheta **aux;
        aux = realloc(main->servers_et, sizeof(eticheta*) * main->nr_servers * 3);
        if(aux)
            main->servers_et = aux;
        for(int i = (main->nr_servers -1) *3; i< main->nr_servers * 3; i++)
            main->servers_et[i] = NULL;

        
        //Put the new "etichete"
        for(int i=0;i<=2;i++){
            eticheta* new_et = malloc(sizeof(eticheta));
            new_et->server = new_server;
            new_et->nr_eticheta = i * 100000 + new_server->id;
            new_et->hash = hash_function_servers(&new_et->nr_eticheta);

            int j = 0, k = 0;
            // Find position to put
            //printf("Upper limit:%d\n", (main->nr_servers - 1) * 3);
            for(j = 0; j < (main->nr_servers - 1) * 3; j++){ 
                if (new_et->hash == main->servers_et[j]->hash) {
                    if(new_et->server->id > main->servers_et[j]->server->id)
                        j++;
                    break;
                }
                if(new_et->hash < main->servers_et[j]->hash)
                    break;
            }

            // Move elements to right
            for (k = (main->nr_servers - 1) * 3 + i - 1 ; k >= j; k--)
                main->servers_et[k + 1] = main->servers_et[k];

            

            main->servers_et[j] = new_et;
        }
        

    }


    for(int i = 0 ; i < main->nr_servers * 3 ; i++){
        if(!main->servers_et[i]->server)
            printf("NU e\n");

        printf("i:%d; id:%d ", i, main->servers_et[i]->server->id);
        printf("et:%d ", main->servers_et[i]->nr_eticheta);
        printf("hash: %d\n", main->servers_et[i]->hash);
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

    // for(int i = 0 ; i < main->nr_servers * 3 ; i++){
    //     if(!main->servers_et[i]->server)
    //         printf("NU e\n");

    //     printf("i:%d; id:%d ", i, main->servers_et[i]->server->id);
    //     printf("et:%d ", main->servers_et[i]->nr_eticheta);
    //     printf("hash: %d\n", main->servers_et[i]->hash);
    // }

    int hash_obj = hash_function_key(key);

    if (hash_obj < main->servers_et[0]){
        *server_id = main->servers_et[0]->server->id;
        server_store(main->servers_et[0]->server,key,value);
        return;
    }

    for(int i = 0; i < main->nr_servers * 3 - 1; i++)
        if(main->servers_et[i]<hash_obj && hash_obj<=main->servers_et[i+1])
            {
                *server_id = main->servers_et[i+1]->server->id;
                server_store(main->servers_et[i+1]->server,key,value);
                return;
            }
    if (hash_obj > main->servers_et[main->nr_servers * 3 - 1]->hash){
        *server_id = main->servers_et[0]->server->id;
        server_store(main->servers_et[0]->server,key,value);
        return;
    }

}

char *loader_retrieve(load_balancer *main, char *key, int *server_id) {
    /* TODO 5 */
    //if(!main || !main->servers_et)
    //    return NULL;

    // for(int i = 0 ; i < main->nr_servers * 3 ; i++){
    //     if(!main->servers_et[i]->server)
    //         printf("NU e\n");

    //     printf("i:%d; id:%d ", i, main->servers_et[i]->server->id);
    //     printf("et:%d ", main->servers_et[i]->nr_eticheta);
    //     printf("hash: %d\n", main->servers_et[i]->hash);
    // }

    int hash_obj = hash_function_key(key);

    int i;
    //Before first "eticheta"
    if (hash_obj < main->servers_et[0]){
        *server_id = main->servers_et[0]->server->id;
        char* value = server_retrieve(main->servers_et[0]->server,key);
        return value;
    }

    for(i=0;i<main->nr_servers * 3 - 1;i++)
        if(main->servers_et[i]<hash_obj && hash_obj<main->servers_et[i+1])
            {
                *server_id = main->servers_et[i+1]->server->id;
                char* value = server_retrieve(main->servers_et[i+1]->server,key);
                return value;
            }

    //After last "eticheta"
    if(hash_obj > main->servers_et[main->nr_servers * 3 - 1])
        {
            *server_id = main->servers_et[0]->server->id;
            char* value = server_retrieve(main->servers_et[0]->server,key);
            return value;
        }
}

void free_load_balancer(load_balancer *main) {
    /* TODO 6 */
    
}
