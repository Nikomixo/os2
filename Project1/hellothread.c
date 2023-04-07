#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

void *run(void *arg)
{
    for (int i = 1; i <= 4; i++)
    {
        printf("%s: %d\n", (char *)arg, i);
        sleep(0);
    }

    return NULL;
}

int main(void)
{
    pthread_t t1;
    pthread_t t2;

    printf("Launching threads\n");

    pthread_create(&t1, NULL, run, "thread 1");
    pthread_create(&t2, NULL, run, "thread 2");

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Threads complete!\n");

    return 0;
}