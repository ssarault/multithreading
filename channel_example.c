#include "channel.h"
#include <stdio.h>

DEF_CHANNEL_ALL(int)
DEF_CHANNEL_BUFF_ALL(int)

void *func(void *chint)
{
    GoChannel_int *c = chint;
    GoChannel_int_send(c, 10);
    GoChannel_int_send(c, 20);
}

void *funcer(void *chint)
{
    GoChannel_int *c = chint;
    GoChannel_int_send(c, 30);
    GoChannel_int_send(c, 40);
}

void *buff_test(void *chbuffint)
{
    GoChannelBuff_int *c = chbuffint;
    GoChannelBuff_int_send(c, 100);
    GoChannelBuff_int_send(c, 200);
    GoChannelBuff_int_send(c, 300);
}

int main(int argc, char **argv)
{
    GoChannel_int *c = make_channel_int();
    GoChannelBuff_int *bc = make_channelbuff_int(2);
    pthread_t go_id, go_id_two, go_id_three;
    pthread_create(&go_id, NULL, func, c);
    pthread_create(&go_id_two, NULL, funcer, c);
    int val = GoChannel_int_receive(c);
    printf("%d\n", val);
    val = GoChannel_int_receive(c);
    printf("%d\n", val);
    val = GoChannel_int_receive(c);
    printf("%d\n", val);
    val = GoChannel_int_receive(c);
    printf("%d\n", val);
    pthread_create(&go_id_three, NULL, buff_test, bc);
    val = GoChannelBuff_int_receive(bc);
    printf("%d\n", val);
    val = GoChannelBuff_int_receive(bc);
    printf("%d\n", val);
    val = GoChannelBuff_int_receive(bc);
    printf("%d\n", val);
    return 0;
}