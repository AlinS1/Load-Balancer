/* Copyright 2023 Similea Alin-Andrei 314CA */
#include "load_balancer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list_ht.h"
#include "server.h"
#include "utils.h"

#define NR_L 3 	// number of labels for each server
#define UINT_MAX 4294967295

struct label {
	server_memory *server;
	int id_server;
	int nr_label;
	unsigned int hash;
};

typedef struct label label;

struct load_balancer {
	label **hash_ring; 	// array of pointers to labels
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
	DIE(!load_b, "malloc failed");
	load_b->nr_servers = 0;
	load_b->hash_ring = NULL;

	return load_b;
}

void loader_add_server(load_balancer *main, int server_id)
{
	if (!main)
		return;

	// Server initialization
	server_memory *new_server = init_server_memory();
	new_server->id = server_id;

	// ===== LABELS =====
	if (main->nr_servers == 0) {  // First added server
		main->nr_servers = 1;
		main->hash_ring = malloc(sizeof(label *) * NR_L);
		DIE(!main->hash_ring, "malloc failed");

		// Create and put the labels in the hash ring
		for (int i = 0; i <= 2; i++) {
			label *new_label = malloc(sizeof(label));
			DIE(!new_label, "malloc failed");

			new_label->id_server = new_server->id;
			new_label->server = new_server;
			new_label->nr_label = i * 100000 + new_server->id;
			new_label->hash = hash_function_servers(&new_label->nr_label);
			main->hash_ring[i] = new_label;
		}

		order_labels(main);	 // Order the labels in the hash ring.

	} else {  // The server to add is not the first to have been added
		main->nr_servers++;

		// Resize the array of labels.
		label **aux;
		aux =
			realloc(main->hash_ring, sizeof(label *) * main->nr_servers * NR_L);
		DIE(!aux, "realloc failed");

		if (aux)
			main->hash_ring = aux;
		for (int i = (main->nr_servers - 1) * NR_L; i < main->nr_servers * NR_L;
			 i++)
			main->hash_ring[i] = NULL;

		// Create and put the new labels in the array
		for (int i = 0; i <= 2; i++) {
			label *new_label = malloc(sizeof(label));
			DIE(!new_label, "malloc failed");

			new_label->server = new_server;
			new_label->id_server = new_server->id;
			new_label->nr_label = i * 100000 + new_server->id;
			new_label->hash = hash_function_servers(&new_label->nr_label);

			int pos = find_pos_for_label(main, i, new_label);
			int current_total_elements = (main->nr_servers - 1) * NR_L + i;

			// Move elements to the right and make room for the new label.
			for (int k = current_total_elements - 1; k >= pos; k--)
				main->hash_ring[k + 1] = main->hash_ring[k];

			main->hash_ring[pos] = new_label;
		}
	}

	// ===== OBJECTS =====
	int added_server_id = new_server->id;
	for (int i = 0; i < main->nr_servers * NR_L; i++) {
		// For each label, if applicable, we transfer objects from the following
		// label that will have a smaller hash than the label's hash.
		if (main->hash_ring[i]->id_server == added_server_id) {
			if (i + 1 < main->nr_servers * NR_L &&
				main->hash_ring[i + 1]->id_server == added_server_id)
				continue;
			else
				cases_move_objects_for_add_server(main, i);
		}
	}
}

