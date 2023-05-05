#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include "hashtable.h"

uint64_t get_time_usec()
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
    {
        perror("clock_gettime");
        exit(1);
    }
    return ts.tv_sec*1000000 + ts.tv_nsec/1000;
}

void insert_keys(hashtable *hash, char **keys, int num_keys)
{
    for (int i=0; i<num_keys; ++i)
    {
        hashtable_insert(hash, keys[i], i);
    }
}

int search_keys(hashtable *hash, char **keys, int num_keys)
{
    int num_missing = 0;
    for (int i=0; i<num_keys; ++i)
    {
        if (hashtable_search(hash, keys[i]) == NULL)
        {
            ++num_missing;
        }
    }
    return num_missing;
}

char *random_key(int len)
{
    char *key = (char *)malloc((len+1)*sizeof(char));
    if (key == NULL)
    {
        perror("malloc");
        exit(1);
    }
    for (int i=0; i<len; ++i)
    {
        key[i] = (random() % 26) + 'a';
    }
    key[len] = '\0';
    return key;
}

void *thread_insert(void *arg) {
  thread_args *targs = (thread_args *)arg;
  
  int min_tasks_per_thread = targs->k_num / targs->t_num;
  int extra_tasks = targs->k_num % targs->t_num;

  int my_start_task = min_tasks_per_thread * targs->id;
  int my_num_tasks = min_tasks_per_thread;
  if (extra_tasks > targs->id)
  {
      my_start_task += targs->id;
      ++my_num_tasks;
  }
  else
  {
      my_start_task += extra_tasks;
  }

  insert_keys(targs->hash, targs->keys, my_num_tasks);
  pthread_exit(NULL);
}

void *thread_search(void *arg) {
  thread_args *targs = (thread_args *)arg;
  
  int min_tasks_per_thread = targs->k_num / targs->t_num;
  int extra_tasks = targs->k_num % targs->t_num;

  int my_start_task = min_tasks_per_thread * targs->id;
  int my_num_tasks = min_tasks_per_thread;
  if (extra_tasks > targs->id)
  {
      my_start_task += targs->id;
      ++my_num_tasks;
  }
  else
  {
      my_start_task += extra_tasks;
  }

  long lost = (long)search_keys(targs->hash, targs->keys, my_num_tasks);
  pthread_exit((void *)lost);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("usage: %s\n", basename(argv[0]));
        exit(1);
    }

    srandom(time(NULL));

    int num_t = atoi(argv[1]);
    if (num_t < 1) {
      printf("Invalid number of threads\n");
      exit(1);
    }

    int num_keys = 100000;
    int key_len = 4;
    char **keys;

    keys = (char **)malloc(num_keys*sizeof(char *));
    if (keys == NULL)
    {
        perror("malloc");
        exit(1);
    }

    for (int i=0; i<num_keys; ++i)
    {
        keys[i] = random_key(key_len);;
    }

    pthread_mutex_t *lock = (pthread_mutex_t *)malloc(num_keys * sizeof(pthread_mutex_t));
    pthread_t *threads = (pthread_t *)malloc(num_t * sizeof(pthread_t));
    
    if (!threads) {
      printf("pthreads error\n");
      exit(1);
    }

    hashtable *hash = make_hashtable(64);

    for (int i = 0; i < key_len; ++i) {
      if (pthread_mutex_init(&lock[i], NULL) != 0) {
        printf("lock init failed\n");
        exit(1);
      }
    }

    hash->locks = lock;

    uint64_t start = get_time_usec();
    thread_args *targs = (thread_args *)malloc(num_t * sizeof(thread_args));

    for (int i = 0; i < num_t; ++i) {
      targs[i].id = i;
      targs[i].t_num = num_t;
      targs[i].k_num = num_keys;
      targs[i].keys = keys;
      targs[i].hash = hash;

      if (pthread_create(&threads[i], NULL, thread_insert, &targs[i]) != 0) {
        perror("pthread_create\n");
        exit(1);
      }
    }

    for (int i = 0; i < num_t; ++i) {
      pthread_join(threads[i], NULL);
    }

    uint64_t stop = get_time_usec();
    uint64_t total = stop-start;
    fprintf(stderr, "insert time=%.6lfs\n", total/1000000.0);

    start = get_time_usec();

    for (int i = 0; i < num_t; ++i) {
      targs[i].id = i;
      targs[i].t_num = num_t;
      targs[i].k_num = num_keys;
      targs[i].keys = keys;
      targs[i].hash = hash;
    
      if (pthread_create(&threads[i], NULL, thread_search, &targs[i]) != 0) {
        perror("pthread_create");
        exit(1);
      }
    }

    int* missing_keys = (int *)malloc(num_t * sizeof(int));
    int sum = 0;

    for (int i = 0; i < num_t; ++i) {
      pthread_join(threads[i], (void **)&missing_keys[i]);
      sum += missing_keys[i];
    }

    stop = get_time_usec();
    total = stop-start;
    fprintf(stderr, "Missing keys: %d\n", sum);
    fprintf(stderr, "search time=%.6lfs\n", total/1000000.0);

    for (int i=0; i<num_keys; ++i)
    {
        free(keys[i]);
    }

    free(keys);
    free(threads);
    free(targs);
    destroy_hashtable(hash);

    return 0;
}
