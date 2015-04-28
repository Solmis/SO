/* Autor: Michał Sołtysiak (nr indeksu: 347246)
   SO - zadanie zaliczeniowe nr 2 */

#define MAX_BUFF 15

/* KEY_READ - klucz do kolejki, z której serwer czyta typy (pidy procesów)
   KEY_READ_DET - klucz do kolejki, z której serwer czyta dane
   KEY_TO_KOM - klucz do kolejki, do której pisze serwer, a czyta z niej komisja
   KEY_TO_RAP - klucz do kolejki, do której pisze serwer, a czyta z niej raport */
const long KEY_READ = 4242L;
const long KEY_READ_DET = 9494L;
const long KEY_TO_KOM = 3276L;
const long KEY_TO_RAP = 1310L;

/* READ_TYPE_KOM - typ komunikatu do czytania pidów komisji
   READ_TYPE_RAP - typ komunikatu do czytania pidów raportów */
const long READ_TYPE_KOM = 42L;
const long READ_TYPE_RAP = 94L;

typedef struct
{
	long mesg_type;
	char mesg_data[MAX_BUFF];
} Mesg;