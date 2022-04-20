/*
 * This is a slightly modified version of the hash table lab from 321.
 *
 * This file implements a hash table, which is an efficient data structure for
 * maintaining a collection of "key"-to-"value" mappings.  This hash table, in
 * particular, maintains mappings from strings to opaque objects. 
 *
 * This file is placed in the public domain by its authors, Alan L. Cox and
 * Kaushik Kumar Ram.
 */

/* Remove this if you wish to use the assertions. */
#define NDEBUG

#include <assert.h>
#include <stdlib.h> /* For malloc, free, and calloc. */
#include "yfs.h"
#include "hash_table.h"

/*
 * Computes the hash of a given key using the idea in Piazza @533.
 */
static unsigned int
hash_value(int key, int size)
{
	return key % size;
}

/*
 * A hash table mapping:
 *
 *  Stores the association or mapping from a string "key" to its opaque
 *  "value".  Each mapping is an element of a singly-linked list.  This list
 *  is called a collision chain.
 */
struct hash_table_mapping {
	int key;
	/* 
	 * The "value" is a pointer to an object of an unknown type, because
	 * the hash table doesn't need to know the type in order to store and
	 * return the pointer.
	 */
	void *value;
	/*
	 * The pointer to the next element in the same collision chain.
	 */
	struct hash_table_mapping *next; 
};

/*
 * A hash table:
 *
 *  Stores a collection of "key"-to-"value" mappings.  Each mapping is an
 *  element of a collision chain.  For efficiency, the number of collision
 *  chains is kept in proportion to the number of mappings so that the average
 *  length of a collision chain remains constant no matter how many mappings
 *  the hash table contains.
 */
struct hash_table {
	/* 
	 * The array of collision chains.  Really, this is a pointer to an
	 * array of pointers.  Each element of that array is the head of a
	 * collision chain.
	 */
	struct hash_table_mapping **head;
	/*
	 * The number of collision chains.
	 */
	unsigned int size;
	/*
	 * The number of mappings in the hash table.
	 */
	unsigned int occupancy;		
	/*
	 * The upper bound on the average collision chain length that is
	 * allowed before the number of collision chains is increased.
	 */
	double load_factor;
};

/*
 * Requires:
 *  "load_factor" must be greater than zero.
 *
 * Effects:
 *  Creates a hash table with the upper bound "load_factor" on the average
 *  length of a collision chain.  Returns a pointer to the hash table if it
 *  was successfully created and NULL if it was not.
 */
struct hash_table * 
hash_table_create(int size)
{
	struct hash_table *ht;

	ht = malloc(sizeof(struct hash_table));
	if (ht == NULL)
		return (NULL);
	/* 
	 * Allocate and initialize the hash table's array of pointers.  In
	 * contrast to malloc(), calloc() initializes each of the returned
	 * memory locations to zero, effectively initializing the head of
	 * every collision chain to NULL.  
	 */
	ht->head = calloc(size, sizeof(struct hash_table_mapping *));
	if (ht->head == NULL) {
		free(ht);
		return (NULL);
	}
	ht->size = size;
	ht->occupancy = 0;
	ht->load_factor = 2.0; // 2.0 is manual upper bound
	return (ht);
}

/*
 * Requires:
 *  "new_size" must be greater than zero.
 *
 * Effects:
 *  Grows (or shrinks) the number of collision chains in the hash table "ht"
 *  to "new_size".  Returns 0 if the increase (or decrease) was successful and
 *  -1 if it was not.
 */
