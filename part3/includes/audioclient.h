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

/* temps en dixième de seconde pour le décalage de l'echo */
#define AC_decEcho 5 // == 0.5 sec, 

/* % du volume modifier pour l'echo*/
#define AC_volEcho 75


/* 
	Initialise un processus enfant qui va s'occupé de l'echo
	renvois le descripteur de fichier permetant de lui envoyer le buffer réserver pour l'echo
*/
int initFork(int rate, int size, int channels);


/* 
	fonction pour définir un usleep non bloquand dans le processus qui s'occupe de l'echo
*/
void finDelaiEcho(int sig);

/* 
	fonction appelé pour le processus qui va géré l'echo
	fdMain est le descripteur de fichier permetant de recevoir le buffer de l'echo envoyer par le processus principal
*/
void mainForkEcho(int fdMain, int rate, int size, int channels);


/*
	Convertie une chaine de caractère composé de chiffre.
	Si ce n'est pas un nombre dans str (ex : "12ab", "a", "a12", ...), erreur est mis à 1
*/
int stringToInt(char * str, int *erreur);

/*
	renvois a puissance b 
*/
int puissance(int a, int b);

#endif