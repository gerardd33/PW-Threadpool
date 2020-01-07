#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stddef.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct runnable {
	void (*function)(void*, size_t);
	void* arg;
	size_t argsz;
} runnable_t;

typedef struct queue_node {
	runnable_t value;
	struct queue_node* next;
} queue_node_t;

typedef struct queue {
	queue_node_t* begin;
	queue_node_t* end;
} queue_t;

typedef struct thread_pool {
	bool interrupted;
	int no_threads;
	pthread_t* threads;
	queue_t* job_queue;
	pthread_mutex_t security;
	pthread_cond_t job_to_do;
} thread_pool_t;

int thread_pool_init(thread_pool_t* pool, size_t pool_size);

void thread_pool_destroy(thread_pool_t* pool);

int defer(thread_pool_t* pool, runnable_t runnable);

#endif
