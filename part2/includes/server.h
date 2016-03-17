#ifndef _SERVER_H_
#define _SERVER_H_

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "requete.h"

#define S_nbrMaxClient 20

typedef struct serv{
	struct sockaddr_in addrClient;
	int fdSocket;
	requete *reqSend;
	requete *reqRecv;
	int listeIdClient[S_nbrMaxClient];
}server;


int initServer(server *serv, int portAUtilise);

void fermerServer(server *serv);

int accepterConnexion(server *serv);

int genererIdUnique();

void fermerConnexionClient(server *serv,int idClientAEnlever);

int attendreMsg(server *serv);

int fichierNonTrouver(server *serv);

int fichierTrouver(server *serv, int rate, int size, int channels);

int envoyerPartieFichier(server *serv, char *buf, int tailleBuf);

int finFichier(server *serv);

#endif