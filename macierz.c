#include "threadpool.h"
#include <unistd.h>

const int THREADS = 4;

typedef struct row {
	long long sum;
	pthread_mutex_t security;
} row_t;

typedef struct field {
	int value;
	int time_needed;
	row_t* row;
} field_t;

void compute_field(void* arg, size_t argsz __attribute__((unused))) {
	field_t* field = arg;

	usleep(1000 * field->time_needed);
	pthread_mutex_lock(&(field->row->security));
	field->row->sum += field->value;
	pthread_mutex_unlock(&(field->row->security));
}

int main() {
	int no_columns, no_rows;
	scanf("%d %d", &no_columns, &no_rows);

	field_t** matrix = malloc(sizeof(field_t*) * no_rows);
	for (int row = 0; row < no_rows; ++row) {
		matrix[row] = malloc(sizeof(field_t) * no_columns);
	}

	for (int i = 0; i < no_columns * no_rows; ++i) {
		int row = i / no_rows;
		int column = i % no_rows;
		scanf("%d %d", &(matrix[row][column].value), &(matrix[row][column].time_needed));
	}

	row_t* rows = malloc(sizeof(row_t) * no_rows);
	for (int row = 0; row < no_rows; ++row) {
		pthread_mutex_init(&(rows[row].security), NULL);
		rows[row].sum = 0;
	}
	thread_pool_t* pool = malloc(sizeof(thread_pool_t));
	thread_pool_init(pool, THREADS);

	for (int row = 0; row < no_rows; ++row) {
		for (int column = 0; column < no_columns; ++column) {
			runnable_t runnable;
			runnable.arg = &matrix[row][column];
			runnable.argsz = sizeof(field_t);
			runnable.function = &compute_field;
			defer(pool, runnable);
		}
	}

	thread_pool_destroy(pool);
	free(pool);

	for (int row = 0; row < no_rows; ++row) {
		printf("%lld\n", rows[row].sum);
	}

	for (int row = 0; row < no_rows; ++row) {
		free(matrix[row]);
	}
	free(matrix);
	free(rows);
}