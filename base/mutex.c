#include <stdlib.h>
#include <pthread.h>


void *wg_mutex_create(void)
{
	pthread_mutex_t *mutex;
	mutex = malloc(sizeof(pthread_mutex_t));
	if (mutex != NULL)
		pthread_mutex_init(mutex, NULL);
	return mutex;
}

void wg_mutex_destroy(void *mutex)
{
	pthread_mutex_destroy(mutex);
	free(mutex);
}

void wg_mutex_lock(void *mutex)
{
	pthread_mutex_lock(mutex);
}

void wg_mutex_unlock(void *mutex)
{
	pthread_mutex_unlock(mutex);
}
