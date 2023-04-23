#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include "eventbuf.c"

struct eventbuf *event_buffer;

int num_prod;         // number of producers
int num_cons;         // number of consumers
int num_events;       // number of events per producer
int event_queue_size; // number of events to be active at once

sem_t *items;
sem_t *mutex;
sem_t *spaces;

sem_t *sem_open_temp(const char *name, int value)
{
    sem_t *sem;

    // Create the semaphore
    if ((sem = sem_open(name, O_CREAT, 0600, value)) == SEM_FAILED)
        return SEM_FAILED;

    // Unlink it so it will go away after this process exits
    if (sem_unlink(name) == -1)
    {
        sem_close(sem);
        return SEM_FAILED;
    }

    return sem;
}

void *producer(void *arg)
{
    int *producer_number = arg;
    int event_number = 0;

    while (event_number < num_events)
    {
        int event = *producer_number * 100 + event_number;

        sem_wait(spaces); // waiting for room in queue
        sem_wait(mutex);  // mutex around event buff

        printf("P%i: adding event %i\n", *producer_number, event);
        eventbuf_add(event_buffer, event);

        sem_post(mutex);
        sem_post(items); // signal to consumers that a new event has been added
        event_number += 1;
    }

    printf("P%i: exiting\n", *producer_number);
    return NULL;
}

void *consumer(void *arg)
{
    int *consumer_number = arg;

    while (1)
    {
        sem_wait(items); // waiting for item signal
        sem_wait(mutex); // locking mutex

        // if buffer is empty and items was signaled, time to quit
        if (eventbuf_empty(event_buffer))
        {
            sem_post(mutex);
            break;
        }

        int event = eventbuf_get(event_buffer);
        printf("C%i: got event %i\n", *consumer_number, event);

        sem_post(mutex);
        sem_post(spaces); // signal to producers that there is room in queue.
    }

    printf("C%i: exiting\n", *consumer_number);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("usage: ./pcseml num_prod num_cons num_events event_queue_size\n");
        return 1;
    }

    num_prod = atoi(argv[1]);
    num_cons = atoi(argv[2]);
    num_events = atoi(argv[3]);
    event_queue_size = atoi(argv[4]);

    // creating initial event buffer queue
    event_buffer = eventbuf_create();

    // initializing semaphores
    items = sem_open_temp("items", 0);
    mutex = sem_open_temp("items", 1);
    spaces = sem_open_temp("items", event_queue_size);

    // allocating thread array and thread id array for producer threads
    pthread_t *prod_threads = calloc(num_prod, sizeof *prod_threads);
    int *prod_thread_ids = calloc(num_prod, sizeof *prod_thread_ids);
    // creating producer threads
    for (int i = 0; i < num_prod; i++)
    {
        prod_thread_ids[i] = i;
        pthread_create(prod_threads + i, NULL, producer, prod_thread_ids + i);
    }

    // allocating thread array and thread id array for consumer threads
    pthread_t *cons_threads = calloc(num_cons, sizeof *cons_threads);
    int *cons_thread_ids = calloc(num_cons, sizeof *cons_thread_ids);
    // creating consumer threads
    for (int i = 0; i < num_cons; i++)
    {
        cons_thread_ids[i] = i;
        pthread_create(cons_threads + i, NULL, consumer, cons_thread_ids + i);
    }

    // waiting for all producers to finish
    for (int i = 0; i < num_prod; i++)
    {
        pthread_join(prod_threads[i], NULL);
    }

    // signaling all consumers to finish
    for (int i = 0; i < num_cons; i++)
    {
        sem_post(items);
    }
    // waiting for all consumers to finish
    for (int i = 0; i < num_cons; i++)
    {
        pthread_join(cons_threads[i], NULL);
    }

    // freeing event queue
    eventbuf_free(event_buffer);

    return 0;
}