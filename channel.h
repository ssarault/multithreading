#ifndef SSARAULT_GOCHANNEL_H
#define SSARAULT_GOCHANNEL_H

#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct GoChannel {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool blocked;
} GoChannel;

#define DEF_CHANNEL(T) \
typedef struct GoChannel_##T { \
    pthread_mutex_t lock; \
    pthread_cond_t cond; \
    bool blocked; \
    T value; \
} GoChannel_##T; \
GoChannel_##T *make_channel_##T(void);\
void GoChannel_##T##_send(GoChannel_##T *c, T value); \
T GoChannel_##T##_receive(GoChannel_##T *c)

#define DEF_CHANNEL_BUFF(T) \
typedef struct GoChannelBuff_##T { \
    pthread_mutex_t lock; \
    pthread_cond_t send_blocked; \
    pthread_cond_t receive_blocked; \
    size_t capacity; \
    size_t len; \
    size_t head; \
    size_t tail; \
    T values[0]; \
} GoChannelBuff_##T; \
GoChannelBuff_##T *make_channelbuff_##T(size_t sz);\
void GoChannelBuff_##T##_send(GoChannelBuff_##T *c, T value); \
T GoChannelBuff_##T##_receive(GoChannelBuff_##T *c)

#define DEF_MAKE_CHANNEL(T) \
GoChannel_##T *make_channel_##T(void) \
{ \
    GoChannel_##T *c = malloc(sizeof(GoChannel_##T)); \
    c->blocked = false; \
    pthread_mutex_init(&c->lock, NULL); \
    pthread_cond_init(&c->cond, NULL); \
    return c; \
}

#define DEF_MAKE_CHANNEL_BUFF(T) \
GoChannelBuff_##T *make_channelbuff_##T(size_t sz) \
{ \
    GoChannelBuff_##T *c = malloc(sizeof(GoChannelBuff_##T) + sizeof(T) * sz); \
    c->capacity = sz; \
    c->len = 0; \
    pthread_mutex_init(&c->lock, NULL); \
    pthread_cond_init(&c->send_blocked, NULL); \
    pthread_cond_init(&c->receive_blocked, NULL); \
    c->head = c->tail = 0; \
    return c; \
}

#define DEF_CHANNEL_BUFF_SEND(T) \
void GoChannelBuff_##T##_send(GoChannelBuff_##T * c, T value) \
{ \
    pthread_mutex_lock(&c->lock); \
    while(c->capacity == c->len) { \
        puts("sending blocked"); \
        pthread_cond_wait(&c->send_blocked, &c->lock); \
    } \
    puts("sending unblocked"); \
    c->values[c->tail] = value; \
    c->tail = (c->tail + 1) % c->capacity; \
    c->len++; \
    pthread_mutex_unlock(&c->lock); \
    pthread_cond_signal(&c->receive_blocked); \
}

#define DEF_CHANNEL_SEND(T) \
void GoChannel_##T##_send(GoChannel_##T * c, T value) \
{ \
    pthread_mutex_lock(&c->lock); \
    while(c->blocked) { \
        puts("sending blocked"); \
        pthread_cond_wait(&c->cond, &c->lock); \
    } \
    puts("sending unblocked"); \
    c->value = value; \
    c->blocked = true; \
    pthread_mutex_unlock(&c->lock); \
    pthread_cond_signal(&c->cond); \
}

#define DEF_CHANNEL_RECEIVE(T) \
T GoChannel_##T##_receive(GoChannel_##T *c) \
{ \
    pthread_mutex_lock(&c->lock); \
    while(!c->blocked) {\
        puts("receiving blocked"); \
        pthread_cond_wait(&c->cond, &c->lock); \
    } \
    puts("receiving unblocked"); \
    T value = c->value; \
    c->blocked = false; \
    pthread_mutex_unlock(&c->lock); \
    pthread_cond_signal(&c->cond); \
    return value; \
}

#define DEF_CHANNEL_BUFF_RECEIVE(T) \
T GoChannelBuff_##T##_receive(GoChannelBuff_##T *c) \
{ \
    pthread_mutex_lock(&c->lock); \
    while(c->len == 0) {\
        puts("receiving blocked"); \
        pthread_cond_wait(&c->receive_blocked, &c->lock); \
    } \
    puts("receiving unblocked"); \
    T value = c->values[c->head]; \
    c->head = (c->head + 1) % c->capacity; \
    c->len--; \
    pthread_mutex_unlock(&c->lock); \
    pthread_cond_signal(&c->send_blocked); \
    return value; \
}

#define DEF_CHANNEL_ALL(T) \
DEF_CHANNEL(T); \
DEF_MAKE_CHANNEL(T) \
DEF_CHANNEL_SEND(T) \
DEF_CHANNEL_RECEIVE(T)

#define DEF_CHANNEL_BUFF_ALL(T) \
DEF_CHANNEL_BUFF(T); \
DEF_MAKE_CHANNEL_BUFF(T) \
DEF_CHANNEL_BUFF_SEND(T) \
DEF_CHANNEL_BUFF_RECEIVE(T)

#endif
