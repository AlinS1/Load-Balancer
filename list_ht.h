#ifndef LIST_HT_H_
#define LIST_HT_H_

struct ll_node_t;
typedef struct ll_node_t ll_node_t;
struct linked_list_t;
typedef struct linked_list_t linked_list_t;

linked_list_t *ll_create(unsigned int data_size);
void ll_add_nth_node(linked_list_t *list, unsigned int n, const void *new_data);
ll_node_t *ll_remove_nth_node(linked_list_t *list, unsigned int n);
unsigned int ll_get_size(linked_list_t *list);
void ll_free(linked_list_t **pp_list);
void ll_print_int(linked_list_t *list);
void ll_print_string(linked_list_t *list);

struct info;
typedef struct info info;
struct hashtable_t;
typedef struct hashtable_t hashtable_t;

int compare_function_ints(void *a, void *b);
int compare_function_chars(void *a, void *b);
int compare_function_strings(void *a, void *b);
unsigned int hash_function_int(void *a);
unsigned int hash_function_string(void *a);
void key_val_free_function(void *data);
hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void *),
					   int (*compare_function)(void *, void *),
					   void (*key_val_free_function)(void *));
int ht_has_key(hashtable_t *ht, void *key);
void *ht_get(hashtable_t *ht, void *key);
void ht_put(hashtable_t *ht, void *key, unsigned int key_size, void *value,
			unsigned int value_size);
void ht_remove_entry(hashtable_t *ht, void *key);
void ht_free(hashtable_t *ht);
unsigned int ht_get_size(hashtable_t *ht);
unsigned int ht_get_hmax(hashtable_t *ht);
void move_objects_ht_by_hash(hashtable_t *ht_receive, hashtable_t *ht_give,
							 unsigned int hash_high, unsigned int hash_low);
void move_all_objects_ht(hashtable_t *ht_receive, hashtable_t *ht_give);

#endif /* LIST_HT_H_ */