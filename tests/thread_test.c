#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>

void* thread_function(void* arg) {
    printf("Thread started, sleeping for 15 seconds...\n");
    sleep(15);
    printf("Thread finished sleeping\n");
    return NULL;
}

int main() {
    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_function, NULL) != 0) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }
    printf("Main thread waiting for worker thread to finish...\n");
    if (pthread_join(thread, NULL) != 0) {
        fprintf(stderr, "Error joining thread\n");
        return 1;
    }
    printf("Thread joined successfully\n");
    return 0;
}
