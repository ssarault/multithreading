#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>

#define MAX_QUEQUE_SIZE 1024
 
struct Closure;

typedef void (*callback)(struct Closure *);

#define CLOSURE_HEAD \
    callback fn; \
    void (*delete)(struct Closure *);

typedef struct Closure {
    CLOSURE_HEAD
} Closure;

Closure *event_queque[MAX_QUEQUE_SIZE] = { NULL };

size_t queque_head, queque_tail = 0;

bool event_loop_stop = false;

void event_delete(Closure *event)
{
    if (event == NULL)
        return;
    if (event->delete != NULL)
        event->delete(event);
    free(event);
}

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
size_t eventc = 0;

void event_push(Closure *event)
{
    pthread_mutex_lock(&mtx);
    event_queque[queque_tail] = event;
    queque_tail = (queque_tail + 1) % MAX_QUEQUE_SIZE;
    eventc++;
    pthread_mutex_unlock(&mtx);
    pthread_cond_signal(&cond);
}

Closure *event_pop()
{ 
    Closure *event = event_queque[queque_head];
    queque_head = (queque_head + 1) % MAX_QUEQUE_SIZE;
    eventc--;

    return event;
}

void event_stop(pthread_t loop_id)
{
    pthread_mutex_lock(&mtx);
    event_loop_stop = true;
    pthread_mutex_unlock(&mtx);
    pthread_cond_signal(&cond);
    pthread_join(loop_id, NULL);
}

typedef struct Channel
{
    Closure *event;
    bool work;
    bool stop;
    pthread_cond_t cond;
    pthread_mutex_t mtx;
    pthread_t id;
} Channel;

void *worker(void *channel)
{
    Channel *c = channel;
    for(;;) {
        pthread_mutex_lock(&c->mtx);
        while(!c->work) {
            if (c->stop) {
                pthread_mutex_unlock(&c->mtx);
                return NULL;
            }
            pthread_cond_wait(&c->cond, &c->mtx);
        }
        Closure *event = c->event;
        c->work = false;
        pthread_mutex_unlock(&c->mtx);
        event->fn(event);
    }
}

Channel workers[4];

int worker_priority = 0;

void *event_loop(void *not_used)
{
    Closure *event;
    
    for (int i = 0; i < 4; i++) {
        workers[i].stop = false;
        workers[i].work = false;
        pthread_mutex_init(&workers[i].mtx, NULL);
        pthread_cond_init(&workers[i].cond, NULL);
        pthread_create(&workers[i].id, NULL, worker, &workers[i]);
    }
    for(;;) {
        // read, if stop finish processing events then return
        // puts("event loop iteration");
        pthread_mutex_lock(&mtx);

        while(eventc == 0) {
            if (event_loop_stop) {

                pthread_mutex_unlock(&mtx);
                
                for (int i = 0; i < 4; i++) {
                    Channel *worker = &workers[i];
                    pthread_mutex_lock(&worker->mtx);
                    worker->stop = true;
                    pthread_mutex_unlock(&worker->mtx);
                    pthread_cond_signal(&worker->cond);
                    pthread_join(worker->id, NULL);
                }

                return NULL;
            }

            pthread_cond_wait(&cond, &mtx);
        }

        event = event_pop();
        pthread_mutex_unlock(&mtx);

        for (;;) {
            Channel *w = &workers[worker_priority];
            
            if (pthread_mutex_trylock(&w->mtx) != EBUSY) {
                
                if (!w->work) {
                    w->work = true;
                    w->event = event;
                    pthread_mutex_unlock(&w->mtx);
                    pthread_cond_signal(&w->cond);
                    worker_priority = (worker_priority + 1) % 4;
                    break;
                }

                pthread_mutex_unlock(&w->mtx);
            }

            worker_priority = (worker_priority + 1) % 4;
        }
    }
}

struct tester_closure {
    CLOSURE_HEAD
    int x;
    int y;
};

void tester(Closure *c)
{
    struct tester_closure *cc = (struct tester_closure *) c;
    printf("%d, %d\n", cc->x, cc->y);
}

int main(int argc, char **argv)
{
    pthread_t loop_id;
    pthread_create(&loop_id, NULL, event_loop, NULL);
    Closure *events[4];

    puts("in main");

    for(int i = 0; i < 4; i++) {
        struct tester_closure *t = malloc(sizeof(struct tester_closure));
        t->fn = tester;
        t->delete = NULL;
        t->x = i;
        t->y = i + 10;
        events[i] = (Closure *) t;
        event_push((Closure *) t);
    }

    event_stop(loop_id);
    puts("done");

    return 0;
}