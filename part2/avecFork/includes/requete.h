#ifndef _REQUETE_H_
#define _REQUETE_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* taille des différents attribut */
#define R_tailleMaxData 1024
#define R_tailleMaxReq (sizeof(int)*3+R_tailleMaxData)


/* Constante pour définir les types de requete */

/* 
	demande au serveur si le client peut se connecter et recevoir un identifiant 
	
	id non utile
	data non utile
*/
#define R_demandeCo 0

/*
	requete envoyer par le serveur pour confirmer que le client peut se connecter
	
	id non utile
	data doit être au moin de la taille d'un int et contient l'id attribué au client
*/
#define R_okDemandeCo 1000

#define R_okDemandeCoRecu 2000

/* 
	le client informe le serveur qu'il ferme la connection 

	id obligatoire
	data non utile
*/
#define R_fermerCo 2
/* 
	le serveur prévient le client qu'il à bien reçu 
	sa requète de fermeture de connexion

	id obligatoire
	data non utile
*/
#define R_okFermerCo 1002

/*
	demande au serveur de commencer à lire un fichier

	id obligatoire
	data obligatoire, contient le nom du fichier à ouvrir

*/
#define R_demanderFicherAudio 3

/*
	réponse du serveur si le fichier demander à été trouvé 

	id non utile
	data obligatoire, contient des informations sur le fichier son : fréquence, taille, mono ou stéréo 

*/
#define R_okDemanderFichierAudio 1003

/*
	réponse du serveur si le fichier demander n'à pas été trouvé ou n'est pas un fichier son

	id non utile
	data non utile
*/	
#define R_fichierAudioNonTrouver 1004

/*
	Le client demande au serveur de lui envoyer la suite du fichier

	id obligatoire
	data non utile
*/	
#define R_demandePartieSuivanteFichier 12
/*
	Le client demande au serveur de lui envoyer la partie du fichier sans lire la suite du fichier
	Utilisé si le serveur ne répond pas à R_demandePartieSuivanteFichier

	id obligatoire
	data non utile
*/
#define R_redemandePartieFichier 13

/*
	id non utile
	data obligatoire, contient une partie du fichier demandé
*/
#define R_okPartieSuivanteFichier 1012

/*
	Indique au client que le serveur à envoyer tout le fichier

	id non utile
	data non utile
*/
#define R_finFichier 1013

/*
	Requete qu'envoi le serveur pour prévenir le client qu'il n'y à plus de place

	id non utile
	data non utile
*/
#define R_serverPlein 10


/*
	Requete qu'envois le serveur si il reçois un id qui n'est pas définit

	id non utile
	data non utile
*/
#define R_idInexistant 11


/* 
	id à mettre lorsqu'il n'à pas été attribué ou n'est pas nécessaire 
*/
#define R_idNull -1



/* 
	type utilisé par le serveur et le client pour communiquer 
	typeReq : définit quel est le type de requete (ex demande de connection, envoi fichier son ...)
	Utilisé les constante pour l'initialisé 	

	id : identifiant d'un client permettant au serveur de distinguer les clients
		chaque id doit etre unique au niveau d'un serveur 
		si c'est un client sans id attribué ou le serveur son champ doit être égal à R_idNull
	
	tailleData : indique la taille du buffer data

	data : correspond au donnée qui seront envoyer tel que le fichier 
		son par le serveur au client.

	Les attribut du type requete ne sont pas à modifier,
	il faut passer par les fonctions ci dessous
*/

typedef struct req {
	int typeReq;
	int id;
	int tailleData;
	char *data;
} requete;

/*
	créer une requete en l'initialisans avec typeReq, id, tailleData 
	et le contenu de buf dans data.

	Si tailleData est supérieur à R_tailleMaxData, il est remis à R_tailleMaxData
*/
requete* createRequete(int typeReq, int id, unsigned int tailleData, char *buf);

/*
	créer une requete en l'initialisans en fonction d'une liste d'octet
	bytes est divisé en 4 partie :

		1er partie correspond au type de la requète. Codé sur 'sizeof(int)' octets
		
		2eme partie à l'éventuel identifiant du client. Codé sur 'sizeof(int)' octets

		3eme partie à la taille de la zone data qui se trouve en partie 4. Codé sur 'sizeof(int)' octets

		4eme partie qui sera copier dans la partie data de la requete.
		Cette 4eme à la longueur qui est indiqué dans la 3eme partie

	bytes doit avoir une taille égal ou supérieur à (sizeof(int)*3 + requete->tailleReq)
	Possibilité d'utilisé la constante R_tailleMaxReq pour être sur d'avoir aucun problème de débordement
*/
requete* createRequeteFromBytes(char *bytes);

/*
	convertie une requète en sa représentation en octet.
	dest doit avoir être égal ou supérieur à 
	sizeof(int)*3 + reqAConvertir->tailleReq

	Possibilité d'utilisé la constante R_tailleMaxReq pour définir 
	la taille de dest et être assuré qu'il possède une taille suffisante
*/
int requeteToBytes(char* dest, requete *reqAConvertir);

/*
	Libère la mémoire d'un objet requete
*/
void freeReq(requete* req);

/*
	Copie le contenu de buf dans data.
	buf doit au moin avoir une taille égal à requete->tailleReq
*/
int copyData(requete* req, char* buf);

/*
	Renvois la taille en octet que devra prendre la requète req
*/
int sizeofReq(requete* req);

/* 
	Converti les 'sizeof(int)' 1er octet de bytes en int
*/
int bytesToInt(char* bytes);

/* 
	Convertie un int en sa représentation en octet. 
	dest doit avoir une taille d'au moin sizeof(int) 
*/
void intToBytes(char* dest, int n);

#endif