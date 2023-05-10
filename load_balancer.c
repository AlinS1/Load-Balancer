/* Copyright 2023 <> */
#include "load_balancer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list_ht.h"
#include "server.h"

#define MIN_HASH 0
#define MAX_HASH 360
#define UINT_MAX 4294967295

struct eticheta {
	int nr_eticheta;
	unsigned int hash;
	server_memory *server;
	int id_server;
};

typedef struct eticheta eticheta;

struct load_balancer {
	/* TODO 0 */
	eticheta **servers_et;
	int nr_servers;
};

unsigned int hash_function_servers(void *a)
{
	unsigned int uint_a = *((unsigned int *)a);

	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = (uint_a >> 16u) ^ uint_a;
	return uint_a;
}

unsigned int hash_function_key(void *a)
{
	unsigned char *puchar_a = (unsigned char *)a;
	unsigned int hash = 5381;
	int c;

	while ((c = *puchar_a++))
		hash = ((hash << 5u) + hash) + c;

	return hash;
}

load_balancer *init_load_balancer()
{
	/* TODO 1 */
	load_balancer *load_b = malloc(sizeof(load_balancer));
	load_b->nr_servers = 0;
	load_b->servers_et = NULL;

	return load_b;
}

void loader_add_server(load_balancer *main, int server_id)
{
	/* TODO 2 */
	if (!main)
		return;

	// Server init
	// printf("server_id: %d\n", server_id);
	server_memory *new_server = init_server_memory();
	new_server->id = server_id;

	// ===== ETICHETE =====
	if (main->nr_servers == 0) {  // First server

		main->nr_servers = 1;

		main->servers_et = malloc(sizeof(eticheta *) * 3);

		for (int i = 0; i <= 2; i++) {
			eticheta *new_et = malloc(sizeof(eticheta));
			new_et->id_server = new_server->id;
			new_et->server = new_server;
			new_et->nr_eticheta = i * 100000 + new_server->id;
			new_et->hash = hash_function_servers(&new_et->nr_eticheta);
			main->servers_et[i] = new_et;
			// free(new_et);
			// memcpy(main->servers_et[i], new_et, sizeof(eticheta*));
		}

		for (int i = 0; i < 2; i++) {
			unsigned int hash_min = main->servers_et[i]->hash;
			int pos_hash_min = i;
			for (int j = i + 1; j < 3; j++) {
				if (hash_min > main->servers_et[j]->hash) {
					hash_min = main->servers_et[j]->hash;
					pos_hash_min = j;
				}
			}
			if (i != pos_hash_min) {
				eticheta *aux = main->servers_et[i];
				main->servers_et[i] = main->servers_et[pos_hash_min];
				main->servers_et[pos_hash_min] = aux;
			}
		}

	} else {
		main->nr_servers++;

		// REDIM
		eticheta **aux;
		aux = realloc(main->servers_et,
					  sizeof(eticheta *) * main->nr_servers * 3);
		if (aux)
			main->servers_et = aux;
		for (int i = (main->nr_servers - 1) * 3; i < main->nr_servers * 3; i++)
			main->servers_et[i] = NULL;

		// Put the new "etichete"
		for (int i = 0; i <= 2; i++) {
			eticheta *new_et = malloc(sizeof(eticheta));
			new_et->server = new_server;
			new_et->id_server = new_server->id;
			new_et->nr_eticheta = i * 100000 + new_server->id;
			new_et->hash = hash_function_servers(&new_et->nr_eticheta);

			int j = 0, k = 0;
			// Find position to put the element on
			int current_total_elements = (main->nr_servers - 1) * 3 + i;
			for (j = 0; j < current_total_elements; j++) {
				if (new_et->hash == main->servers_et[j]->hash) {
					if (new_et->server->id > main->servers_et[j]->server->id)
						j++;
					break;
				}
				if (new_et->hash < main->servers_et[j]->hash)
					break;
				if (j == current_total_elements - 1 &&
					new_et->hash > main->servers_et[j]->hash) {
					j++;
					break;
				}
			}

			// Move elements to right
			for (k = current_total_elements - 1; k >= j; k--)
				main->servers_et[k + 1] = main->servers_et[k];

			main->servers_et[j] = new_et;
			// aici am putea sa mutam
		}
	}

	// printf("\n========\nLOADED_sv:%d\n=========\n", new_server->id);
	// for (int i = 0; i < main->nr_servers * 3; i++) {
	// 	if (main->servers_et[i]->server == NULL)
	// 		printf("NU e\n");

	// 	printf("i:%d; id:%d ", i, main->servers_et[i]->server->id);
	// 	printf("et:%d ", main->servers_et[i]->nr_eticheta);
	// 	printf("hash: %u\n", main->servers_et[i]->hash);
	// }

	// ===== ELEMENTE =====
	int added_server_id = new_server->id;
	for (int i = 0; i < main->nr_servers * 3; i++) {
		if (main->servers_et[i]->id_server == added_server_id) {
			if (i + 1 < main->nr_servers * 3 &&
				main->servers_et[i + 1]->id_server == added_server_id) {
				continue;
			}

			if (i == 0) {
				// If we add a server on pos 0, we need to move the elements
				// from hash[last]... UINT_MAX & 0 ... hash[0] from server[1] to
				// server[0]
				move_objects_ht_by_hash(main->servers_et[0]->server->ht,
										main->servers_et[1]->server->ht,
										main->servers_et[0]->hash, 0);
				move_objects_ht_by_hash(
					main->servers_et[0]->server->ht,
					main->servers_et[1]->server->ht, UINT_MAX,
					main->servers_et[main->nr_servers * 3 - 1]->hash);
				continue;
			}

			if (i == main->nr_servers * 3 - 1) {
				// If we add a server on pos last, we need to move the elements
				// from hash[last-1] ... hash[last] from server[0] to
				// server[last]
				move_objects_ht_by_hash(main->servers_et[i]->server->ht,
										main->servers_et[0]->server->ht,
										main->servers_et[i]->hash,
										main->servers_et[i - 1]->hash);
				continue;
			}
			// If we add a server on a random pos, we need to move the elements
			// from hash[pos-1] ... hash[pos] from server[pos + 1] to
			// server[pos]
			move_objects_ht_by_hash(main->servers_et[i]->server->ht,
									main->servers_et[i + 1]->server->ht,
									main->servers_et[i]->hash,
									main->servers_et[i - 1]->hash);
		}
	}
}

