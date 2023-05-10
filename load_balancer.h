/* Copyright 2023 Similea Alin-Andrei 314CA */
#ifndef LOAD_BALANCER_H_
#define LOAD_BALANCER_H_

#include "server.h"

struct label;
typedef struct label label;

struct load_balancer;
typedef struct load_balancer load_balancer;

unsigned int hash_function_key(void *a);

/**
 * init_load_balancer() - initializes the memory for a new load balancer and its
 * fields and returns a pointer to it
 *
 * Return: pointer to the load balancer struct
 */
load_balancer *init_load_balancer();

/**
 * free_load_balancer() - frees the memory of every field that is related to the
 * load balancer (servers, hashring)
 *
 * @arg1: Load balancer to free
 */
void free_load_balancer(load_balancer *main);

/**
 * load_store() - Stores the key-value pair inside the system.
 * @arg1: Load balancer which distributes the work.
 * @arg2: Key represented as a string.
 * @arg3: Value represented as a string.
 * @arg4: This function will RETURN via this parameter
 *        the server ID which stores the object.
 *
 * The load balancer will use Consistent Hashing to distribute the
 * load across the servers. The chosen server ID will be returned
 * using the last parameter.
 *
 * Hint:
 * Search the hashring associated to the load balancer to find the server where
 * the entry should be stored and call the function to store the entry on the
 * respective server.
 *
 */
void loader_store(load_balancer *main, char *key, char *value, int *server_id);

/**
 * load_retrieve() - Gets a value associated with the key.
 * @arg1: Load balancer which distributes the work.
 * @arg2: Key represented as a string.
 * @arg3: This function will RETURN the server ID
		  which stores the value via this parameter.
 *
 * The load balancer will search for the server which should posess the
 * value associated to the key. The server will return NULL in case
 * the key does NOT exist in the system.
 *
 * Hint:
 * Search the hashring associated to the load balancer to find the server where
 the entry
 * should be stored and call the function to store the entry on the respective
 server.
 */
char *loader_retrieve(load_balancer *main, char *key, int *server_id);

/**
 * load_add_server() - Adds a new server to the system.
 * @arg1: Load balancer which distributes the work.
 * @arg2: ID of the new server.
 *
 * The load balancer will generate 3 replica labels and it will
 * place them inside the hash ring. The neighbor servers will
 * distribute some the objects to the added server.
 *
 * Hint:
 * Resize the servers array to add a new one.
 * Add each label in the hashring in its appropiate position.
 * Do not forget to resize the hashring and redistribute the objects
 * after each label add (the operations will be done 3 times, for each replica).
 */
void loader_add_server(load_balancer *main, int server_id);

/**
 * load_remove_server() - Removes a specific server from the system.
 * @arg1: Load balancer which distributes the work.
 * @arg2: ID of the removed server.
 *
 * The load balancer will distribute ALL objects stored on the
 * removed server and will delete ALL replicas from the hash ring.
 *
 */
void loader_remove_server(load_balancer *main, int server_id);

// Handles the cases for object moving when we add a new server.
void cases_move_objects_for_add_server(load_balancer *main, int i);

// Order the labels in the hash ring by looking for the server that has
// the smallest hash and putting it at the beginning.
void order_labels(load_balancer *main);

// Because the labels need to be in ascending order, we firstly need
// to find the proper position to put the label on.
int find_pos_for_label(load_balancer *main, int i, label *new_label);

// Handles the cases for object moving when we remove a server.
// The function returns 1 if both the first and last labels need to be
// removed.
int cases_move_objects_for_remove_server(load_balancer *main, int i,
										 int server_id);

#endif 	// LOAD_BALANCER_H_
