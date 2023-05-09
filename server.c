/* Copyright 2023 <> */
#include "server.h"

#include <stdlib.h>
#include <string.h>

#include "list_ht.h"
#include "load_balancer.h"

#define HMAX 40

server_memory *init_server_memory()
{
	/* TODO 1 */
	server_memory *new_server = malloc(sizeof(server_memory));
	new_server->ht = ht_create(HMAX, hash_function_key,
							   compare_function_strings, key_val_free_function);
	return new_server;
}

void server_store(server_memory *server, char *key, char *value)
{
	/* TODO 2 */
	ht_put(server->ht, key, strlen(key) + 1, value, strlen(value) + 1);
	// printf("\nPUS: %s\n",(char*)ht_get(server->ht,key));
}

char *server_retrieve(server_memory *server, char *key)
{
	/* TODO 3 */
	return (char *)ht_get(server->ht, key);
}

void server_remove(server_memory *server, char *key)
{
	/* TODO 4 */
}

void free_server_memory(server_memory *server)
{
	/* TODO 5 */

	ht_free(server->ht);
	server->ht = NULL;
	free(server);
}
