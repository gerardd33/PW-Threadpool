#include "future.h"

typedef void* (*function_t)(void*);

void compute(void* arg, size_t argsz __attribute__((unused))) {
	int err;
	future_t* future = arg;

	if ((err = pthread_mutex_lock(&(future->security))) != 0)
		exit(1);
	void* result = future->callable.function(future->callable.arg,
																					 future->callable.argsz, &future->result_size);

	future->result = result;
	if ((err = pthread_cond_broadcast(&(future->pending))) != 0)
		exit(1);
	if ((err = pthread_mutex_unlock(&(future->security))) != 0)
		exit(1);
}

void wait_take_and_compute(void* arg, size_t argsz __attribute__((unused))) {
	int err;
	future_t* future = arg;
	if ((err = pthread_mutex_lock(&(future->from->security))) != 0)
		exit(1);
	if ((err = pthread_cond_wait(&(future->from->pending), &(future->from->security))) != 0)
		exit(1);

	size_t processed_arg_size;
	void* processed_arg = future->modifier(future->from->result,
																				 future->from->result_size, &processed_arg_size);
	void* result = (future->callable.function(processed_arg,
																						processed_arg_size, &(future->result_size)));

	future->result = result;
	if ((err = pthread_cond_broadcast(&(future->pending))) != 0)
		exit(1);
	if ((err = pthread_mutex_unlock(&(future->security))) != 0)
		exit(1);
}

int async(thread_pool_t* pool, future_t* future, callable_t callable) {
	int err;
	if ((err = pthread_mutex_lock(&(future->security))) != 0)
		exit(1);
	if ((err = pthread_cond_init(&(future->pending), NULL)) != 0)
		exit(1);
	future->callable = callable;
	future->from = NULL;
	future->modifier = NULL;

	runnable_t task;
	task.arg = future;
	task.argsz = sizeof(future);
	task.function = &compute;

	if ((err = defer(pool, task)) != 0)
		return -1;
	if ((err = pthread_mutex_unlock(&(future->security))) != 0)
		exit(1);
	return 0;
}

int map(thread_pool_t* pool, future_t* future, future_t* from,
				void* (*function)(void*, size_t, size_t*)) {
	int err;
	if ((err = pthread_mutex_lock(&(future->security))) != 0)
		exit(1);
	future->from = from;
	future->modifier = function;

	runnable_t task;
	task.arg = future;
	task.argsz = sizeof(future);
	task.function = &wait_take_and_compute;

	if ((err = defer(pool, task)) != 0)
		return -1;
	if ((err = pthread_mutex_unlock(&(future->security))) != 0)
		exit(1);
	return 0;
}

void* await(future_t* future) {
	int err;
	if ((err = pthread_mutex_lock(&(future->security))) != 0)
		exit(1);
	if ((err = pthread_cond_wait(&(future->pending), &(future->security))) != 0)
		exit(1);

	void* result = future->result;
	if ((err = pthread_mutex_unlock(&(future->security))) != 0)
		exit(1);
	return result;
}
