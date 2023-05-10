/* Copyright 2023 Similea Alin-Andrei 314CA */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "load_balancer.h"
#include "utils.h"

#define MAX_STRING_SIZE 256
#define HMAX 50

typedef struct ll_node_t {
	void *data;
	struct ll_node_t *next;
} ll_node_t;

typedef struct linked_list_t {
	ll_node_t *head;
	unsigned int data_size;
	unsigned int size;
} linked_list_t;

linked_list_t *ll_create(unsigned int data_size)
{
	linked_list_t *ll;

	ll = malloc(sizeof(*ll));
	DIE(!ll, "malloc failed");

	ll->head = NULL;
	ll->data_size = data_size;
	ll->size = 0;

	return ll;
}

void ll_add_nth_node(linked_list_t *list, unsigned int n, const void *new_data)
{
	ll_node_t *prev, *curr;
	ll_node_t *new_node;

	if (!list) {
		return;
	}

	if (n > list->size) {
		n = list->size;
	}

	curr = list->head;
	prev = NULL;
	while (n > 0) {
		prev = curr;
		curr = curr->next;
		--n;
	}

	new_node = malloc(sizeof(*new_node));
	DIE(!new_node, "malloc failed");

	new_node->data = malloc(list->data_size);
	DIE(!new_node->data, "malloc failed");

	memcpy(new_node->data, new_data, list->data_size);

	new_node->next = curr;
	if (prev == NULL) {
		list->head = new_node;
	} else {
		prev->next = new_node;
	}

	list->size++;
}

ll_node_t *ll_remove_nth_node(linked_list_t *list, unsigned int n)
{
	ll_node_t *prev, *curr;

	if (!list || !list->head) {
		return NULL;
	}

	if (n > list->size - 1) {
		n = list->size - 1;
	}

	curr = list->head;
	prev = NULL;
	while (n > 0) {
		prev = curr;
		curr = curr->next;
		--n;
	}

	if (prev == NULL) {
		/* n == 0. */
		list->head = curr->next;
	} else {
		prev->next = curr->next;
	}

	list->size--;

	return curr;
}

unsigned int ll_get_size(linked_list_t *list)
{
	if (!list) {
		return -1;
	}

	return list->size;
}

void ll_free(linked_list_t **pp_list)
{
	ll_node_t *currNode;

	if (!pp_list || !*pp_list) {
		return;
	}

	while (ll_get_size(*pp_list) > 0) {
		currNode = ll_remove_nth_node(*pp_list, 0);
		free(currNode->data);
		currNode->data = NULL;
		free(currNode);
		currNode = NULL;
	}

	free(*pp_list);
	*pp_list = NULL;
}

// ===========================
// =========HASHTABLE=========
// ===========================

typedef struct info info;
struct info {
	void *key;
	void *value;
};

typedef struct hashtable_t hashtable_t;
struct hashtable_t {
	linked_list_t **buckets;
	unsigned int size;
	unsigned int hmax;
	unsigned int (*hash_function)(void *);
	int (*compare_function)(void *, void *);
	void (*key_val_free_function)(void *);
};

int compare_function_strings(void *a, void *b)
{
	char *str_a = (char *)a;
	char *str_b = (char *)b;

	return strcmp(str_a, str_b);
}

unsigned int hash_function_int(void *a)
{
	/*
	 * Credits: https://stackoverflow.com/a/12996028/7883884
	 */
	unsigned int uint_a = *((unsigned int *)a);

	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = (uint_a >> 16u) ^ uint_a;
	return uint_a;
}

unsigned int hash_function_string(void *a)
{
	/*
	 * Credits: http://www.cse.yorku.ca/~oz/hash.html
	 */
	unsigned char *puchar_a = (unsigned char *)a;
	unsigned long hash = 5381;
	int c;

	while ((c = *puchar_a++))
		hash = ((hash << 5u) + hash) + c; /* hash * 33 + c */

	return hash;
}

void key_val_free_function(void *data)
{
	info *data_info = (info *)data;
	free(data_info->key);
	free(data_info->value);
}

hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void *),
					   int (*compare_function)(void *, void *),
					   void (*key_val_free_function)(void *))
{
	if (!hash_function || !compare_function) {
		return NULL;
	}

	hashtable_t *map = malloc(sizeof(hashtable_t));
	DIE(!map, "malloc failed");

	map->size = 0;
	map->hmax = hmax;
	map->hash_function = hash_function;
	map->compare_function = compare_function;
	map->key_val_free_function = key_val_free_function;

	map->buckets = malloc(map->hmax * sizeof(linked_list_t *));
	DIE(!map->buckets, "malloc failed");

	for (unsigned int i = 0; i < map->hmax; ++i) {
		map->buckets[i] = ll_create(sizeof(info));
	}

	return map;
}

