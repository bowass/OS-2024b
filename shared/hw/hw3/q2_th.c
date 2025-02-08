#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int last = -1, sum = 0, mode;
pthread_mutex_t mutex;

void* f(void* p) {
	if (mode) pthread_mutex_lock(&mutex);
	int v = *(int *) p;
	sum += v;
	last = v;
	if (mode) pthread_mutex_unlock(&mutex);
}

int main(int argc, char** argv) {
	int n = atoi(argv[1]);
	int *x;
	mode = atoi(argv[2]);
	pthread_t tid;
	pthread_mutex_init(&mutex, NULL);
	while (n--) {
		x = (int*) malloc(sizeof(int));
		*x = n;
		pthread_create(&tid, NULL, f, (void*) x);
	}
	printf("%d %d\n", sum, last);
}