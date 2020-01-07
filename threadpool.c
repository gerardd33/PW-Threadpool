#include "threadpool.h"
#include <stdio.h>

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

bool queue_push(queue_t* queue, runnable_t value) {
	queue_node_t* new_node = malloc(sizeof(queue_node_t));
	if (new_node == NULL)
		return false;
	new_node->value = value;
	new_node->next = NULL;

	if (queue->begin) {
		queue->end->next = new_node;
		queue->end = new_node;
	} else {
		queue->begin = new_node;
		queue->end = new_node;
	}

	return true;
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

void* worker(void* data)
{
	thread_pool_t *pool = data;
	while (1)
	{
		pthread_mutex_lock(&(pool->security));
		if (queue_empty(pool->job_queue))
		{
			if (pool->interrupted)
				break;
			else
				pthread_cond_wait(&(pool->job_to_do), &(pool->security));
		}

		queue_node_t *job_node = queue_pop(pool->job_queue);
		pthread_mutex_unlock(&(pool->security));

		if (job_node)
		{
			job_node->value.function(job_node->value.arg, job_node->value.argsz);
			free(job_node);
		}
	}

	//pthread_cond_signal(&(pool->joining));
	pthread_mutex_unlock(&(pool->security));
	return NULL;
}

int thread_pool_init(thread_pool_t* pool, size_t num_threads) {
	pool->interrupted = false;
	pool->no_threads = num_threads;

	queue_t* new_queue = malloc(sizeof(queue_t));
	new_queue->begin = NULL;
	new_queue->end = NULL;
	pool->job_queue = new_queue;

	pthread_mutex_init(&(pool->security), NULL);
	//syserr
	pthread_cond_init(&(pool->job_to_do), NULL);
	//syserr
	//pthread_cond_init(&(pool->joining), NULL);
	//syserr

	pool->threads = malloc(sizeof(pthread_t) * num_threads);

	for (size_t i = 0; i < num_threads; ++i) {
		pthread_create(pool->threads + i, NULL, worker, pool);
		/*pthread_t thread;
		pthread_attr_t attr;
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		//syserr
		pthread_create(&thread, &attr, worker, pool);
		//syserr
		 */
	}

	return 0;
}

void thread_pool_destroy(thread_pool_t* pool) {
	if (pool == NULL)
		return;

	pthread_mutex_lock(&(pool->security));
	pool->interrupted = true;
	pthread_cond_broadcast(&(pool->job_to_do));
	pthread_mutex_unlock(&(pool->security));

	for (int i = 0; i < pool->no_threads; ++i) {
		pthread_join(pool->threads[i], NULL);
	}

	free(pool->threads);
	/*
	while (pool->threads_alive > 0)
		pthread_cond_wait(&(pool->joining), &(pool->security));
	*/

	queue_clear(pool->job_queue);
	pthread_mutex_destroy(&(pool->security));
	//err
	//pthread_cond_destroy(&(pool->joining));
	//err
	pthread_cond_destroy(&(pool->job_to_do));
	//err
}

int defer(thread_pool_t* pool, runnable_t runnable) {
	if (pool == NULL)
		return -1;

	pthread_mutex_lock(&(pool->security));
	if (pool->interrupted) {
		pthread_mutex_unlock(&(pool->security));
		return -1;
	}

	queue_push(pool->job_queue, runnable);
	pthread_cond_broadcast(&(pool->job_to_do));
	//albo signal?
	pthread_mutex_unlock(&(pool->security));

	return 0;
}
