#include <assert.h>
#include <cache.h>
#include <defs.h>
#include <locks.h>
#include <taskman/taskman.h>

#include <implement_me.h>

/// @brief Maximum number of wait handlers.
#define TASKMAN_NUM_HANDLERS 32

/// @brief Maximum number of scheduled tasks.
#define TASKMAN_NUM_TASKS 128

/// @brief Maximum total stack size.
#define TASKMAN_STACK_SIZE (256 << 10)

#define TASKMAN_LOCK_ID 2

#define TASKMAN_LOCK()             \
    do {                           \
        get_lock(TASKMAN_LOCK_ID); \
    } while (0)

#define TASKMAN_RELEASE()              \
    do {                               \
        release_lock(TASKMAN_LOCK_ID); \
    } while (0)

__global static struct {
    /// @brief Wait handlers.
    struct taskman_handler* handlers[TASKMAN_NUM_HANDLERS];

    /// @brief Number of wait handlers;
    size_t handlers_count;

    /// @brief Stack area. Contains multiple independent stacks.
    uint8_t stack[TASKMAN_STACK_SIZE];

    /// @brief Stack offset (for the next allocation).
    size_t stack_offset;

    /// @brief Scheduled tasks.
    void* tasks[TASKMAN_NUM_TASKS];

    /// @brief Number of tasks scheduled.
    size_t tasks_count;

    /// @brief True if the task manager should stop.
    uint32_t should_stop;
} taskman;

/**
 * @brief Extra information attached to the coroutine used by the task manager.
 *
 */
struct task_data {
    struct {
        /// @brief Handler
        /// @note NULL if waiting on `coro_yield`.
        struct taskman_handler* handler;

        /// @brief Argument to the wait handler
        void* arg;
    } wait;

    /// @brief 1 if running, 0 otherwise.
    int running;
};

void taskman_glinit() {
    taskman.handlers_count = 0;
    taskman.stack_offset = 0;
    taskman.tasks_count = 0;
    taskman.should_stop = 0;
}

void* taskman_spawn(coro_fn_t coro_fn, void* arg, size_t stack_sz) {
    // (1) allocate stack space for the new task
    // (2) initialize the coroutine and struct task_data
    // (3) register the coroutine in the tasks array
    // use die_if_not() statements to handle error conditions (like no memory)

    die_if_not(taskman.stack_offset + stack_sz <= TASKMAN_STACK_SIZE);

    TASKMAN_LOCK();
    
    void* new_stack = &taskman.stack[taskman.stack_offset];
    taskman.stack_offset += stack_sz;

    coro_init(new_stack, stack_sz, coro_fn, arg);


    
    die_if_not(taskman.tasks_count + 1 <= TASKMAN_NUM_TASKS);

    taskman.tasks[taskman.tasks_count] = new_stack;
    taskman.tasks_count++;


    struct task_data new_task;
    new_task.wait.handler = NULL;
    new_task.wait.arg = NULL;
    new_task.running = 0;
    *(struct task_data*)coro_data(new_stack) = new_task;
    
    TASKMAN_RELEASE();  

    return new_stack;

}

void taskman_loop() 
{
    // (a) Call the `loop` functions of all the wait handlers.
    // (b) Iterate over all the tasks, and resume them if.
    //        * The task is not complete.
    //        * it yielded using `taskman_yield`.
    //        * the waiting handler says it can be resumed.

    while (1) 
    {
        TASKMAN_LOCK();
        int stop = taskman.should_stop;
        TASKMAN_RELEASE();
        if (stop) break;

        TASKMAN_LOCK();
        size_t tasks_count = taskman.tasks_count;
        size_t handlers_count = taskman.handlers_count;
        struct taskman_handler* local_handlers[TASKMAN_NUM_HANDLERS];
        for (size_t i = 0; i < handlers_count; ++i) 
        {
            local_handlers[i] = taskman.handlers[i];
        }
        TASKMAN_RELEASE();


        for (size_t i = 0; i < handlers_count; ++i) 
        {
            local_handlers[i]->loop(local_handlers[i]);
        }


        // (b) Iterate over all the tasks, and resume them.
        for(int i = 0; i < tasks_count; i++) 
        {
            int should_run = 0;
            void* stack = NULL;
            TASKMAN_LOCK();
            struct task_data* task_data = coro_data(taskman.tasks[i]);  



            //        * The task is not running.
            if(task_data->running == 1)
            {
                TASKMAN_RELEASE();
                continue;
            } 

            //        * The task is not complete.
            if (coro_completed(taskman.tasks[i], NULL)) 
            {
                TASKMAN_RELEASE();
                continue;
            }

            //        * it yielded using `coro_yield`.
            if(task_data->wait.handler == NULL) 
            {
                task_data->running = 1;
                   
                should_run = 1;
            }
            //        * the waiting handler says it can be resumed.
            else if(task_data->wait.handler->can_resume(task_data->wait.handler,taskman.tasks[i], task_data->wait.arg)) 
            {
                task_data->wait.handler = NULL;
                task_data->wait.arg = NULL;
                task_data->running = 1;
                should_run = 1;
            }
            TASKMAN_RELEASE();

            if (should_run) coro_resume(taskman.tasks[i]);
            
        }
    }
}




void taskman_stop() {
    TASKMAN_LOCK();
    taskman.should_stop = 1;
    TASKMAN_RELEASE();
}

void taskman_register(struct taskman_handler* handler) 
{

    die_if_not(handler != NULL);
    die_if_not(taskman.handlers_count < TASKMAN_NUM_HANDLERS);

    TASKMAN_LOCK();
    taskman.handlers[taskman.handlers_count] = handler;
    taskman.handlers_count++;
    TASKMAN_RELEASE();
}

void taskman_wait(struct taskman_handler* handler, void* arg) 
{
    void* stack = coro_stack();
    struct task_data* task_data = coro_data(stack);

    // I suggest that you read `struct taskman_handler` definition.
    // Call handler->on_wait, see if there is a need to yield.
    // Update the wait field of the task_data.
    // Yield if necessary.

    TASKMAN_LOCK();

    if(handler != NULL)
    {
        if(! handler->on_wait(handler,stack, arg ))
        {
            task_data->wait.handler = handler;
            task_data->wait.arg = arg;
            task_data->running = 0;
            TASKMAN_RELEASE();
            coro_yield();
        }
        else
        {
            TASKMAN_RELEASE();
            return;
        }
    }
    else
    {
        task_data->running = 0;
        TASKMAN_RELEASE();
        coro_yield();
    }
}


void taskman_yield() 
{
    taskman_wait(NULL, NULL);
}

void taskman_return(void* result) 
{
    coro_return(result);
}
