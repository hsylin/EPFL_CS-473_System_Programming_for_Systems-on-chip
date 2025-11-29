#include <defs.h>
#include <taskman/semaphore.h>

#include <implement_me.h>

__global static struct taskman_handler semaphore_handler;


struct wait_data 
{
    struct taskman_semaphore* sem;
    int is_down;  

};

static int impl(struct wait_data* wait_data) 
{
    if (wait_data->is_down) 
    {
        if (wait_data->sem->count > 0) 
        {
            wait_data->sem->count--;  
            return 1;     
        } 
        else 
        {
            return 0;     
        }
    } 
    else 
    {
        if (wait_data->sem->count < wait_data->sem->max) 
        {
            wait_data->sem->count++;   
            return 1;     
        } 
        else 
        {
            return 0;     
        }
    }
}


static int on_wait(struct taskman_handler* handler, void* stack, void* arg) {
    UNUSED(handler);
    UNUSED(stack);

    return impl((struct wait_data*)arg);
}

static int can_resume(struct taskman_handler* handler, void* stack, void* arg) {
    UNUSED(handler);
    UNUSED(stack);

    return impl((struct wait_data*)arg);
}

static void loop(struct taskman_handler* handler) {
    UNUSED(handler);
}

/* END SOLUTION */

void taskman_semaphore_glinit() {
    semaphore_handler.name = "semaphore";
    semaphore_handler.on_wait = &on_wait;
    semaphore_handler.can_resume = &can_resume;
    semaphore_handler.loop = &loop;

    taskman_register(&semaphore_handler);
}

void taskman_semaphore_init(
    struct taskman_semaphore* semaphore,
    uint32_t initial,
    uint32_t max
) {
    semaphore->count = initial;
    semaphore->max = max;
}

void __no_optimize taskman_semaphore_down(struct taskman_semaphore* semaphore) 
{
    struct wait_data w = 
    {
        .sem = semaphore,
        .is_down = 1,
    };

    taskman_wait(&semaphore_handler, &w);
}

void __no_optimize taskman_semaphore_up(struct taskman_semaphore* semaphore) 
{
    struct wait_data w = 
    {
        .sem = semaphore,
        .is_down = 0,
    };


    taskman_wait(&semaphore_handler, &w);

}

