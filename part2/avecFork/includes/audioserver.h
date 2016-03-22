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
#define AS_nbrMaxClient 2

typedef struct proc
{
	pid_t pid;
	int fdProc;
	int utiliser;
} procFork;

void arretServeur(int sig);

int initListeFork(int fdSocket);

void mainFork(int fdMain, int fdSocket);

#endif