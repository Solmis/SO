/* Autor: Michał Sołtysiak (nr indeksu: 347246)
   SO - zadanie zaliczeniowe nr 2 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "err.h"
#include "komunikaty.h"


/* l - opcjonalny parametr oznaczający numer listy (> 0) */
long l = 0;

/* msg_rcv_id - id kolejki, z której czyta raport (od serwera)
   msg_snd_id - id kolejki, do której raport wstawia swój pid (do serwera)
   msg_snd_det_id - id kolejki, do której pisze raport (do serwera) */
int msg_rcv_id, msg_snd_id, msg_snd_det_id;

int my_pid;

/* Pobieranie numerów id kolejek */
void init_queues()
{
    if ((msg_rcv_id = msgget(KEY_TO_RAP, 0666)) == -1)
        syserr("Error in msgget (msg_rcv_id)");

    if ((msg_snd_id = msgget(KEY_READ, 0666)) == -1)
        syserr("Error in msgget (msg_snd_id)");

    if ((msg_snd_det_id = msgget(KEY_READ_DET, 0666)) == -1)
        syserr("Error in msgget (msg_snd_det_id)");
}

/* Odczytuje kolejną wartość z kolejki i konwertuje ją na inta, którego zwraca */
long read_next()
{
    Mesg mesg;
    int bytes_rcvd;

    if ((bytes_rcvd = msgrcv(msg_rcv_id, &mesg, MAX_BUFF, my_pid, 0)) <= 0)
        syserr("Error in msgrcv (in procedure read_next())");
    mesg.mesg_data[bytes_rcvd] = '\0';

    return atol(mesg.mesg_data);
}

/* Procedura odbiera informacje od serwera i wypisuje je na stdout */
void response()
{
    long x, M , y, z, v, K;

    /* Pobieramy liczbę K kandydatów na liście */
    K = read_next();
    M = read_next();
    x = read_next();
    y = read_next();
    z = read_next();
    v = read_next();

    float frek = z + v;

    if (y != 0)
        frek /= y;
    else
        frek = 0;

    frek *= 100;

    printf("Przetworzonych komisji: %ld / %ld\n", x, M);
    printf("Uprawnionych do glosowania: %ld\n", y);
    printf("Glosow waznych: %ld\n", z);
    printf("Glosow niewaznych: %ld\n", v);
    printf("Frekwencja: %.2f%%\n", frek);
    printf("Wyniki poszczegolnych list:\n");

    long nr_listy;
    int i;
    while (1)
    {
        nr_listy = read_next();

        if (nr_listy <= 0)
            break;
        else
        {
            printf("%ld %ld", nr_listy, read_next());

            for (i = 0; i < K; i++)
                printf(" %ld", read_next());
        }
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1)
        l = atol(argv[1]);

    my_pid = getpid();
    init_queues();

    Mesg to_send;
    to_send.mesg_type = READ_TYPE_RAP;

    sprintf(to_send.mesg_data, "%d", my_pid);

    if (msgsnd(msg_snd_id, (char*)&to_send, MAX_BUFF, 0) != 0)
        syserr("Error in msgsnd (sending type(pid)) in process 'raport'");

    to_send.mesg_type = my_pid;
    sprintf(to_send.mesg_data, "%ld", l);

    if (msgsnd(msg_snd_det_id, (char*)&to_send, MAX_BUFF, 0) != 0)
        syserr("Error in msgsnd (sending l) in process 'raport'");

    response();

    exit(0);
}