#ifndef _AUDIOCLIENT_H_
#define _AUDIOCLIENT_H_

/* Auteur : 
	Loic Beaulieu
	Maël PETIT
*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>


/* port utilisé par le serveur */
#define AC_portServ 5000

/* temps en micro seconde  pour le décalage de l'echo */
#define AC_decEcho 4000000

#define AC_echoTailleBuf R_tailleMaxData*(AC_decEcho/100000)

/* % du volume modifier pour l'echo*/
#define AC_volEcho 75


int stringToInt(char * str, int *erreur);

int puissance(int a, int b);

int initFork(int rate, int size, int channels);

void mainFork(int fdMain, int rate, int size, int channels);

#endif