#include "future.h"

typedef void* (*function_t)(void*);

void compute(void* arg, size_t argsz __attribute__((unused))) {
	future_t* future = arg;

	pthread_mutex_lock(&(future->security));
	void* result = future->callable.function(future->callable.arg,
																					 future->callable.argsz, &future->result_size);

	future->result = result;
	pthread_cond_broadcast(&(future->pending));
	pthread_mutex_unlock(&(future->security));
}

void wait_take_and_compute(void* arg, size_t argsz __attribute__((unused))) {
	future_t* future = arg;
	pthread_mutex_lock(&(future->from->security));
	pthread_cond_wait(&(future->from->pending), &(future->from->security));

	size_t processed_arg_size;
	void* processed_arg = future->modifier(future->from->result,
																				 future->from->result_size, &processed_arg_size);
	void* result = (future->callable.function(processed_arg,
																						processed_arg_size, &(future->result_size)));

	future->result = result;
	pthread_cond_broadcast(&(future->pending));
	pthread_mutex_unlock(&(future->security));
}

int async(thread_pool_t* pool, future_t* future, callable_t callable) {
	pthread_mutex_lock(&(future->security));
	pthread_cond_init(&(future->pending), NULL);
	future->callable = callable;
	future->from = NULL;
	future->modifier = NULL;

	runnable_t task;
	task.arg = future;
	task.argsz = 1;
	task.function = compute;

	defer(pool, task);
	pthread_mutex_unlock(&(future->security));
	return 0;
}

int map(thread_pool_t* pool, future_t* future, future_t* from,
				void* (*function)(void*, size_t, size_t*)) {
	pthread_mutex_lock(&(future->security));
	future->from = from;
	future->modifier = function;

	runnable_t task;
	task.arg = future;
	task.argsz = 1;
	task.function = wait_take_and_compute;

	defer(pool, task);
	pthread_mutex_unlock(&(future->security));
	return 0;
}

void* await(future_t* future) {
	pthread_mutex_lock(&(future->security));
	pthread_cond_wait(&(future->pending), &(future->security));

	void* result = future->result;
	pthread_mutex_unlock(&(future->security));
	return result;
	return 0;
}
