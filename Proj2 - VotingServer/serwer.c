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
#include <pthread.h>

#include "err.h"
#include "komunikaty.h"

#define L_MAX 100L
#define K_MAX 100L
#define MAX_THREADS 100

/* L - liczba list
   K - liczba kandydatów na jednej liście
   M - liczba komisji */
long L, K, M;

/* msg_rcv_id - id kolejki, z której czyta serwer (tylko typy komunikatow)
   msg_rcv_det_id - id kolejki, z której czyta serwer (konkretne dane)
   msg_snd_rap_id - id kolejki, do której pisze serwer, a czyta z niej raport
   msg_snd_kom_id - id kolejki, do której pisze serwer, a czyta z niej komisja */
int msg_rcv_id, msg_rcv_det_id, msg_snd_rap_id, msg_snd_kom_id;


/* Struktura trzymająca dane o przebiegu głosowania */
typedef struct
{
    long committees_served;
    long could_vote;
    long valid_votes;
    long invalid_votes;
    long votes[L_MAX][K_MAX];
} Vote_db;

Vote_db db;


pthread_attr_t attr;
pthread_rwlock_t rwlock;


/* Obsłużenie przerwania procesu - m.in. usuwanie kolejek */
void exit_server(int sig) 
{
    int thr_err;

    if (msgctl(msg_rcv_id, IPC_RMID, 0) == -1)
        syserr("Error in msgctl RMID (msg_rcv_id)");

    if (msgctl(msg_rcv_det_id, IPC_RMID, 0) == -1)
        syserr("Error in msgctl RMID (msg_rcv_det_id)");

    if (msgctl(msg_snd_rap_id, IPC_RMID, 0) == -1)
        syserr("Error in msgctl RMID (msg_snd_rap_id)");

    if (msgctl(msg_snd_kom_id, IPC_RMID, 0) == -1)
        syserr("Error in msgctl RMID (msg_snd_kom_id)");

    if ((thr_err = pthread_attr_destroy(&attr)) != 0)
        syserr_ext(thr_err, "attrdestroy");

    exit(0);
}

