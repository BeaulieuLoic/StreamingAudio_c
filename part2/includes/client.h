#ifndef _CLIENT_H_
#define _CLIENT_H_

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

#include "requete.h"

typedef struct cl{
	struct sockaddr_in addrSend;
	struct sockaddr_in addrRecv;
	int fdSocket;
	int id;
	requete *reqSend;
	requete *reqRecv;	
}client;

/* timeout en microseconde */
#define C_timeoutDemandeCo 1000000 // = 1 seconde
#define C_timeoutFermerCo 1000000 // = 1 seconde
#define C_timeoutDemandeFichier 500000 // = 1 seconde



int initClient(client *cl, char * adrServ, int portServ);

int demandeDeConnexion(client *cl);

void fermerConnexion(client *cl);

void arreterConnexion(client *cl);

int demanderFichierAudio(client *cl, char *nomFichier, int *rate, int *size, int *channels);

int partieSuivante(client *cl, char *buf);

#endif