static int
hash_table_resize(struct hash_table *ht, unsigned int new_size)
{
	struct hash_table_mapping **new_head;
	struct hash_table_mapping *elem, *next;
	unsigned int index, new_index;

	/*
	 * Does the hash table already have the desired number of collision
	 * chains?  If so, do nothing.
	 */
	if (ht->size == new_size)
		return (0);
	/* 
	 * Allocate and initialize the hash table's new array of pointers.
	 * In contrast to malloc(), calloc() initializes each of the returned
	 * memory locations to zero, effectively initializing the head of
	 * every collision chain to NULL.
	 */
	new_head = calloc(new_size, sizeof(struct hash_table_mapping *));
	if (new_head == NULL)
		return (-1);
	/*
	 * Iterate over the collision chains.
	 */
	for (index = 0; index < ht->size; index++) {
		/*
		 * Transfer each of the mappings in the old collision chain to
		 * its new collision chain.  Typically, this results in two
		 * shorter collision chains.
		 */
        for (elem = ht->head[index]; elem != NULL; elem = next) {
			/*
			 * The next mapping pointer is overwritten when the
			 * mapping is inserted into its new collision chain,
			 * so the next mapping pointer must be saved in order
			 * that the loop can go on.
			 */
            next = elem->next;
			/*
			 * Compute the array index of the new collision chain
			 * into which the mapping should be inserted.
			 */
			new_index = hash_value(elem->key, ht->size) % new_size;
			/*
			 * Insert the mapping into its new collision chain.
			 */
            elem->next = new_head[new_index];
            new_head[new_index] = elem;
		}
	}
	/*
	 * Free the old array and replace it with the new array.
	 */
	free(ht->head);
	ht->head = new_head;
	ht->size = new_size;
	return (0);
}

/*
 * Requires:
 *  "key" is not already in "ht".
 *  "value" is not NULL.
 *
 * Effects:
 *  Creates an association or mapping from the string "key" to the pointer
 *  "value" in the hash table "ht".  Returns 0 if the mapping was successfully
 *  created and -1 if it was not.
 */
int
hash_table_insert(struct hash_table *ht, int key, void *value)
{
	struct hash_table_mapping *elem;
	unsigned int index;

	assert(hash_table_lookup(ht, key) == NULL);
	assert(value != NULL);

	/*
	 * Should the number of collision chains be increased?
	 */
	if (ht->occupancy >= ht->load_factor * ht->size) {
		/*
		 * Try to double the number of collision chains.
		 */
		if (hash_table_resize(ht, ht->size * 2) == -1)
			return (-1);
	}
	/*
	 * Allocate memory for a new mapping and initialize it.
	 */
	elem = malloc(sizeof(struct hash_table_mapping));
	if (elem == NULL)
		return (-1);
	elem->key = key;
	elem->value = value;
	/*
	 * Compute the array index of the collision chain in which the mapping
	 * should be inserted.
	 */
	index = hash_value(key, ht->size) % ht->size;
	/*
	 * Insert the new mapping into that collision chain.
	 */
    elem->next = ht->head[index];
    ht->head[index] = elem;
	ht->occupancy++;
	return (0);
}

/*
 * Requires:
 *  Nothing.
 *
 * Effects:
 *  Searches the hash table "ht" for the string "key".  If "key" is found,
 *  returns its associated value.  Otherwise, returns NULL.
 */
void *
hash_table_lookup(struct hash_table *ht, int key)
{
	struct hash_table_mapping *elem;
	unsigned int index;

	/*
	 * Compute the array index of the collision chain in which "key"
	 * should be found.
	 */
	index = hash_value(key, ht->size) % ht->size;
	/* 
	 * Iterate over that collision chain.
	 */
	for (elem = ht->head[index]; elem != NULL; elem = elem->next) {
		/*
		 * If "key" matches the element's key, then return the
		 * element's value.
		 */
		if (elem->key == key)
			return (elem->value);
	}
	/*
	 * Otherwise, return NULL to indicate that "key" was not found.
	 */
	return (NULL);
}

/*
 * Requires:
 *  Nothing.
 *
 * Effects:
 *  Searches the hash table "ht" for the string "key".  If "key" is found,
 *  removes its mapping from "ht".
 */
void *
hash_table_remove(struct hash_table *ht, int key)
{
	struct hash_table_mapping *elem, *prev;
	unsigned int index;

	/*
	 * A pair of casts to prevent compilation warnings.  Remove these
	 * statements once you have completed this function.
	 */
	prev = NULL;

	/*
	 * Compute the array index of the collision chain in which "key"
	 * should be found.
	 */
	index = hash_value(key, ht->size) % ht->size;
	/*
	 * Iterate over that collision chain.
	 */
	for (elem = ht->head[index]; elem != NULL; elem = elem->next) {
		/*
		 * If "key" matches the element's key, then remove the
		 * mapping from the hash table.
		 */
		if (elem->key == key) {
			/*
			 * First, remove the mapping from its collision
			 * chain.
			 */
			if (prev == NULL)
			    ht->head[index] = elem->next;
			else
			    prev->next = elem->next;
			ht->occupancy--;
		}
		prev = elem;
	}
	return (NULL);
}
