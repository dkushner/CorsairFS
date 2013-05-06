#ifndef BDECODE_H_
#define BDECODE_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* * * * * * * * * * * * *
 * BENCODING DATA TYPES  *
 * * * * * * * * * * * * */
enum bd_type
{
  NUMBER,
  STRING,
  DICTIONARY,
  LIST,
};

struct bd_dict;
struct bd_list;

typedef struct
{
  enum bd_type type;
  union
  {
    void* data;
    char* str;
    struct bd_dict* dict;
    struct bd_list* list;
  };
} bd_entry;

typedef struct bd_dict
{
  char* key;
  enum bd_type type;
  union
  {
    void* data;
    char* str;
    struct bd_dict* dict;
    struct bd_list* list;
  };
  struct bd_dict* next;
} bd_dict;

typedef struct bd_list
{
  int used;
  int allocated;
  bd_entry* entries;
} bd_list;

bd_list* bd_list_create();
bd_dict* bd_dict_create();
void bd_dict_add(bd_dict** dict, char* key, enum bd_type type, void* data);
void bd_list_add(bd_list* list, enum bd_type type, void* data);
void bd_list_destroy(bd_list* list);
void bd_dict_destroy(bd_dict* dict);

/* * * * * * * * * * * * * * * *
 * BENCODE DECODING FUNCT *
 * * * * * * * * * * * * * * * */
void* decode(unsigned char* buf, size_t size);
bd_dict* decode_dictionary(unsigned char* buf, int* index, size_t size);
bd_list* decode_list(unsigned char* buf, int* index, size_t size);
long long decode_number(unsigned char* buf, int* index, size_t size);
char* decode_string(unsigned char* buf, int* index, size_t size);

#endif
