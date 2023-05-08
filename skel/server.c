/* Copyright 2023 <> */
#include <stdlib.h>
#include <string.h>

#include "load_balancer.h"
#include "server.h"
#include "list_ht.h"

#define HMAX 40



server_memory *init_server_memory()
{
	/* TODO 1 */
	server_memory* new_server = malloc(sizeof(server_memory));
	new_server->ht = ht_create(HMAX, hash_function_key,compare_function_chars,
		key_val_free_function);
	return NULL;
}

void server_store(server_memory *server, char *key, char *value) {
	/* TODO 2 */
	ht_put(server->ht, key, strlen(key) + 1, value, strlen(value) + 1);
}

char *server_retrieve(server_memory *server, char *key) {
	/* TODO 3 */
	return ht_get(server->ht,key);
}

void server_remove(server_memory *server, char *key) {
	/* TODO 4 */
}

void free_server_memory(server_memory *server) {
	/* TODO 5 */
}