void loader_remove_server(load_balancer *main, int server_id)
{
	if (!main)
		return;

	// First transfer the objects of the servers to be removed
	int moved_first_n_last = 0;
	for (int i = 0; i < main->nr_servers * NR_L; i++) {
		if (main->hash_ring[i]->id_server == server_id) {
			// Do not transfer from a server to the same server
			if (i + 1 < main->nr_servers * NR_L &&
				main->hash_ring[i + 1]->id_server == server_id) {
				continue;
			}
			if (i == main->nr_servers * NR_L - 1 && moved_first_n_last == 1)
				continue;
			if (i == 0)
				moved_first_n_last =
					cases_move_objects_for_remove_server(main, i, server_id);
			else
				cases_move_objects_for_remove_server(main, i, server_id);
		}
	}

	// Free the memory of the labels that need to be removed.
	for (int i = 0; i < main->nr_servers * NR_L; i++) {
		if (main->hash_ring[i]->id_server == server_id) {
			// Free the server memory (only once)
			if (main->hash_ring[i]->nr_label == main->hash_ring[i]->id_server) {
				free_server_memory(main->hash_ring[i]->server);
				main->hash_ring[i]->server = NULL;
			}
			// Free the labels
			free(main->hash_ring[i]);
			main->hash_ring[i] = NULL;
		}
	}

	// Move the labels to the left in order to position the NULL elements at the
	// end of the array.
	for (int i = 0; i < (main->nr_servers - 1) * NR_L; i++) {
		if (main->hash_ring[i] == NULL) {
			// Look for a non-NULL element and swap
			int j = i + 1;
			while (j < main->nr_servers * NR_L && main->hash_ring[j] == NULL) {
				j++;
			}
			if (j < main->nr_servers * NR_L) {
				main->hash_ring[i] = main->hash_ring[j];
				main->hash_ring[j] = NULL;
			}
		}
	}

	// Resize the array of labels.
	main->nr_servers--;
	label **aux;
	aux = realloc(main->hash_ring, sizeof(label *) * main->nr_servers * NR_L);
	DIE(!aux, "realloc failed");

	if (aux)
		main->hash_ring = aux;
}

void loader_store(load_balancer *main, char *key, char *value, int *server_id)
{
	if (!main)
		return;

	unsigned int hash_obj = hash_function_key(key);

	// If the object's hash will be before the first label's or after the last
	// label's we will store it in the first label's server (because of the
	// circular structure of the hash_ring)
	if (hash_obj <= main->hash_ring[0]->hash ||
		hash_obj > main->hash_ring[main->nr_servers * NR_L - 1]->hash) {
		*server_id = main->hash_ring[0]->server->id;
		server_store(main->hash_ring[0]->server, key, value);
		return;
	}

	for (int i = 1; i < main->nr_servers * NR_L; i++)
		if (hash_obj <= main->hash_ring[i]->hash) {
			*server_id = main->hash_ring[i]->server->id;
			server_store(main->hash_ring[i]->server, key, value);
			return;
		}
}

