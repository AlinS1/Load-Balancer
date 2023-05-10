/* Copyright 2023 Similea Alin-Andrei 314CA */
#include "load_balancer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list_ht.h"
#include "server.h"

#define MIN_HASH 0
#define MAX_HASH 360
#define UINT_MAX 4294967295

struct label {
	int nr_label;
	unsigned int hash;
	server_memory *server;
	int id_server;
};

typedef struct label label;

struct load_balancer {
	label **hash_ring;
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
	load_balancer *load_b = malloc(sizeof(load_balancer));
	load_b->nr_servers = 0;
	load_b->hash_ring = NULL;

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
		main->hash_ring = malloc(sizeof(label *) * 3);

		for (int i = 0; i <= 2; i++) {
			label *new_label = malloc(sizeof(label));
			new_label->id_server = new_server->id;
			new_label->server = new_server;
			new_label->nr_label = i * 100000 + new_server->id;
			new_label->hash = hash_function_servers(&new_label->nr_label);
			main->hash_ring[i] = new_label;
			// free(new_label);
			// memcpy(main->hash_ring[i], new_label, sizeof(label*));
		}

		for (int i = 0; i < 2; i++) {
			unsigned int hash_min = main->hash_ring[i]->hash;
			int pos_hash_min = i;
			for (int j = i + 1; j < 3; j++) {
				if (hash_min > main->hash_ring[j]->hash) {
					hash_min = main->hash_ring[j]->hash;
					pos_hash_min = j;
				}
			}
			if (i != pos_hash_min) {
				label *aux = main->hash_ring[i];
				main->hash_ring[i] = main->hash_ring[pos_hash_min];
				main->hash_ring[pos_hash_min] = aux;
			}
		}

	} else {
		main->nr_servers++;

		// REDIM
		label **aux;
		aux = realloc(main->hash_ring, sizeof(label *) * main->nr_servers * 3);
		if (aux)
			main->hash_ring = aux;
		for (int i = (main->nr_servers - 1) * 3; i < main->nr_servers * 3; i++)
			main->hash_ring[i] = NULL;

		// Put the new "etichete"
		for (int i = 0; i <= 2; i++) {
			label *new_label = malloc(sizeof(label));
			new_label->server = new_server;
			new_label->id_server = new_server->id;
			new_label->nr_label = i * 100000 + new_server->id;
			new_label->hash = hash_function_servers(&new_label->nr_label);

			int j = 0, k = 0;
			// Find position to put the element on
			int current_total_elements = (main->nr_servers - 1) * 3 + i;
			for (j = 0; j < current_total_elements; j++) {
				if (new_label->hash == main->hash_ring[j]->hash) {
					if (new_label->server->id > main->hash_ring[j]->server->id)
						j++;
					break;
				}
				if (new_label->hash < main->hash_ring[j]->hash)
					break;
				if (j == current_total_elements - 1 &&
					new_label->hash > main->hash_ring[j]->hash) {
					j++;
					break;
				}
			}

			// Move elements to right
			for (k = current_total_elements - 1; k >= j; k--)
				main->hash_ring[k + 1] = main->hash_ring[k];

			main->hash_ring[j] = new_label;
			// aici am putea sa mutam
		}
	}

	// printf("\n========\nLOADED_sv:%d\n=========\n", new_server->id);
	// for (int i = 0; i < main->nr_servers * 3; i++) {
	// 	if (main->hash_ring[i]->server == NULL)
	// 		printf("NU e\n");

	// 	printf("i:%d; id:%d ", i, main->hash_ring[i]->server->id);
	// 	printf("et:%d ", main->hash_ring[i]->nr_label);
	// 	printf("hash: %u\n", main->hash_ring[i]->hash);
	// }

	// ===== ELEMENTE =====
	int added_server_id = new_server->id;
	for (int i = 0; i < main->nr_servers * 3; i++) {
		if (main->hash_ring[i]->id_server == added_server_id) {
			if (i + 1 < main->nr_servers * 3 &&
				main->hash_ring[i + 1]->id_server == added_server_id) {
				continue;
			}

			if (i == 0) {
				// If we add a server on pos 0, we need to move the elements
				// from hash[last]... UINT_MAX & 0 ... hash[0] from server[1] to
				// server[0]
				move_objects_ht_by_hash(main->hash_ring[0]->server->ht,
										main->hash_ring[1]->server->ht,
										main->hash_ring[0]->hash, 0);
				move_objects_ht_by_hash(
					main->hash_ring[0]->server->ht,
					main->hash_ring[1]->server->ht, UINT_MAX,
					main->hash_ring[main->nr_servers * 3 - 1]->hash);
				continue;
			}

			if (i == main->nr_servers * 3 - 1) {
				// If we add a server on pos last, we need to move the elements
				// from hash[last-1] ... hash[last] from server[0] to
				// server[last]
				move_objects_ht_by_hash(main->hash_ring[i]->server->ht,
										main->hash_ring[0]->server->ht,
										main->hash_ring[i]->hash,
										main->hash_ring[i - 1]->hash);
				continue;
			}
			// If we add a server on a random pos, we need to move the elements
			// from hash[pos-1] ... hash[pos] from server[pos + 1] to
			// server[pos]
			move_objects_ht_by_hash(main->hash_ring[i]->server->ht,
									main->hash_ring[i + 1]->server->ht,
									main->hash_ring[i]->hash,
									main->hash_ring[i - 1]->hash);
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
	// 	if (!main->hash_ring[i]->server)
	// 		printf("NU e\n");

	// 	printf("i:%d; id:%d ", i, main->hash_ring[i]->server->id);
	// 	printf("et:%d ", main->hash_ring[i]->nr_label);
	// 	printf("hash: %d\n", main->hash_ring[i]->hash);
	// }

	// Problem if last si first sunt de eliminat. ???
	for (int i = 0; i < main->nr_servers * 3; i++) {
		if (main->hash_ring[i]->id_server == server_id) {
			// !!! next server should not be the same server
			if (i + 1 < main->nr_servers * 3 &&
				main->hash_ring[i + 1]->id_server == server_id) {
				continue;
			}

			if (i == 0) {
				// If we remove the first server, we need to move the
				// elements from hash[last] ... U_INTMAX & 0 ... hash[0] to
				// server[1]
				move_objects_ht_by_hash(
					main->hash_ring[1]->server->ht,
					main->hash_ring[0]->server->ht, UINT_MAX,
					main->hash_ring[main->nr_servers * 3 - 1]->hash);
				move_objects_ht_by_hash(main->hash_ring[1]->server->ht,
										main->hash_ring[0]->server->ht,
										main->hash_ring[0]->hash, 0);
				continue;
			}

			if (i == main->nr_servers * 3 - 1) {
				// If we remove the last server, we need to move the
				// elements from hash[last - 1] ... hash[last] to server[0]
				move_objects_ht_by_hash(main->hash_ring[0]->server->ht,
										main->hash_ring[i]->server->ht,
										main->hash_ring[i]->hash,
										main->hash_ring[i - 1]->hash);
				continue;
			}  // moves elements to the next server in hashring
			// If we remove a random server, we need to move the
			// elements from hash[pos - 1] ... hash[pos] to server[pos + 1]
			move_objects_ht_by_hash(main->hash_ring[i + 1]->server->ht,
									main->hash_ring[i]->server->ht,
									main->hash_ring[i]->hash,
									main->hash_ring[i - 1]->hash);
		}
	}

	for (int i = 0; i < main->nr_servers * 3; i++) {
		if (main->hash_ring[i]->id_server == server_id) {
			// free the server memory (once)
			if (main->hash_ring[i]->nr_label == main->hash_ring[i]->id_server) {
				free_server_memory(main->hash_ring[i]->server);
				main->hash_ring[i]->server = NULL;
			}
			// free the etichete
			free(main->hash_ring[i]);
			main->hash_ring[i] = NULL;
		}
	}

	// move etichete

	for (int i = 0; i < (main->nr_servers - 1) * 3; i++) {
		if (main->hash_ring[i] == NULL) {
			int j = i + 1;
			while (j < main->nr_servers * 3 && main->hash_ring[j] == NULL) {
				j++;
			}
			if (j < main->nr_servers * 3) {
				main->hash_ring[i] = main->hash_ring[j];
				main->hash_ring[j] = NULL;
			}
		}
	}

	// realloc etichete
	main->nr_servers--;
	label **aux;
	aux = realloc(main->hash_ring, sizeof(label *) * main->nr_servers * 3);
	if (aux)
		main->hash_ring = aux;

	// printf("DUPA:\n");
	// for (int i = 0; i < main->nr_servers * 3; i++) {
	// 	if (main->hash_ring[i]->server == NULL)
	// 		printf("NU e\n");

	// 	printf("i:%d; id:%d ", i, main->hash_ring[i]->server->id);
	// 	printf("et:%d ", main->hash_ring[i]->nr_label);
	// 	printf("hash: %d\n", main->hash_ring[i]->hash);
	// }
}

void loader_store(load_balancer *main, char *key, char *value, int *server_id)
{
	/* TODO 4 */
	if (!main)
		return;

	// for(int i = 0 ; i < main->nr_servers * 3 ; i++){
	//     if(!main->hash_ring[i]->server)
	//         printf("NU e\n");

	//     printf("i:%d; id:%d ", i, main->hash_ring[i]->server->id);
	//     printf("et:%d ", main->hash_ring[i]->nr_label);
	//     printf("hash: %d\n", main->hash_ring[i]->hash);
	// }

	unsigned int hash_obj = hash_function_key(key);
	// printf("\nhash_obj_stored:%u\n", hash_obj);
	// If the object's hash is bigger than the last server's, we will
	// put it in the first server (because of the circle structure of the
	// hashring).
	if (hash_obj <= main->hash_ring[0]->hash ||
		hash_obj > main->hash_ring[main->nr_servers * 3 - 1]->hash) {
		*server_id = main->hash_ring[0]->server->id;
		server_store(main->hash_ring[0]->server, key, value);
		return;
	}

	for (int i = 1; i < main->nr_servers * 3; i++)
		if (hash_obj <= main->hash_ring[i]->hash) {
			*server_id = main->hash_ring[i]->server->id;
			server_store(main->hash_ring[i]->server, key, value);
			return;
		}
}

char *loader_retrieve(load_balancer *main, char *key, int *server_id)
{
	/* TODO 5 */
	if (!main || !main->hash_ring)
		return NULL;

	unsigned int hash_obj = hash_function_key(key);
	// printf("\n=====\nhash_obj:%u; key:%s\n====\n", hash_obj, key);
	int i;
	// Before first "label" or after last "label" (because of the circling
	// structure of the hashring)
	if (hash_obj <= main->hash_ring[0]->hash ||
		hash_obj > main->hash_ring[main->nr_servers * 3 - 1]->hash) {
		*server_id = main->hash_ring[0]->server->id;
		char *value = NULL;
		value = server_retrieve(main->hash_ring[0]->server, key);
		return value;
	}

	for (i = 1; i < main->nr_servers * 3; i++)
		if (hash_obj <= main->hash_ring[i]->hash) {
			*server_id = main->hash_ring[i]->server->id;
			char *value = NULL;
			value = server_retrieve(main->hash_ring[i]->server, key);
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
		if (main->hash_ring[i]->nr_label == main->hash_ring[i]->id_server) {
			free_server_memory(main->hash_ring[i]->server);
			main->hash_ring[i]->server = NULL;
		}
		free(main->hash_ring[i]);
		main->hash_ring[i] = NULL;
	}

	free(main->hash_ring);
	main->hash_ring = NULL;
	free(main);
	main = NULL;
}
