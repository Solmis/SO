/* Michał Sołtysiak (nr indeksu 347246) */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "err.h"

const int ARG_COUNT = 2;

/* Rozkazy czytane będą z łącza, które zostało podmienione ze standardowym wejściem */
const int FD_COMMAND = 0;

/* Wynik wypisywany będzie do łącza, które zostało podmienione ze standardowym wyjściem */
const int FD_RESULT = 1;


/* Liczba procesów W (do utworzenia) */
long int n;

/* Numer procesu (indeks) */
long int nr = 1;

/* Wartość przechowywana w bieżącym węźle */
long int value = 1;

/* Flaga, ktora okresla, czy wynik z danego procesu zostal juz wyslany */
int result_sent = 0;

/* Dla W(i), i < n: Deskryptory do komunikacji z procesem W(i+1) */
int fd_write_to_next[2];
int fd_read_from_next[2];


/* Tworzy nowe łącza */
void make_pipes()
{
	if (pipe(fd_write_to_next) == -1)
		syserr("Error in pipe(fd_write_to_next) -> in process W");

	if (pipe(fd_read_from_next) == -1)
		syserr("Error in pipe(fd_read_from_next) -> in process W");
}

/* Tworzy n-1 procesów W, by w sumie było ich n */
void create_processes(long int n)
{
	if (nr < n)
	{
		make_pipes();

		switch(fork())
		{
			case -1:
				syserr("Error in fork() -> in process W");

			case 0:
				/* Podmiana standardowego wejścia */
				if (close(0) == -1)
					syserr("Error in close(0) -> in process W");

				if (dup(fd_write_to_next[0]) != 0)
					syserr("Error in dup(fd_write_to_next[0]) -> in process W");

				/* Podmiana standardowego wyjścia */
				if (close(1) == -1)
					syserr("Error in close(1) -> in process W");

				if (dup(fd_read_from_next[1]) != 1)
					syserr("Error in dup(fd_read_from_next[1]) -> in process W");

				/* Zamykanie niepotrzebnych deskryptorów */
				if (close(fd_write_to_next[0]) == -1)
					syserr("Error in close(fd_write_to_next[1]) -> in process W");

				if (close(fd_write_to_next[1]) == -1)
					syserr("Error in close(fd_write_to_next[1]) -> in process W");

				if (close(fd_read_from_next[0]) == -1)
					syserr("Error in close(fd_read_from_next[0]) -> in process W");

				if (close(fd_read_from_next[1]) == -1)
					syserr("Error in close(fd_read_from_next[0]) -> in process W");

				value = 1;
				nr++;
				create_processes(n);
			break;

			default:
				if (close(fd_write_to_next[0]) == -1)
					syserr("Error in close(fd_write_to_next[0]) -> in process W");

				if (close(fd_read_from_next[1]) == -1)
					syserr("Error in close(fd_read_from_next[1]) -> in process W");
		}
	}
}

/* Dla W(i): przekazuje wartość do W(i-1) lub Pascala, jeśli i = 1 */
void send_value_up(long int next_value)
{
	if (write(FD_RESULT, &next_value, sizeof(next_value)) == -1)
		syserr("Error in write() -> in send_value_up()");
}

/* Wykonuje rozkazy wykonania kolejnych kroków obliczeń */
void execute_commmand(long int k)
{
	if (k == nr)
	{
		value = 1;
	}
	else if (k > nr)
	{
		long int neg_prev;

		/* Przekazywanie rozkazu dalej (aż do procesu o numerze k) */
		if (write(fd_write_to_next[1], &k, sizeof(k)) == -1)
			syserr("Error in write() [sending command to next process] -> in execute_commmand()");

		/* Wysyłanie swojej wartości (zanegowanej) do następnego procesu, jeśli będzie potrzebna */
		if (nr < k-1)
		{
			long int neg_val = value * (-1);
			if (write(fd_write_to_next[1], &neg_val, sizeof(neg_val)) == -1)
				syserr("Error in write() [sending value to next process] -> in execute_commmand()");
		}

		/* Odczytywanie wartości od poprzedniego procesu i aktualizacja własnej */
		if (nr > 1)
		{
			if (read(FD_COMMAND, &neg_prev, sizeof(neg_prev)) == -1)
				syserr("Error in read() [reading 'neg_prev'] -> in execute_commmand()");

			/* Odejmowanie wartości zanegowanej <=> dodawanie nie zanegowanej */
			value -= neg_prev;
		}
	}
}

/* Funkcja wysyła rozkaz podania wartości przez następny proces. Następnie odbiera
   i przekazuje dalej otrzymane wartości, po czym wysyła swoją wartość */
void result_flow(long int n)
{
	long int k;
	ssize_t resu;

	if (nr < n)
	{
		/* Wysyłanie rozkazu podania wyniku */
		k = 0;
		if (write(fd_write_to_next[1], &k, sizeof(k)) == -1)
			syserr("Error in write() [command '0'] -> in result_flow()");

		while (1)
		{
			switch(resu = read(fd_read_from_next[0], &k, sizeof(k)))
			{
				case -1:
					syserr("Error in read() -> in result_flow()");

				case 0:
					send_value_up(value);
					result_sent = 1;

					if (close(FD_RESULT) == -1)
						syserr("Error in close() [closing FD_RESULT] -> in result_flow()");
				return;

				default:
					send_value_up(k);
			}
		}
	}
	else
	{
		result_sent = 1;
		send_value_up(value);
		
		if (close(FD_RESULT) == -1)
			syserr("Error in close() [closing FD_RESULT of process N] -> in result_flow()");
	}
}

/* Interpretuje polecenie, zwraca 0, gdy proces może się zakończyć lub 1 wpp */
int parse_command(long int k)
{
	int resu = 1;

	/* Rozkaz '0' - zwrócenia rezultatu (za pierwszym razem) lub zakończenia pracy (za drugim) */
	if (k == 0)
	{
		if (!result_sent)
			result_flow(n);
		else
		{
			if (close(fd_write_to_next[1]) == -1)
				syserr("Error in close() -> in parse_command()");

			resu = 0;
		}
	}
	else
	{
		execute_commmand(k);

		/* Jeśli wykonujemy n-ty krok obliczeń i jesteśmy W(1) to inicjujemy wypisanie wiersza */
		if ((k == n) && (nr == 1))
		{
			if (!result_sent)
				result_flow(n);
		}
	}

	return resu;
}

int main(int argc, char *argv[])
{
	if (argc == ARG_COUNT)
	{
		n = atoi(argv[1]);
		create_processes(n);

		int cont = 1;
		ssize_t resu;
		long int k;

		while (cont)
		{
			switch (resu = read(FD_COMMAND, &k, sizeof(k)))
			{
				case -1:
					syserr("Error in read() -> in main() of process W");
				break;

				case 0:
					cont = 0;
					/* Zamykanie swojego łącza do wysyłania rozkazów dalej */
					if (nr != n)
					{
						if (close(fd_write_to_next[1]) == -1)
							syserr("Error in close() -> in main() of process W");
					}
				break;

				default:
					cont = parse_command(k);
			}
		}

		if (nr != n)
		{
			if (wait(0) == -1)
				syserr("Error in wait() -> in process W");
		}
	}
	else
	{
		fprintf(stderr, "Usage: %s <row_number>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	return 0;
}