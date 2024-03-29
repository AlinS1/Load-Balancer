/* Copyright 2023 Similea Alin-Andrei 314CA */
#include "server.h"

#include <stdlib.h>
#include <string.h>

#include "list_ht.h"
#include "load_balancer.h"
#include "utils.h"

#define HMAX 40

server_memory *init_server_memory()
{
	server_memory *new_server = malloc(sizeof(server_memory));
	DIE(!new_server, "malloc failed");
	new_server->ht = ht_create(HMAX, hash_function_key,
							   compare_function_strings, key_val_free_function);
	return new_server;
}

void server_store(server_memory *server, char *key, char *value)
{
	ht_put(server->ht, key, strlen(key) + 1, value, strlen(value) + 1);
}

char *server_retrieve(server_memory *server, char *key)
{
	if (!server || ht_has_key(server->ht, key) == 0)
		return NULL;
	return (char *)ht_get(server->ht, key);
}

void server_remove(server_memory *server, char *key)
{
	if (!ht_has_key(server->ht, key))
		return;
	ht_remove_entry(server->ht, key);
}

void free_server_memory(server_memory *server)
{
	ht_free(server->ht);
	server->ht = NULL;
	free(server);
}