void loader_remove_server(load_balancer *main, int server_id)
{
	/* TODO 3 */
	if (!main)
		return;
	// printf("\n====removed_sv_id:%d ====\n", server_id);

	// printf("INAINTE:\n");
	// for (int i = 0; i < main->nr_servers * 3; i++) {
	// 	if (!main->servers_et[i]->server)
	// 		printf("NU e\n");

	// 	printf("i:%d; id:%d ", i, main->servers_et[i]->server->id);
	// 	printf("et:%d ", main->servers_et[i]->nr_eticheta);
	// 	printf("hash: %d\n", main->servers_et[i]->hash);
	// }

	// Problem if last si first sunt de eliminat. ???
	for (int i = 0; i < main->nr_servers * 3; i++) {
		if (main->servers_et[i]->id_server == server_id) {
			// !!! next server should not be the same server
			if (i + 1 < main->nr_servers * 3 &&
				main->servers_et[i + 1]->id_server == server_id) {
				continue;
			}

			if (i == 0) {
				// If we remove the first server, we need to move the
				// elements from hash[last] ... U_INTMAX & 0 ... hash[0] to
				// server[1]
				move_objects_ht_by_hash(
					main->servers_et[1]->server->ht,
					main->servers_et[0]->server->ht, UINT_MAX,
					main->servers_et[main->nr_servers * 3 - 1]->hash);
				move_objects_ht_by_hash(main->servers_et[1]->server->ht,
										main->servers_et[0]->server->ht,
										main->servers_et[0]->hash, 0);
				continue;
			}

			if (i == main->nr_servers * 3 - 1) {
				// If we remove the last server, we need to move the
				// elements from hash[last - 1] ... hash[last] to server[0]
				move_objects_ht_by_hash(main->servers_et[0]->server->ht,
										main->servers_et[i]->server->ht,
										main->servers_et[i]->hash,
										main->servers_et[i - 1]->hash);
				continue;
			}  // moves elements to the next server in hashring
			// If we remove a random server, we need to move the
			// elements from hash[pos - 1] ... hash[pos] to server[pos + 1]
			move_objects_ht_by_hash(main->servers_et[i + 1]->server->ht,
									main->servers_et[i]->server->ht,
									main->servers_et[i]->hash,
									main->servers_et[i - 1]->hash);
		}
	}

	for (int i = 0; i < main->nr_servers * 3; i++) {
		if (main->servers_et[i]->id_server == server_id) {
			// free the server memory (once)
			if (main->servers_et[i]->nr_eticheta ==
				main->servers_et[i]->id_server) {
				free_server_memory(main->servers_et[i]->server);
				main->servers_et[i]->server = NULL;
			}
			// free the etichete
			free(main->servers_et[i]);
			main->servers_et[i] = NULL;
		}
	}

	// move etichete

	for (int i = 0; i < (main->nr_servers - 1) * 3; i++) {
		if (main->servers_et[i] == NULL) {
			int j = i + 1;
			while (j < main->nr_servers * 3 && main->servers_et[j] == NULL) {
				j++;
			}
			if (j < main->nr_servers * 3) {
				main->servers_et[i] = main->servers_et[j];
				main->servers_et[j] = NULL;
			}
		}
	}

	// realloc etichete
	main->nr_servers--;
	eticheta **aux;
	aux = realloc(main->servers_et, sizeof(eticheta *) * main->nr_servers * 3);
	if (aux)
		main->servers_et = aux;

	// printf("DUPA:\n");
	// for (int i = 0; i < main->nr_servers * 3; i++) {
	// 	if (main->servers_et[i]->server == NULL)
	// 		printf("NU e\n");

	// 	printf("i:%d; id:%d ", i, main->servers_et[i]->server->id);
	// 	printf("et:%d ", main->servers_et[i]->nr_eticheta);
	// 	printf("hash: %d\n", main->servers_et[i]->hash);
	// }
}

