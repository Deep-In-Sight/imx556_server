
#include "queue.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define ABS(x) ((x>0)?(x):-(x))

//#define QDBG
#ifdef QDBG
#define QDBG_ENTER printf(">> Enter %s\n", __FUNCTION__)
#define QDBG_EXIT printf("<< Exit %s\n", __FUNCTION__)
#define QDBG_LINE printf("... %s Line %d\n", __FUNCTION__, __LINE__)
#define QDBG_ENTER
#define QDBG_EXIT
#define QDBG_LINE
#define QDBG_PRINT(...) printf(__VA_ARGS__)
#else
#define QDBG_ENTER
#define QDBG_EXIT
#define QDBG_LINE
#define QDBG_PRINT(msg, ...)
#endif

typedef struct
{
	int32_t head;
	int32_t tail;
	int32_t count;
	int32_t	maxElem;
	int32_t total;
}queueStatus_t;

static queueStatus_t qStatus;
static frame_desc_t* q;
static pthread_mutex_t lock;
static pthread_cond_t cv_full;
static pthread_cond_t cv_empty;

void cleanQueue() {
	QDBG_ENTER;

	QDBG_EXIT;
}

void queueInit(int depth)
{
	QDBG_ENTER;

	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&cv_full, NULL);
	pthread_cond_init(&cv_empty, NULL);

	QDBG_LINE;

	q = (frame_desc_t *)malloc(depth * sizeof (frame_desc_t));

	qStatus.maxElem = depth;
	qStatus.head = 0;
	qStatus.tail = 0;
	qStatus.count = 0;
	qStatus.total = 0;

	for (int i = 0; i < depth; i++) {
		q[i].addr = 0;
		q[i].size = 0;
	}

	QDBG_EXIT;
}

void queueDestroy() {
	int ret;

	QDBG_ENTER;

	ret = pthread_cond_destroy(&cv_full);
	ret = pthread_cond_destroy(&cv_empty);
	ret = pthread_mutex_destroy(&lock);

	free(q);

	printf("Dequeued %d frames\n", qStatus.total);

	QDBG_EXIT;
}

//---------------------------------------------------------------------------

void enqueue(frame_desc_t e)
{
	QDBG_ENTER;

//	int full = 0;
	pthread_mutex_lock(&lock);

	while (isQueueFull()) {
		QDBG_PRINT("########## QUEUE FULL ##########\n");
		pthread_cond_wait(&cv_full, &lock);
//		if (!full) {
//			full = 1;
//		} else {
//			return;
//		}
//		usleep(20000);
	};

	QDBG_PRINT("Enqueue frame %p at queue index %d\n", e.addr, qStatus.head);

	q[qStatus.head] = e;
	qStatus.head = (qStatus.head + 1) % qStatus.maxElem;

	if (qStatus.count == 0) {
		pthread_cond_signal(&cv_empty);
	}
	qStatus.count++;

	QDBG_LINE;
	pthread_mutex_unlock(&lock);

	QDBG_EXIT;
}

//---------------------------------------------------------------------------

void dequeue(frame_desc_t *e)
{
	QDBG_ENTER;
//	pid_t tid = syscall(__NR_gettid);

	pthread_mutex_lock(&lock);
//	int empty = 0;
	while (isQueueEmpty()) {
		QDBG_PRINT("######### QUEUE Empty ##########\n");
		pthread_cond_wait(&cv_empty, &lock);
//		if (!empty) {
//			empty = 1;
//		} else {
//			return;
//		}
//		usleep(1000);
	};
	QDBG_LINE;



	*e = q[qStatus.tail];

	QDBG_PRINT("Dequeue frame %p at queue index %d\n", e->addr, qStatus.tail);

	qStatus.tail = (qStatus.tail + 1) % qStatus.maxElem;

	if (qStatus.count == qStatus.maxElem) {
		pthread_cond_signal(&cv_full);
	}
	qStatus.count--;
	qStatus.total++;
	QDBG_LINE;


	pthread_mutex_unlock(&lock);

	QDBG_EXIT;
	return;
}


//---------------------------------------------------------------------------

bool isQueueFull(void)
{
	bool full;

	QDBG_ENTER;
//	pthread_mutex_lock(&lock);

	QDBG_LINE;
	full = (qStatus.count == qStatus.maxElem);

//	pthread_mutex_unlock(&lock);
	QDBG_EXIT;
	return full;
}


//---------------------------------------------------------------------------

bool isQueueEmpty(void)
{
	bool empty;

	QDBG_ENTER;

//	pthread_mutex_lock(&lock);
	QDBG_LINE;
	empty = (qStatus.count == 0);

//	pthread_mutex_unlock(&lock);
	QDBG_EXIT;
	return empty;
}



/// @}
