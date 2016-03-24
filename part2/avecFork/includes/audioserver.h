#ifndef _AUDIOSERVER_H_
#define _AUDIOSERVER_H_

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#include "requete.h"
#include "audio.h"

#define AS_portServer 5000
#define AS_nbrMaxClient 5



/*
	pid : permet de définir le pid du processus enfant. 
		Utilisé lorsque le processus principal doit tuer tout ces enfant lorsque le serveur doit s'arreter.

	fdProc : descripteur de fichier permetant au processus principal de comuniquer avec le processus enfant.

	utiliser : boolean permetant de voir si le processus enfant est réserver à un client
*/
typedef struct proc {
	pid_t pid;
	int fdProc;
	int utiliser;
} procFork;

// Arret du serveur proprement lorsque le signal SIGINT (ctrl + c) est reçu 
// ou lorsqu'un processus enfant s'arrete
void arretServeur(int sig);

// initialise les processus enfant qui devront s'occuper des clients
int initListeFork(int fdSocket);

/* 
	Fonction directement appeler dans un processus enfant.
	fdMain correspond au descripteur de fichier permetant au processus enfant de recevoir des messages envoyer par le processus principal
	fdSocket est le descripteur de fichier correspondant à la socket du serveur
*/
void mainFork(int fdMain, int fdSocket);

#endif