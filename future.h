#ifndef FUTURE_H
#define FUTURE_H

#include "threadpool.h"

typedef struct callable {
	void* (*function)(void*, size_t, size_t*);
	void* arg;
	size_t argsz;
} callable_t;

typedef struct future {
	callable_t callable;
	pthread_mutex_t security;
	pthread_cond_t pending;
	void* result;
	size_t result_size;
	struct future* from;
	void* (*modifier)(void*, size_t, size_t*);
} future_t;

int async(thread_pool_t* pool, future_t* future, callable_t callable);

int map(thread_pool_t* pool, future_t* future, future_t* from,
				void* (*function)(void*, size_t, size_t*));

void* await(future_t* future);

#endif
