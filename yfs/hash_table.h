/*
 * This file defines the interface for a hash table, which is an efficient
 * data structure for maintaining a collection of "key"-to-"value" mappings.
 * This hash table, in particular, maintains mappings from strings to opaque
 * objects.
 *
 * This file is placed in the public domain by its authors, Alan L. Cox and
 * Kaushik Kumar Ram.
 */

/*
 * Declare the hash table struct type without exposing its implementation
 * details, because the callers to the following functions don't need to know
 * those details in order to declare, assign, or pass a pointer to this struct
 * type.  However, without knowing those details, any attempt to dereference
 * such a pointer will result in a compilation error.  This is an example of
 * "data hiding", which is a good principle to follow in designing and
 * implementing data abstractions.
 */
struct hash_table;

/*
 * Requires:
 *  "load_factor" must be greater than zero.
 *
 * Effects:
 *  Creates a hash table with the upper bound "load_factor" on the average
 *  length of a collision chain.  Returns a pointer to the hash table if it
 *  was successfully created and NULL if it was not.
 */
struct hash_table *hash_table_create(int size);

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
int hash_table_insert(struct hash_table *ht, int key, void *value);

/*
 * Requires:
 *  Nothing.
 *
 * Effects:
 *  Searches the hash table "ht" for the string "key".  If "key" is found,
 *  returns its associated value.  Otherwise, returns NULL.
 */
void *hash_table_lookup(struct hash_table *ht, int key);

/*
 * Requires:
 *  Nothing.
 *
 * Effects:
 *  Searches the hash table "ht" for the string "key".  If "key" is found,
 *  removes its mapping from "ht".
 *  Returns NULL.
 */
void *hash_table_remove(struct hash_table *ht, int key);
