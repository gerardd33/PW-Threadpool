#include "threadpool.h"

bool queue_empty(queue_t* queue) {
	return queue->begin == NULL;
}

queue_node_t* queue_pop(queue_t* queue) {
	queue_node_t* node = queue->begin;
	if (node == NULL)
		return NULL;

	queue->begin = queue->begin->next;
	return node;
}

// Returns 0 if successful, -1 otherwise.
int queue_push(queue_t* queue, runnable_t value) {
	queue_node_t* new_node = malloc(sizeof(queue_node_t));
	if (new_node == NULL)
		return -1;
	new_node->value = value;
	new_node->next = NULL;

	if (queue->begin) {
		queue->end->next = new_node;
		queue->end = new_node;
	} else {
		queue->begin = new_node;
		queue->end = new_node;
	}

	return 0;
}

void queue_clear(queue_t* queue) {
	queue_node_t* current_node = queue->begin;
	queue_node_t* next_node;
	while (current_node != NULL) {
		next_node = current_node->next;
		free(current_node);
		current_node = next_node;
	}

	free(queue);
}

void* worker(void* data) {
	thread_pool_t *pool = data;
	int err;

	while (1) {
		if ((err = pthread_mutex_lock(&(pool->security))) != 0)
			exit(1);
		if (queue_empty(pool->job_queue)) {
			if (pool->interrupted)
				break;
			else if ((err = pthread_cond_wait(&(pool->job_to_do), &(pool->security))) != 0)
				exit(1);
		}

		queue_node_t *job_node = queue_pop(pool->job_queue);
		if ((err = pthread_mutex_unlock(&(pool->security))) != 0)
			exit(1);

		if (job_node) {
			job_node->value.function(job_node->value.arg, job_node->value.argsz);
			free(job_node);
		}
	}

	if ((err = pthread_mutex_unlock(&(pool->security))) != 0)
		exit(1);
	return NULL;
}

int thread_pool_init(thread_pool_t* pool, size_t num_threads) {
	pool->interrupted = false;
	pool->no_threads = num_threads;

	queue_t* new_queue = malloc(sizeof(queue_t));
	if (new_queue == NULL)
		return -1;
	new_queue->begin = NULL;
	new_queue->end = NULL;
	pool->job_queue = new_queue;

	int err;
	if ((err = pthread_mutex_init(&(pool->security), NULL)) != 0)
		exit(1);
	if ((err = pthread_cond_init(&(pool->job_to_do), NULL)) != 0)
		exit(1);

	pool->threads = malloc(sizeof(pthread_t) * num_threads);
	if (pool->threads == NULL)
		return -1;

	for (size_t i = 0; i < num_threads; ++i) {
		if ((err = pthread_create(pool->threads + i, NULL, worker, pool) != 0))
			exit(1);
	}

	return 0;
}

void thread_pool_destroy(thread_pool_t* pool) {
	if (pool == NULL)
		return;

	int err;
	if ((err = pthread_mutex_lock(&(pool->security))) != 0)
		exit(1);
	pool->interrupted = true;
	if ((err = pthread_cond_broadcast(&(pool->job_to_do))) != 0)
		exit(1);
	if ((err = pthread_mutex_unlock(&(pool->security))) != 0)
		exit(1);

	void* tmp_res;
	for (size_t i = 0; i < pool->no_threads; ++i) {
		if ((err = pthread_join(pool->threads[i], &tmp_res)) != 0)
			exit(1);
	}

	free(pool->threads);
	queue_clear(pool->job_queue);
	if ((err = pthread_mutex_destroy(&(pool->security))) != 0)
		exit(1);
	if ((err = pthread_cond_destroy(&(pool->job_to_do))) != 0)
		exit(1);
}

int defer(thread_pool_t* pool, runnable_t runnable) {
	if (pool == NULL)
		return -1;

	int err;
	if ((err = pthread_mutex_lock(&(pool->security))) != 0)
		exit(1);
	if (pool->interrupted) {
		if ((err = pthread_mutex_unlock(&(pool->security))) != 0)
			exit(1);
		return -1;
	}

	if ((err = queue_push(pool->job_queue, runnable)) != 0)
		return -1;
	if ((err = pthread_cond_broadcast(&(pool->job_to_do))) != 0)
		exit(1);
	if ((err = pthread_mutex_unlock(&(pool->security))) != 0)
		exit(1);

	return 0;
}
