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

#ifndef THREADSAFE_QUEUE
void queueInit(int depth);
void queueDestroy();
void enqueue(frame_desc_t e);
void dequeue(frame_desc_t *e);
bool isQueueEmpty();
bool isQueueFull();
#else

#include <condition_variable>
#include <mutex>
#include <queue>

// A threadsafe-queue.
template <class T>
class SafeQueue
{
public:
    SafeQueue() : q(), m(), c() {}

    ~SafeQueue() {}

    // Add an element to the queue.
    void enqueue(T t)
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(t);
        c.notify_one();
    }

    // Get the front element.
    // If the queue is empty, wait till a element is avaiable.
    T dequeue(void)
    {
        std::unique_lock<std::mutex> lock(m);
        while (q.empty())
        {
            // release lock as long as the wait and reaquire it afterwards.
            c.wait(lock);
        }
        T val = q.front();
        q.pop();
        return val;
    }


private:
    std::queue<T> q;
    mutable std::mutex m;
    std::condition_variable c;
};
#endif //THREADSAFE_QUEUE

#endif /* SRC_INCLUDE_QUEUE_H_ */