void loader_store(load_balancer *main, char *key, char *value, int *server_id)
{
	/* TODO 4 */
	if (!main)
		return;

	// for(int i = 0 ; i < main->nr_servers * 3 ; i++){
	//     if(!main->servers_et[i]->server)
	//         printf("NU e\n");

	//     printf("i:%d; id:%d ", i, main->servers_et[i]->server->id);
	//     printf("et:%d ", main->servers_et[i]->nr_eticheta);
	//     printf("hash: %d\n", main->servers_et[i]->hash);
	// }

	unsigned int hash_obj = hash_function_key(key);

	// If the object's hash is bigger than the last server's, we will
	// put it in the first server (because of the circle structure of the
	// hashring).
	if (hash_obj <= main->servers_et[0]->hash ||
		hash_obj > main->servers_et[main->nr_servers * 3 - 1]->hash) {
		*server_id = main->servers_et[0]->server->id;
		server_store(main->servers_et[0]->server, key, value);
		return;
	}

	for (int i = 0; i < main->nr_servers * 3 - 1; i++)
		if (main->servers_et[i]->hash < hash_obj &&
			hash_obj <= main->servers_et[i + 1]->hash) {
			*server_id = main->servers_et[i + 1]->server->id;
			server_store(main->servers_et[i + 1]->server, key, value);
			return;
		}
}

char *loader_retrieve(load_balancer *main, char *key, int *server_id)
{
	/* TODO 5 */
	if (!main || !main->servers_et)
		return NULL;

	unsigned int hash_obj = hash_function_key(key);
	//printf("\n=====\nhash_obj:%u; key:%s\n====\n", hash_obj, key);
	int i;
	// Before first "eticheta" or after last "eticheta" (because of the circling
	// structure of the hashring)
	if (hash_obj <= main->servers_et[0]->hash ||
		hash_obj > main->servers_et[main->nr_servers * 3 - 1]->hash) {
		*server_id = main->servers_et[0]->server->id;
		char *value = server_retrieve(main->servers_et[0]->server, key);
		return value;
	}

	for (i = 0; i < main->nr_servers * 3 - 1; i++)
		if (hash_obj <= main->servers_et[i + 1]->hash) {
			*server_id = main->servers_et[i + 1]->server->id;
			char *value = server_retrieve(main->servers_et[i + 1]->server, key);
			return value;
		}

	return NULL;
}

void free_load_balancer(load_balancer *main)
{
	/* TODO 6 */

	// Because we allocated the server memory only once for 3 different
	// "etichete", after we free the memory of a server, we need to go through
	// the remaining array and NULL the other pointers that have the same
	// server. Then, we separately free the rest of the data.

	for (int i = 0; i < main->nr_servers * 3; i++) {
		// free server just this once, cuz the others have the same pointer
		if (main->servers_et[i]->nr_eticheta ==
			main->servers_et[i]->id_server) {
			free_server_memory(main->servers_et[i]->server);
			main->servers_et[i]->server = NULL;
		}
		free(main->servers_et[i]);
		main->servers_et[i] = NULL;
	}

	free(main->servers_et);
	main->servers_et = NULL;
	free(main);
	main = NULL;
}
