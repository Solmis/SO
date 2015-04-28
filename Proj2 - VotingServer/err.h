// Lekko zmodyfikowany kod ze strony http://mimuw.edu.pl/~janowska/SO-LAB/

#ifndef _ERR_
#define _ERR_

/* wypisuje informacje o blednym zakonczeniu funkcji systemowej 
i konczy dzialanie */
extern void syserr(const char *fmt, ...);
extern void syserr_ext(int bl, const char *fmt, ...);

/* wypisuje informacje o bledzie i konczy dzialanie */
extern void fatal(const char *fmt, ...);

#endif
