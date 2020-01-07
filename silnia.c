#include "future.h"
const int THREADS = 3;

typedef struct pair {
	long long first;
	long long second;
} pair_t;

void* multiply(void* arg, size_t argsz __attribute__((unused)), size_t* result_size __attribute__((unused))) {
	pair_t* pair = arg;
	pair->second *= pair->first;
	pair->first += 3;
	return pair;
}

int main() {
	int xx = 0;

	int n;
	scanf("%d", &n);
	fflush(stdout);
	thread_pool_t* pool = malloc(sizeof(thread_pool_t));
	thread_pool_init(pool, THREADS);

	future_t* futures = malloc(sizeof(future_t) * (n + 1));

	callable_t callable;
	callable.function = &multiply;
	callable.argsz = sizeof(pair_t);

	fprintf(stderr, "X: %d\n", xx);
	++xx;

	pair_t products[3];
	for (int i = 0; i < 3; ++i) {
		products[i].first = i + 1;
		products[i].second = 1;
		callable.arg = &products[i];
		async(pool, &futures[i + 1], callable);
	}

	fprintf(stderr, "X: %d\n", xx);
	++xx;

	for (int i = 4; i <= n; ++i) {
		callable.arg = &products[(i - 1) % 3];
		futures[i].callable = callable;
		map(pool, &futures[i], &futures[i - 3], &multiply);
	}

	fprintf(stderr, "X: %d\n", xx);
	++xx;

	for (int i = 1; i < n; ++i) {
		await(&futures[i]);
	}

	fprintf(stderr, "X: %d\n", xx);
	++xx;

	void* tmp = await(&futures[n]);
	pair_t* pair = tmp;
	long long result = pair->second;

	fprintf(stderr, "X: %d\n", xx);
	++xx;

	printf("%lld", result);
	fflush(stdout);

	thread_pool_destroy(pool);
	free(pool);
	free(futures);
}
