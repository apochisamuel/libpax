#include "types.h"
#include <stdlib.h>
#include <string.h>
uint32_t ht_hash(const char *key) {
    uint32_t h = 5381;
    for (const char *p = key; *p; p++) h = ((h << 5) + h) ^ (uint8_t)*p;
    return h;
}
pax_hash_table *ht_create(void) {
    pax_hash_table *ht = (pax_hash_table*)calloc(1, sizeof(pax_hash_table));
    return ht;
}
void ht_destroy(pax_hash_table *ht) {
    if (!ht) return;
    for (int i = 0; i < PAX_HASH_BUCKETS; i++) {
        pax_hash_node *n = ht->buckets[i];
        while (n) { pax_hash_node *next = n->next; free(n); n = next; }
    }
    free(ht);
}
int ht_insert(pax_hash_table *ht, const char *key, int value) {
    uint32_t idx = ht_hash(key) % PAX_HASH_BUCKETS;
    pax_hash_node *n = (pax_hash_node*)malloc(sizeof(pax_hash_node));
    if (!n) return PAX_ERR_MEMORY;
    n->key = key; n->entry_index = value; n->next = ht->buckets[idx];
    ht->buckets[idx] = n; ht->count++;
    return PAX_OK;
}
int ht_lookup(const pax_hash_table *ht, const char *key) {
    uint32_t idx = ht_hash(key) % PAX_HASH_BUCKETS;
    for (pax_hash_node *n = ht->buckets[idx]; n; n = n->next)
        if (strcmp(n->key, key) == 0) return n->entry_index;
    return -1;
}
