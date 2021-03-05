// hash.h

#ifndef _HASH_H
#define _HASH_H

typedef i32 Hvalue;
#define HTABLE_KEY_SIZE 32 - sizeof(Hvalue)
typedef char Hkey[HTABLE_KEY_SIZE];

struct Item;

typedef struct {
  struct Item* items;
  u32 count;	// Count of used slots
  u32 size;	// Total size of the hash table
} Htable;

Htable ht_create(u32 size);

Htable ht_create_empty();

i32 ht_is_empty(const Htable* table);

u32 ht_insert_element(Htable* table, const Hkey key, const Hvalue value);

const Hvalue* ht_lookup(const Htable* table, const Hkey key);

const Hvalue* ht_lookup_by_index(const Htable* table, const u32 index);

const Hkey* ht_lookup_key(const Htable* table, const u32 index);

i32 ht_element_exists(const Htable* table, const Hkey key);

void ht_remove_element(Htable* table, const Hkey key);

u32 ht_get_size(const Htable* table);

u32 ht_num_elements(const Htable* table);

void ht_free(Htable* table);

#endif