char *loader_retrieve(load_balancer *main, char *key, int *server_id)
{
	if (!main || !main->hash_ring)
		return NULL;

	unsigned int hash_obj = hash_function_key(key);

	// If the object's hash is before the first label's or after the last
	// label's, it means the object was stored in the first label's server
	// (because of the circular structure of the hash_ring)
	if (hash_obj <= main->hash_ring[0]->hash ||
		hash_obj > main->hash_ring[main->nr_servers * NR_L - 1]->hash) {
		*server_id = main->hash_ring[0]->server->id;
		char *value = NULL;
		value = server_retrieve(main->hash_ring[0]->server, key);
		return value;
	}

	for (int i = 1; i < main->nr_servers * NR_L; i++)
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
	for (int i = 0; i < main->nr_servers * NR_L; i++) {
		// Because all 3 labels have a pointer to the same server, we need to
		// free the memory of a server only once, for the original label that
		// has the number equal to the server's id.
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

void cases_move_objects_for_add_server(load_balancer *main, int i)
{
	if (i == 0) {
		// If we add a server on pos 0, we need to move the elements
		// from hash[last]... UINT_MAX & 0 ... hash[0] from server[1] to
		// server[0]
		move_objects_ht_by_hash(main->hash_ring[0]->server->ht,
								main->hash_ring[1]->server->ht,
								main->hash_ring[0]->hash, 0);
		move_objects_ht_by_hash(
			main->hash_ring[0]->server->ht, main->hash_ring[1]->server->ht,
			UINT_MAX, main->hash_ring[main->nr_servers * NR_L - 1]->hash);
		return;
	}

	if (i == main->nr_servers * NR_L - 1) {
		// If we add a server on pos last, we need to move the elements
		// from hash[last-1] ... hash[last] from server[0] to
		// server[last]
		move_objects_ht_by_hash(
			main->hash_ring[i]->server->ht, main->hash_ring[0]->server->ht,
			main->hash_ring[i]->hash, main->hash_ring[i - 1]->hash);
		return;
	}

	// If we add a server on a random pos, we need to move the elements
	// from hash[pos-1] ... hash[pos] from server[pos + 1] to
	// server[pos]
	move_objects_ht_by_hash(
		main->hash_ring[i]->server->ht, main->hash_ring[i + 1]->server->ht,
		main->hash_ring[i]->hash, main->hash_ring[i - 1]->hash);
}

void order_labels(load_balancer *main)
{
	for (int i = 0; i < 2; i++) {
		unsigned int hash_min = main->hash_ring[i]->hash;
		int pos_hash_min = i;
		for (int j = i + 1; j < NR_L; j++) {
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
}

int find_pos_for_label(load_balancer *main, int i, label *new_label)
{
	int pos = 0;
	int current_total_elements = (main->nr_servers - 1) * NR_L + i;
	for (pos = 0; pos < current_total_elements; pos++) {
		// If the hashes are the same, order by server id
		if (new_label->hash == main->hash_ring[pos]->hash) {
			if (new_label->server->id > main->hash_ring[pos]->server->id)
				pos++;
			break;
		}
		// Break the loop when we find a smaller hash.
		if (new_label->hash < main->hash_ring[pos]->hash)
			break;
		if (pos == current_total_elements - 1 &&
			new_label->hash > main->hash_ring[pos]->hash) {
			pos++;
			break;
		}
	}
	return pos;
}

int cases_move_objects_for_remove_server(load_balancer *main, int i,
										 int server_id)
{
	if (i == 0 &&
		main->hash_ring[main->nr_servers * NR_L - 1]->id_server == server_id) {
		// If we remove the first and last server, we need to move the
		// elements from hash[last - 1] ... hash [last] from
		// server[last] -> server[1]
		// and from hash[last] ... U_INTMAX & 0 ... hash[0] from
		// server[0] to server[1]

		move_objects_ht_by_hash(
			main->hash_ring[1]->server->ht,
			main->hash_ring[main->nr_servers * NR_L - 1]->server->ht,
			main->hash_ring[main->nr_servers * NR_L - 1]->hash,
			main->hash_ring[main->nr_servers * NR_L - 2]->hash);
		move_objects_ht_by_hash(
			main->hash_ring[1]->server->ht, main->hash_ring[0]->server->ht,
			UINT_MAX, main->hash_ring[main->nr_servers * NR_L - 1]->hash);
		move_objects_ht_by_hash(main->hash_ring[1]->server->ht,
								main->hash_ring[0]->server->ht,
								main->hash_ring[0]->hash, 0);
		return 1;  // keeps track whether we have moved the last
				   // server's objects
	}

	if (i == 0) {
		// If we remove the first server, we need to move the
		// elements from hash[last] ... U_INTMAX & 0 ... hash[0] to
		// server[1]
		move_objects_ht_by_hash(
			main->hash_ring[1]->server->ht, main->hash_ring[0]->server->ht,
			UINT_MAX, main->hash_ring[main->nr_servers * NR_L - 1]->hash);
		move_objects_ht_by_hash(main->hash_ring[1]->server->ht,
								main->hash_ring[0]->server->ht,
								main->hash_ring[0]->hash, 0);
		return 0;
	}

	if (i == main->nr_servers * NR_L - 1) {
		// If we remove the last server, we need to move the
		// elements from hash[last - 1] ... hash[last] to server[0]
		move_objects_ht_by_hash(
			main->hash_ring[0]->server->ht, main->hash_ring[i]->server->ht,
			main->hash_ring[i]->hash, main->hash_ring[i - 1]->hash);
		return 0;
	}
	// If we remove a random server, we need to move the
	// elements from hash[pos - 1] ... hash[pos] to server[pos + 1]
	move_objects_ht_by_hash(
		main->hash_ring[i + 1]->server->ht, main->hash_ring[i]->server->ht,
		main->hash_ring[i]->hash, main->hash_ring[i - 1]->hash);
	return 0;
}