int ht_has_key(hashtable_t *ht, void *key)
{
	if (!ht || !key) {
		return -1;
	}

	int hash_index = ht->hash_function(key) % ht->hmax;
	ll_node_t *node = ht->buckets[hash_index]->head;

	while (node != NULL) {
		info *data_info = (info *)node->data;
		if (!ht->compare_function(data_info->key, key)) {
			return 1;
		}
		node = node->next;
	}

	return 0;
}

void *ht_get(hashtable_t *ht, void *key)
{
	if (!ht || !key || ht_has_key(ht, key) != 1) {
		return NULL;
	}

	int hash_index = ht->hash_function(key) % ht->hmax;
	ll_node_t *node = ht->buckets[hash_index]->head;

	while (node != NULL) {
		info *data_info = (info *)node->data;
		if (!ht->compare_function(data_info->key, key)) {
			return data_info->value;
		}
		node = node->next;
	}

	return NULL;
}

void ht_put(hashtable_t *ht, void *key, unsigned int key_size, void *value,
			unsigned int value_size)
{
	if (!ht || !key || !value) {
		return;
	}

	int hash_index = ht->hash_function(key) % ht->hmax;

	if (ht_has_key(ht, key) == 1) {
		ll_node_t *node = ht->buckets[hash_index]->head;
		while (node != NULL) {
			info *data_info = node->data;

			if (!ht->compare_function(data_info->key, key)) {
				free(data_info->value);

				data_info->value = malloc(value_size);
				DIE(!data_info->value, "malloc failed");

				memcpy(data_info->value, value, value_size);
				return;
			}

			node = node->next;
		}
	}

	info *data_info = malloc(sizeof(info));
	DIE(!data_info, "malloc failed");

	data_info->key = malloc(key_size);
	DIE(!data_info->key, "malloc failed");

	data_info->value = malloc(value_size);
	DIE(!data_info->value, "malloc failed");

	memcpy(data_info->key, key, key_size);
	memcpy(data_info->value, value, value_size);

	ll_add_nth_node(ht->buckets[hash_index], 0, data_info);
	ht->size++;

	free(data_info);
}

void ht_remove_entry(hashtable_t *ht, void *key)
{
	if (!ht || !key || ht_has_key(ht, key) != 1) {
		return;
	}

	int hash_index = ht->hash_function(key) % ht->hmax;
	ll_node_t *node = ht->buckets[hash_index]->head;

	unsigned int node_nr = 0;

	while (node != NULL) {
		info *data_info = (info *)node->data;

		if (!ht->compare_function(data_info->key, key)) {
			ht->key_val_free_function(data_info);
			free(data_info);

			ll_node_t *deleted_node =
				ll_remove_nth_node(ht->buckets[hash_index], node_nr);
			free(deleted_node);

			ht->size--;
			return;
		}

		node = node->next;
		node_nr++;
	}
}

void ht_free(hashtable_t *ht)
{
	if (!ht) {
		return;
	}

	for (unsigned int i = 0; i < ht->hmax; ++i) {
		ll_node_t *node = ht->buckets[i]->head;

		while (node != NULL) {
			ht->key_val_free_function(node->data);
			node = node->next;
		}

		ll_free(&ht->buckets[i]);
	}

	free(ht->buckets);
	free(ht);
}

unsigned int ht_get_size(hashtable_t *ht)
{
	if (ht == NULL)
		return 0;

	return ht->size;
}

unsigned int ht_get_hmax(hashtable_t *ht)
{
	if (ht == NULL)
		return 0;

	return ht->hmax;
}

// ===========================
// ===========EXTRA===========
// ===========================

void move_objects_ht_by_hash(hashtable_t *ht_receive, hashtable_t *ht_give,
							 unsigned int hash_high, unsigned int hash_low)
{
	// Go through the elements of the hashtable (ht_give)
	for (unsigned int i = 0; i < ht_give->hmax; ++i) {
		ll_node_t *node = ht_give->buckets[i]->head;
		while (node != NULL) {
			int transferred = 0;
			int index_node_current = 0;
			info *information = (info *)node->data;
			char *current_key = (char *)information->key;

			// If we find an element to transfer, we put it in ht_receive
			// and eliminate it from ht_give.
			if (hash_high > hash_function_key((void *)current_key) &&
				hash_function_key((void *)current_key) > hash_low) {
				char *current_value = (char *)information->value;
				ht_put(ht_receive, current_key, strlen(current_key) + 1,
					   current_value, strlen(current_value) + 1);
				ht_give->key_val_free_function(node->data);
				transferred = 1;
			}

			node = node->next;
			// If we have transferred an object, we need to remove it from the
			// list.
			if (transferred == 1) {
				ll_node_t *removed =
					ll_remove_nth_node(ht_give->buckets[i], index_node_current);
				free(removed->data);
				removed->data = NULL;
				free(removed);
				removed = NULL;
			}
			index_node_current++;
		}
	}
}
