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
}client;

/* timeout en microseconde */
// Temps avant de redemander l'ouverture d'une connexion au client
#define C_timeoutDemandeCo 1000000 // = 1 seconde

// Temps d'attente avant de reprévenir que le client se déconecte du serveur
#define C_timeoutFermerCo 1000000 // = 1 seconde

// Temps avant laquel le client redemande le fichier
#define C_timeoutDemandeFichier 100000 // = 0,1 seconde



/*
	Définit un timeOut de tempEnMicros micro seconde sur le descripteur de fichier fd
*/
int definirTimeOut(int fd, int tempEnMicros);

/*
	Initialise le client en ouvrant une socket sur le port fournit en paramètre ayant comme adresse ip 'adrServ'
*/
int initClient(client *cl, char * adrServ, int portServ);

int demandeDeConnexion(client *cl);

/* 
	Appel la fonction arreterConnexion, ferme la socket et réinitialise divert champ de l'obj client
*/
void fermerConnexion(client *cl);

/* 
	Envois d'une requète R_fermerCo au serveur
*/
void arreterConnexion(client *cl);

/*
	Envoi une requète de type R_demanderFicherAudio au serveur, nomFichier correspond au nom du fichier 
	que le client souhaite avoir et sera stocké dans la partie data de la requète

	Les varaibles rate, size et channels seront modifier en fonction de se qu'envois le serveur
*/
int demanderFichierAudio(client *cl, char *nomFichier, int *rate, int *size, int *channels);

/*
	Envoi d'une requète R_demandePartieSuivanteFichier au serveur.

	Ecris la partie du fichier dans buf
*/
int partieSuivante(client *cl, char *buf, int partFichier);

#endif