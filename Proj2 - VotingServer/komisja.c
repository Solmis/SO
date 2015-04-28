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


/* m - numer komisji
   i - liczba osób uprawnionych do głosowania w danym lokalu
   j - liczba osób, które oddały głos w danym lokalu (wraz z nieważnymi) */
long m, i, j;

/* msg_rcv_id - id kolejki, z której czyta komisja (od serwera)
   msg_snd_id - id kolejki, do której komisja wstawia swój pid (do serwera)
   msg_snd_det_id - id kolejki, do której pisze komisja (do serwera) */
int msg_rcv_id, msg_snd_id, msg_snd_det_id;

int my_pid;


/* Pobieranie numerów id kolejek */
void init_queues()
{
    if ((msg_rcv_id = msgget(KEY_TO_KOM, 0666)) == -1)
        syserr("Error in msgget (msg_rcv_id)");

    if ((msg_snd_id = msgget(KEY_READ, 0666)) == -1)
        syserr("Error in msgget (msg_snd_id)");

    if ((msg_snd_det_id = msgget(KEY_READ_DET, 0666)) == -1)
        syserr("Error in msgget (msg_snd_det_id)");
}

/* Procedura odbiera informacje zwrotne od serwera i wypisuje podsumowanie */
void summary()
{
    int bytes_rcvd;
    long w, sum_n;
    Mesg mesg;
    
    if ((bytes_rcvd = msgrcv(msg_rcv_id, &mesg, MAX_BUFF, my_pid, 0)) <= 0)
        syserr("Error in msgrcv (in procedure summary())");
    mesg.mesg_data[bytes_rcvd] = '\0';
    w = atol(mesg.mesg_data);

    if ((bytes_rcvd = msgrcv(msg_rcv_id, &mesg, MAX_BUFF, my_pid, 0)) <= 0)
        syserr("Error in msgrcv (in procedure summary())");
    mesg.mesg_data[bytes_rcvd] = '\0';
    sum_n = atol(mesg.mesg_data);

    float frek = j;
    frek /= i;
    frek *= 100;

    printf("Przetworzonych wpisow: %ld\n", w);
    printf("Uprawnionych do glosowania: %ld\n", i);
    printf("Glosow waznych: %ld\n", sum_n);
    printf("Glosow niewaznych: %ld\n", j - sum_n);
    printf("Frekwencja w lokalu: %.2f%%\n", frek);
}

/* Wysyła wartość val podczas sesji z serwerem */
void send_value(long val)
{
    Mesg to_send;
    to_send.mesg_type = my_pid;
    sprintf(to_send.mesg_data, "%ld", val);

    if (msgsnd(msg_snd_det_id, (char*)&to_send, MAX_BUFF, 0) != 0)
        syserr("Error in msgsnd in procedure send_value");
}

int main(int argc, char *argv[])
{
    m = atol(argv[1]);
    my_pid = getpid();
    scanf("%ld %ld", &i, &j);

    init_queues();

    Mesg to_send;
    to_send.mesg_type = READ_TYPE_KOM;
    long l, k, n;

    sprintf(to_send.mesg_data, "%d", my_pid);

    if (msgsnd(msg_snd_id, (char*)&to_send, MAX_BUFF, 0) != 0)
        syserr("Error in msgsnd (sending type(pid)) in process 'komisja'");

    while (scanf("%ld %ld %ld", &l, &k, &n) != EOF)
    {
        send_value(l);
        send_value(k);
        send_value(n);
    }

    send_value(-1L);
    summary();

    exit(0);
}