#ifndef TASKLIST_H
#define TASKLIST_H

#include <stdint.h>

#define TL_SIZE   64
#define TL_MAX_T  64

typedef struct {
    void (*taskPtr) (void * );
    void * dataPtr;
} task_t;

typedef struct {
    uint8_t head;
    uint8_t tail;
    task_t tasks[TL_SIZE];
} taskList_t;

/* Initialises a blank task list */
taskList_t tlInit(void);

/* Runs the next item in the task list.
 * Returns:
 * 0 on success
 * 1 if no items remain in the task list.
 */
uint8_t tlRun(taskList_t * tl);

/* Adds an item to the task list
 * Parameters:
 * tl - The task list
 * taskPtr - A function pointer
 * dataPtr - a pointer that will be passed to taskPtr when run
 * Returns:
 * 0 on success
 * 1 if no room remains in the task list
 */
uint8_t tlAdd(taskList_t * tl, void (*taskPtr) (void * ), void * dataPtr);

/* Returns the number of items in a task list */
uint8_t tlSize(taskList_t * tl);

#endif
