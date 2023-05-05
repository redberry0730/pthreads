#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <pthread.h>

typedef struct _hashitem
{
    char *key;
    int value;
} hashitem;

typedef struct _hashbucket
{
    hashitem *item;
    struct _hashbucket *next;
    struct _hashbucket *prev;
} hashbucket;

typedef struct _hashtable
{
    hashbucket **buckets;
    int capacity;
    pthread_mutex_t *locks;
} hashtable;

typedef struct _thread_args {
  int id;
  int t_num;
  int k_num;
  hashtable *hash;
  char **keys;
} thread_args;

hashtable *make_hashtable(int capacity);
void hashtable_insert(hashtable *hash, char *key, int value);
hashitem *hashtable_search(hashtable *hash, char *key);
void print_hashtable(hashtable *hash);
void destroy_hashtable(hashtable *hash);

#endif
