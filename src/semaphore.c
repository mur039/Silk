#include <semaphore.h>

semaphore_t semaphore_create(int initial_value){
    semaphore_t t;
    t.value = initial_value;
    t.queue = queue_create(5);
    return t;
}


