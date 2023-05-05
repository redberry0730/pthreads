#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include "matrix.h"
 
typedef struct _thread_args {
    int id;
    int t_num;
    matrix *m1;
    matrix *m2;
    matrix *m3;
} thread_args;

// function for each thread that adds data to the queue
void *thread_main(void *arg)
{
    // get the arguments from the main thread
    thread_args *targs = (thread_args *)arg;

    // divide tasks as equally as possible across threads
    int min_tasks_per_thread = targs->m1->num_rows * targs->m2->num_cols / targs->t_num;
    int extra_tasks = targs->m1->num_rows * targs->m2->num_cols % targs->t_num;

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

    for (int i = my_start_task; i < my_start_task + my_num_tasks; ++i) {
      int r = i / targs->m2->num_cols;
      int c = i % targs->m2->num_cols;

      targs->m3->data[r][c] = 0;

      for (int j = 0; j < targs->m1->num_cols; ++j) {
        targs->m3->data[r][c] += targs->m1->data[r][j] * targs->m2->data[j][c];
      }
    }

    return NULL;
}

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

int main(int argc, char *argv[])
{ 
  if (argc != 4)
  {
    printf("usage: %s matrix1_file matrix2_file\n", basename(argv[0]));
    exit(1);
  }

  matrix *m1 = read_matrix(argv[2]);
  matrix *m2 = read_matrix(argv[3]);
  matrix *res = alloc_matrix(m1->num_rows, m2->num_cols);

  int num_t = atoi(argv[1]);

  if (m1->num_cols != m2->num_rows) {
    printf("Wrong Matrices!\n");
    exit(1);
  }

  thread_args *targs = (thread_args *)malloc(num_t*sizeof(thread_args));

  if (num_t < 1) {
    printf("error: must have at least one thread\n");
    printf("usage: %s num_threads num_loops\n", basename(argv[0]));
    exit(1);
  }

  pthread_t *tids = (pthread_t *)malloc(num_t*sizeof(pthread_t));
  if (tids == NULL) {
    printf("out of memory!\n");
    exit(1);
  }

  uint64_t start = get_time_usec();  

  for (int i=0; i<num_t; ++i)
  {
      targs[i].id = i;
      targs[i].t_num = num_t;
      targs[i].m1 = m1;
      targs[i].m2 = m2;
      targs[i].m3 = res;

      if (pthread_create(&tids[i], NULL, thread_main, &targs[i]) != 0)
      {
          perror("pthread_create");
          exit(1);
      }
  }

  for (int i=0; i<num_t; ++i)
  {
    if (pthread_join(tids[i], NULL) != 0)
    {
      perror("pthread_join");
      exit(1);
    }
  }

  print_matrix(res);

  uint64_t stop = get_time_usec();  

  uint64_t total = stop-start;
  fprintf(stderr, "time=%.6lfs\n", total/1000000.0);

  free_matrix(m1);
  free_matrix(m2);
  free_matrix(res);
  free(targs);
  free(tids);
    
  return 0;
}
