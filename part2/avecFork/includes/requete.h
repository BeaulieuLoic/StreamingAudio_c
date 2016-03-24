#ifndef _REQUETE_H_
#define _REQUETE_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Taille du buffer data */
#define R_tailleMaxData 1024

/* Taille maximum que peut avoir une requète */
#define R_tailleMaxReq (sizeof(int)*3+R_tailleMaxData)

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
	char data[R_tailleMaxData];
} requete;



/* Constante pour définir les types de requete */

/* 
	demande au serveur si le client peut se connecter et recevoir un identifiant 
	
	Envoyer par : le client
	id non utile
	data non utile

	Réponse atendu à cette requète :
		R_okDemandeCo
		R_serverPlein

*/
#define R_demandeCo 0


/*
	requete envoyer par le serveur pour confirmer que le client peut se connecter
	
	Envoyer par : le serveur
	id non utile
	data doit être au moin de la taille d'un int et contient l'id attribué au client

	Réponse atendu à cette requète :
		Aucune
*/
#define R_okDemandeCo 1000


/* 
	le client informe le serveur qu'il ferme la connection 

	Envoyer par : le client
	id obligatoire
	data non utile

	Réponse atendu à cette requète :
		R_okFermerCo
		R_idInexistant

*/
#define R_fermerCo 2


/* 
	le serveur prévient le client qu'il à bien reçu 
	sa requète de fermeture de connexion

	Envoyer par : le serveur
	id  non utile
	data non utile

	Réponse atendu à cette requète :
		Aucune
*/
#define R_okFermerCo 1002


/*
	demande au serveur de commencer à lire un fichier

	Envoyer par le client
	id obligatoire
	data obligatoire, contient le nom du fichier à ouvrir

	Réponse atendu à cette requète :
		R_okDemanderFichierAudio
		R_fichierAudioNonTrouver
		R_idInexistant

*/
#define R_demanderFicherAudio 3


/*
	réponse du serveur si le fichier demander à été trouvé 

	Envoyer par  le serveur
	id non utile
	data obligatoire, contient des informations sur le fichier son : fréquence, taille, mono ou stéréo 

	Réponse atendu à cette requète :
		Aucune

*/
#define R_okDemanderFichierAudio 1003


/*
	réponse du serveur si le fichier demander n'à pas été trouvé ou n'est pas un fichier son

	Envoyer par le serveur
	id non utile
	data non utile

	Réponse atendu à cette requète :
		Aucune
*/	
#define R_fichierAudioNonTrouver 1004


/*
	Le client demande au serveur de lui envoyer la suite du fichier
	La partie data doit contenir le numéros du bloc de fichier permetant ainsi de vérifié si il y à eu une perte de paquet lors de la transmission
	Si le serveur à le même numéros que celui envoyer par le serveur c'est qu'il y à eu une perte lors de l'envoi serveur->client.
	Si le serveur à le numéros-1 égal au client, tout c'est bien passer et le serveur envois la suite du fichier
	Les autre cas ne peut pas arrivé

	Envoyer par le client
	id obligatoire
	data obligatoire

	Réponse atendu à cette requète :
		R_okPartieSuivanteFichier
		R_finFichier
		R_fichierAudioNonTrouver
		R_idInexistant
*/	
#define R_demandePartieSuivanteFichier 12


/*
	Le serveur confirme qu'il à bien reçus la demande de lecture du fichier.

	Envoyer par le serveur
	id non utile
	data obligatoire, contient une partie du fichier demandé

	Réponse atendu à cette requète :
		Aucune
*/
#define R_okPartieSuivanteFichier 1012


/*
	Indique au client que le serveur à envoyer tout le fichier

	Envoyer par le serveur
	id non utile
	data non utile

	Réponse atendu à cette requète :
		Aucune
*/
#define R_finFichier 1013


/*
	Requete qu'envoi le serveur pour prévenir le client qu'il n'y à plus de place

	Envoyer par le serveur
	id non utile
	data non utile

	Réponse atendu à cette requète :
		Aucune
*/
#define R_serverPlein 10


/*
	Requete qu'envois le serveur si il reçois un id qui n'est pas définit

	Envoyer par le serveur
	id non utile
	data non utile

	Réponse atendu à cette requète :
		Aucune
*/
#define R_idInexistant 11


/* 
	Id à mettre lorsqu'il n'à pas été attribué ou n'est pas nécessaire 
*/
#define R_idNull -1



/*
	Initialise une requète avec typeReq, id, tailleData 
	et le contenu de buf dans data.

	Si tailleData est supérieur à R_tailleMaxData, il est remis à R_tailleMaxData
*/
void initRequete(requete *req, int typeReq, int id, unsigned int tailleData, char *buf);

/*
	Initialise une requète en fonction d'une liste d'octet
	bytes est divisé en 4 partie :

		1er partie correspond au type de la requète. Codé sur 'sizeof(int)' octets
		
		2eme partie à l'éventuel identifiant du client. Codé sur 'sizeof(int)' octets

		3eme partie à la taille de la zone data qui se trouve en partie 4. Codé sur 'sizeof(int)' octets

		4eme partie qui sera copier dans la partie data de la requete.
		Cette 4eme à la longueur qui est indiqué dans la 3eme partie

	bytes doit avoir une taille égal ou supérieur à sizeofReq(req)
	Possibilité d'utilisé la constante R_tailleMaxReq pour être sur d'avoir aucun problème de débordement
*/
void initRequeteFromBytes(requete *req, char *bytes);

/*
	Renvois la taille en octet que devra prendre la requète req
	soit : sizeof(int)*3 + req->tailleReq
*/
int sizeofReq(requete req);

/*
	Convertie une requète en sa représentation en octet.
	dest doit être égal ou supérieur à 
	sizeofReq(reqAConvertir)

	Possibilité d'utilisé la constante R_tailleMaxReq pour définir 
	la taille de dest et être assuré qu'il possède une taille suffisante
*/
int requeteToBytes(char* dest, requete reqAConvertir);

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