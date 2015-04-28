/* Michał Sołtysiak (nr indeksu 347246) */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "err.h"

const int BUFF_SIZE = 15;
const char SPACE[] = " \0";
const int ARG_COUNT = 2;

/* Deskryptory do łącza (Pascal wysyła rozkazy do W(1)) */
int fd_command[2];

/* Deskryptory do łącza (W(1) wysyła wyniki do Pascala) */
int fd_result[2];

/* Numer wiersza w trójkącie Pascala do wyliczenia */
long int n;


/* Tworzy nowe łącza */
void make_pipes()
{
	if (pipe(fd_result) == -1)
		syserr("Error in pipe(fd_result) -> in main process");

	if (pipe(fd_command) == -1)
		syserr("Error in pipe(fd_command) -> in main process");
}

/* Modyfikuje tablicę deskryptorów i zmienia program na W */
void change_to_w()
{
	/* Podmiana standardowego wejścia */
	if (close(0) == -1)
		syserr("Error in close(0) -> in procedure change_to_w()");

	if (dup(fd_command[0]) != 0)
		syserr("Error in dup(fd_command[0]) -> in procedure change_to_w()");

	/* Podmiana standardowego wyścia */
	if (close(1) == -1)
		syserr("Error in close(1) -> in procedure change_to_w()");

	if (dup(fd_result[1]) != 1)
		syserr("Error in dup(fd_result[1]) -> in procedure change_to_w()");

	/* Zamykanie niepotrzebnych deskryptorów */
	if (close(fd_result[0]) == -1)
		syserr("Error in close(fd_result[0]) -> in procedure change_to_w()");

	if (close(fd_command[1]) == -1)
		syserr("Error in close(fd_command[1]) -> in procedure change_to_w()");

	if (close(fd_result[1]) == -1)
		syserr("Error in close(fd_result[1]) -> in procedure change_to_w()");

	if (close(fd_command[0]) == -1)
		syserr("Error in close(fd_command[0]) -> in procedure change_to_w()");

	/* Zlecenie wykonania programu W z parametrem n */
	char n_str[BUFF_SIZE];
	sprintf(n_str, "%ld", n);
	execl("./w", "./w", n_str, NULL);
	syserr("Error in execl()");
}

/* Wysyła do W(1) rozkazy wykonania kolejnych kroków obliczeń */
void send_commands()
{
	long int i;

	for (i = 1; i <= n; i++)
	{
		if (write(fd_command[1], &i, sizeof(i)) == -1)
			syserr("Error in write() -> in procedure send_commands()");
	}
}

/* Wypisuje wartość na ekran */
void print_value(long int i)
{
	char char_buff[BUFF_SIZE];

	sprintf(char_buff, "%ld", i);
	write(1, char_buff, strlen(char_buff));
	write(1, SPACE, strlen(SPACE));
}

/* Wysyła komunikat do procesu W(1), że nie potrzebuje już procesów W */
void let_close()
{
	long int i = 0;

	if (write(fd_command[1], &i, sizeof(i)) == -1)
		syserr("Error in write() -> in procedure let_close()");
}

/* Odbiera kolejne wartości n-tego wiersza trójkąta Pascala */
void get_result()
{
	int cont = 1;
	ssize_t resu;
	long int buff;

	while (cont)
	{
		switch (resu = read(fd_result[0], &buff, sizeof(buff)))
		{
			case -1:
				syserr("Error in read() -> in procedure get_result()");
			break;

			case 0:
				let_close();

				if (close(fd_command[1]) == -1)
					syserr("Error in close(fd_command[1]) -> in procedure get_result()");

				cont = 0;
			break;

			default:
				print_value(buff);
		}
	}
	printf("\n");
				
	if (wait(0) == -1)
		syserr("Error in wait() -> in main process");
}

int main(int argc, char *argv[])
{
	if (argc == ARG_COUNT)
	{
		n = atoi(argv[1]);
		if (n < 1)
		{
			fprintf(stderr, "Error: Argument n <= 0!\n");
			exit(EXIT_FAILURE);
		}
		make_pipes();
		
		switch (fork())
		{
			case -1:
				syserr("Error in fork() -> in main process");
			break;

			case 0:
				change_to_w();
			break;

			default:
				if (close(fd_result[1]) == -1)
					syserr("Error in close(fd_result[1]) -> in main process");

				if (close(fd_command[0]) == -1)
					syserr("Error in close(fd_command[0]) -> in main process");

				send_commands();
				get_result();
		}
	}
	else
	{
		fprintf(stderr, "Usage: %s <row_number>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	return 0;
}