#include "taskList.h"
#include "hardware/sync.h"
#include "hardware/timer.h"

/* Initialises a blank task list */
taskList_t tlInit(void) {
    // Initialise all values to 0
    taskList_t tl = {0};
    return tl;
}

/* Runs the next item in the task list.
 * Returns:
 * 0 on Success
 * 1 if no items remain in the task list.
 */
uint8_t tlRun(taskList_t * tl) {
    uint8_t nextTail = (tl->tail + 1) % TL_SIZE;
    task_t toRun;

    int ints = save_and_disable_interrupts();

    if (tl->tail == tl->head) {
        // List is empty
        restore_interrupts(ints);
        return 1;
    } else {
        // Run next task
        tl->tail = nextTail;
        toRun = tl->tasks[tl->tail];
        restore_interrupts(ints);
        (toRun.taskPtr) (toRun.dataPtr);
        return 0;
    }
};

/* Adds an item to the task list
 * Parameters:
 * tl - The task list
 * taskPtr - A function pointer
 * dataPtr - a pointer that will be passed to taskPtr when run
 * Returns:
 * 0 on success
 * 1 if no room remains in the task list
 */
uint8_t tlAdd(taskList_t * tl, void (*taskPtr) (void * ), void * dataPtr) {
    uint8_t nextHead = (tl->head + 1) % TL_SIZE;
    task_t toAdd;

    toAdd.taskPtr = taskPtr;
    toAdd.dataPtr = dataPtr;

    int ints = save_and_disable_interrupts();
    if(nextHead == tl->tail) {
        // List is full
        restore_interrupts(ints);
        return 1;
    } else {
        // Add to list
        tl->head = nextHead;
        tl->tasks[nextHead] = toAdd;
        restore_interrupts(ints);
        return 0;
    }
}
