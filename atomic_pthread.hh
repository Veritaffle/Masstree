#ifndef MASSTREE_ATOMIC_PTHREAD_HH
#define MASSTREE_ATOMIC_PTHREAD_HH 1

#include <pthread.h>
#include "compiler.hh"

int pthread_create(relaxed_atomic<pthread_t>& thread,
				   const pthread_attr_t *attr,
				   void *(start_routine)(void *),
				   void *arg);

#endif