#include "atomic_pthread.hh"

int pthread_create(relaxed_atomic<pthread_t>& thread,
				   const pthread_attr_t *attr,
				   void *(start_routine)(void *),
				   void *arg) {
	pthread_t pt;
	int ret = pthread_create(&pt, attr, start_routine, arg);
	thread.store(pt);
	return ret;
}