/* Pobieranie numerów id kolejek */
void init_queues()
{
    if ((msg_rcv_id = msgget(KEY_READ, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
        syserr("Error in msgget (msg_rcv_id)");

    if ((msg_rcv_det_id = msgget(KEY_READ_DET, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
        syserr("Error in msgget (msg_rcv_det_id)");

    if ((msg_snd_rap_id = msgget(KEY_TO_RAP, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
        syserr("Error in msgget (msg_snd_rap_id)");

    if ((msg_snd_kom_id = msgget(KEY_TO_KOM, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
        syserr("Error in msgget (msg_snd_kom_id)");
}

/* Pobiera kolejną wartość o typie type z kolejki o id msg_rcv_det_id */
long read_next(long type)
{
    Mesg mesg;
    int bytes_rcvd;

    if ((bytes_rcvd = msgrcv(msg_rcv_det_id, &mesg, MAX_BUFF, type, 0)) <= 0)
        syserr("Error in msgrcv in procedure read_next");
    mesg.mesg_data[bytes_rcvd] = '\0';

    return atol(mesg.mesg_data);
}

/* Wysyła komunikat z wartością int i typem type do kolejki o id q_id */
void send_val(int q_id, long type, long val)
{
    Mesg to_send;
    to_send.mesg_type = type;
    sprintf(to_send.mesg_data, "%ld", val);

    if (msgsnd(q_id, (char*)&to_send, MAX_BUFF, 0) != 0)
        syserr("Error in msgsnd in procedure send_int");
}

/* Funkcja obsługująca sesję z komisją */
void* serve_committee(void* mesg_type)
{
    long msg_type = atol(mesg_type);
    long l, k, n, rows = 0, sum = 0;

    while (1)
    {
        l = read_next(msg_type);

        if (l == -1)
            break;

        k = read_next(msg_type);
        n = read_next(msg_type);
        rows++;
        sum += n;
    }

    send_val(msg_snd_kom_id, msg_type, rows);
    send_val(msg_snd_kom_id, msg_type, sum);
    //procesy--;

    //(zapisujemy dane)
    return 0;
}

/* Funkcja obsługująca sesję z raportem */
void* serve_report(void* mesg_type)
{
    long msg_type = atol(mesg_type);
    long l = read_next(msg_type);
    long x, y, z, v;
    int i, j;
    long ** rows;

    if (l > 0)
    {
        rows = malloc(sizeof(long));
        rows[0] = malloc(sizeof(long) * K);
    }
    else
    {
        rows = malloc(sizeof(long) * L);

        for (i = 0; i < L; i++)
            rows[i] = malloc(sizeof(long) * K);
    }

    send_val(msg_snd_rap_id, msg_type, K);
    send_val(msg_snd_rap_id, msg_type, M);

    pthread_rwlock_rdlock(&rwlock);
    x = db.committees_served;
    y = db.could_vote;
    z = db.valid_votes;
    v = db.invalid_votes;

    if (l > 0)
    {
        for (i = 0; i < K; i++)
            rows[0][i] = db.votes[l-1][i];
    }
    else
    {
        for (i = 0; i < L; i++)
            for (j = 0; j < K; j++)
                rows[i][j] = db.votes[i][j];
    }
    pthread_rwlock_unlock(&rwlock);
    
    send_val(msg_snd_rap_id, msg_type, x);
    send_val(msg_snd_rap_id, msg_type, y);
    send_val(msg_snd_rap_id, msg_type, z);
    send_val(msg_snd_rap_id, msg_type, v);

    if (l > 0)
    {
        for (i = 0; i < K; i++)
            send_val(msg_snd_rap_id, msg_type, rows[0][i]);
    }
    else
    {
        for (i = 0; i < L; i++)
            for (j = 0; j < K; j++)
                send_val(msg_snd_rap_id, msg_type, rows[i][j]);
    }

    if (l > 0)
        free(rows[0]);
    else
    {
        for (i = 0; i < L; i++)
            free(rows[i]);
    }
    free(rows);

    return 0;
}

/* Ustawia atrybut detached */
void make_attr_detached()
{
    int thr_err;

    if ((thr_err = pthread_attr_init(&attr)) != 0)
        syserr_ext(thr_err, "Error in pthread_attr_init");

    if ((thr_err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0)
        syserr_ext(thr_err, "Error in pthread_attr_setdetachstate");
}

int main(int argc, char *argv[])
{
    L = atol(argv[1]);
    K = atol(argv[2]);
    M = atol(argv[3]);

    if (signal(SIGINT, exit_server) == SIG_ERR)
        syserr("Error in signal (SIGINT)");

    init_queues();

    int thr_err;
    pthread_t thread_id;
    make_attr_detached();
    if (thr_err = pthread_rwlock_init(&rwlock, NULL))
        syserr_ext(thr_err, "Error in function pthread_rwlock_init");

    Mesg mesg;
    int bytes_rcvd;
    while (1)
    {
        if ((bytes_rcvd = msgrcv(msg_rcv_id, &mesg, MAX_BUFF, 0, 0)) <= 0)
            syserr("Error in msgrcv (receiving type(pid))");
        mesg.mesg_data[bytes_rcvd] = '\0';

        if (mesg.mesg_type == READ_TYPE_KOM)
        {
            if ((thr_err = pthread_create(&thread_id, &attr, serve_committee, &mesg.mesg_data)) != 0)
                syserr_ext(thr_err, "Error in pthread_create (for serve_committee)");
        }
        else
        {
            if ((thr_err = pthread_create(&thread_id, &attr, serve_report, &mesg.mesg_data)) != 0)
                syserr_ext(thr_err, "Error in pthread_create (for serve_report)");
        }
    }

    exit_server(0);
}