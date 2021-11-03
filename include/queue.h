/*
 * queue.h
 *
 *  Created on: Jul 16, 2021
 *      Author: Linh
 */

#ifndef SRC_INCLUDE_QUEUE_H_
#define SRC_INCLUDE_QUEUE_H_

#define MAX_QUEUE_DEPTH		32


#include <stdint.h>
#include <stdbool.h>

//#define THREADSAFE_QUEUE

typedef struct {
	uint16_t* addr;
	uint32_t size;
} frame_desc_t;

void queueInit(int depth);
void queueDestroy();
void enqueue(frame_desc_t e);
void dequeue(frame_desc_t *e);
bool isQueueEmpty();
bool isQueueFull();

#endif /* SRC_INCLUDE_QUEUE_H_ */
