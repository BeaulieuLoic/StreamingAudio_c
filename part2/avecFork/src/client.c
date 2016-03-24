#include "client.h"


/*
	Définit un timeOut de tempEnMicros micro seconde sur le descripteur de fichier fd
*/
int definirTimeOut(int fd, int tempEnMicros){
	fd_set read_set;
	struct timeval timeout;
	FD_ZERO(&read_set);
	FD_SET(fd,&read_set);
	timeout.tv_sec = 0;
	timeout.tv_usec = tempEnMicros;

	return select(fd+1,&read_set, NULL,NULL,&timeout);
}


/*
	Initialise le client en ouvrant une socket sur le port fournit en paramètre ayant comme adresse ip 'adrServ'
*/
int initClient(client *cl, char * adrServ, int portServ){
	int fdSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (fdSocket < 0){
		perror("Erreur lors de la création d'une socket");
		return -1;
	}
	cl->addrSend.sin_family = AF_INET;
	cl->addrSend.sin_port = htons(portServ);
	cl->addrSend.sin_addr.s_addr = inet_addr(adrServ);

	cl->id = R_idNull;
	cl->fdSocket = fdSocket;
	return 0;
}


/*
	Envoi au serveur une demande de connexion via une requète 'R_demandeCo'
*/
int demandeDeConnexion(client *cl){
	int nbrtentativeReco = 0;
	int nbrMaxTentative = 3;
	int erreur;

	char buffer[R_tailleMaxReq];
	memset(buffer,0,R_tailleMaxReq);

	requete reqSend;
	requete reqRecv;
	

	// Initialise une requète de type R_demandeCo et la converti en octets
	initRequete(&reqSend,R_demandeCo, R_idNull, 0,NULL);
	requeteToBytes(buffer,reqSend); 

	/* 
		tentative de connexion au serveur.
		Si le serveur ne répond pas, réessaye un certain nombre de fois
	*/
	do{
		erreur = sendto(cl->fdSocket, buffer, sizeofReq(reqSend),
			0, (struct sockaddr*) &(cl->addrSend),  sizeof(struct sockaddr_in));
		if (erreur < 0){
			perror("Erreur lors de l'appel à demandeDeConnexion, sendto renvois une erreur");
			return -1;
		}

		erreur = definirTimeOut(cl->fdSocket, C_timeoutDemandeCo);
		if (erreur < 0){
			perror("Erreur lors de l'appel à demandeDeConnexion, problème lors du select");
			return -2;
		}else if(erreur == 0){
			nbrtentativeReco++;
			printf("le serveur ne répond pas, tentative n°%d sur %d\n", nbrtentativeReco, nbrMaxTentative);
		}else{/* si le serveur à reçu la demande de connexion */
			socklen_t fromlen = sizeof(struct sockaddr_in);

			erreur = recvfrom(cl->fdSocket,buffer,R_tailleMaxReq,
				0,(struct sockaddr*) &(cl->addrRecv),&fromlen);
			
			if (erreur < 0){
				perror("Erreur lors de l'appel à demandeDeConnexion, recvfrom renvois une erreur");
				return -3;
			}

			initRequeteFromBytes(&reqRecv, buffer);

			int id = 0;
			/* si le serveur répond ok à la demande de connexion, récupérer l'id 
				se trouvant dans la partie data de la requète reçu*/
			if (reqRecv.typeReq == R_okDemandeCo){
				id = bytesToInt(reqRecv.data);
				cl->id = id;
				printf("Connexion effectuée\n");

				return 0;
			}else if(reqRecv.typeReq == R_serverPlein){
				close(cl->fdSocket);
				printf("Impossible de ce connecter au serveur, le serveur est plein\n");
				return -4;
			}else{
				printf("Requète reçu non prévue lors de la demande de connexion : %d",reqRecv.typeReq);
				return -5;
			}
		}

	} while (nbrtentativeReco < nbrMaxTentative);
	printf("Echec de demande de connexion au serveur, aucune réponse\n");
	return -6;
}

/* 
	Appel la fonction arreterConnexion, ferme la socket et réinitialise divert champ de l'obj client
*/
void fermerConnexion(client *cl){
	arreterConnexion(cl);
	cl->id = 0;
	close(cl -> fdSocket);
}

/* 
	Envois d'une requète R_fermerCo au serveur
*/
void arreterConnexion(client *cl){
	int nbrtentativeDeco = 0;
	int nbrMaxTentative = 5;
	int erreur;
	char buffer[R_tailleMaxReq];
	memset(buffer,0,R_tailleMaxReq);

	requete reqSend;
	requete reqRecv;

	// Initialise une requète de type R_fermerCo et la converti en octets
	initRequete(&reqSend, R_fermerCo, cl->id, 0,NULL);
	requeteToBytes(buffer, reqSend);


	do{
		erreur = sendto(cl->fdSocket, buffer, sizeofReq(reqSend),
			0, (struct sockaddr*) &(cl->addrSend), sizeof(struct sockaddr_in));

		if (erreur < 0){
			perror("Erreur lors de l'appel à arreterConnexion, sendto renvois une erreur");
			return;
		}

		erreur = definirTimeOut(cl->fdSocket, C_timeoutFermerCo);

		if (erreur < 0){
			perror("Erreur lors de l'appel à arreterConnexion, problème lors du select");
			return;
		}else if(erreur == 0){
			nbrtentativeDeco++;
			printf("Le serveur ne répond pas, tentative n°%d\n", nbrtentativeDeco);
		}else{/* si le serveur à reçu la demande de connexion */
			socklen_t fromlen = sizeof(struct sockaddr_in);
			erreur = recvfrom(cl->fdSocket,buffer,R_tailleMaxReq,
				0,(struct sockaddr*) &(cl->addrRecv),&fromlen);
			if (erreur < 0){
				perror("Erreur lors de l'appel à arreterConnexion, recvfrom renvois une erreur");
				return;
			}

			// Initialise une requète avec les octets envoyer par le serveur
			initRequeteFromBytes(&reqRecv, buffer);
			if (reqRecv.typeReq == R_okFermerCo){
				printf("Fermeture effectuée\n");
			}else if(reqRecv.typeReq == R_idInexistant){
				printf("Le serveur ne reconnais pas l'id qui à été envoyer lors de la fermeture de connexion\n");
			}else{
				printf("La requète reçu en réponse à la fermeture de connexion n'est pas prévue: %d\n",reqRecv.typeReq);
			}
			return;
		}		
	} while (nbrtentativeDeco < nbrMaxTentative);
	printf("Echec de fermeture de connexion au serveur, aucune réponse\n");
}


/*
	Envoi une requète de type R_demanderFicherAudio au serveur, nomFichier correspond au nom du fichier 
	que le client souhaite avoir et sera stocké dans la partie data de la requète

	Les varaibles rate, size et channels seront modifier en fonction de se qu'envois le serveur
*/
int demanderFichierAudio(client *cl, char *nomFichier, int *rate, int *size, int *channels){
	int nbrtentativeDeco = 0;
	int nbrMaxTentative = 5;
	int erreur;

	char buffer[R_tailleMaxReq];
	memset(buffer,0,R_tailleMaxReq);

	requete reqSend;
	requete reqRecv;

	// Initialise une requète de type R_demanderFicherAudio et la converti en octets
	initRequete(&reqSend, R_demanderFicherAudio, cl->id, strlen(nomFichier)+1,nomFichier);
	requeteToBytes(buffer,reqSend);


	do{
		erreur = sendto(cl->fdSocket, buffer, sizeofReq(reqSend),
			0, (struct sockaddr*) &(cl->addrSend), sizeof(struct sockaddr_in));

		if (erreur < 0){
			perror("Erreur lors de l'appel à demanderFichierAudio, sendto renvois une erreur");
			return -1;
		}

		erreur = definirTimeOut(cl->fdSocket, C_timeoutDemandeFichier);

		if (erreur < 0){
			perror("Erreur lors de l'appel à demanderFichierAudio, problème lors du select");
			return -2;
		}else if(erreur == 0){
			nbrtentativeDeco++;
			printf("Le serveur ne répond pas, tentative n°%d\n", nbrtentativeDeco);
		}else{/* si le serveur à reçu la demande de connexion */
			socklen_t fromlen = sizeof(struct sockaddr_in);
			erreur = recvfrom(cl->fdSocket,buffer,R_tailleMaxReq,
				0,(struct sockaddr*) &(cl->addrRecv),&fromlen);
			if (erreur < 0){
				perror("Erreur lors de l'appel à demanderFichierAudio, recvfrom renvois une erreur");
				return -3;
			}

			// Initialise une requète avec les octets envoyer par le serveur
			initRequeteFromBytes(&reqRecv, buffer);
			if (reqRecv.typeReq == R_okDemanderFichierAudio){
				/* 
					Le serveur possède bien ce fichier

					La partie data  de la requète envoyer par le serveur est composé de 3 information :
						rate se trouve dans l'interval [0,sizeof(int)[
						size se trouve dans l'interval [sizeof(int),sizeof(int)*2[
						channels se trouve dans l'interval [sizeof(int)*2,sizeof(int)*3[
				*/				
				*rate = bytesToInt(reqRecv.data);
				*size = bytesToInt(reqRecv.data+sizeof(int));
				*channels = bytesToInt(reqRecv.data+sizeof(int)*2);
				return 0;
			}else if(reqRecv.typeReq == R_fichierAudioNonTrouver){
				printf("Le serveur ne possède pas ce fichier\n");
				return -6;
			}else if(reqRecv.typeReq == R_idInexistant){
				printf("Le serveur ne reconnais pas l'id qui à été envoyer lors de la demande d'info du fichier audio\n");
			}else{
				printf("La requète reçu en réponse à la demande de fichier n'est pas prévue: %d\n",reqRecv.typeReq);
				return -4;
			}
			
		}		
	} while (nbrtentativeDeco < nbrMaxTentative);
	printf("Echec de demande de fichier, aucune réponse\n");
	return -5;
}


/*
	Envoi d'une requète R_demandePartieSuivanteFichier au serveur.

	Ecris la partie du fichier dans buf
*/
int partieSuivante(client *cl, char *buf, int partFichier){
	int nbrTentative = 0;
	int nbrMaxTentative = 5;
	int erreur = 0;

	char buffer[R_tailleMaxReq];

	requete reqSend;
	requete reqRecv;

	intToBytes(buffer, partFichier);

	// Initialise une requète de type R_demandePartieSuivanteFichier et la converti en octets
	initRequete(&reqSend, R_demandePartieSuivanteFichier, cl->id, sizeof(int), buffer);
	requeteToBytes(buffer,reqSend);


	do{
		erreur = sendto(cl->fdSocket, buffer, sizeofReq(reqSend),
			0, (struct sockaddr*) &(cl->addrSend), sizeof(struct sockaddr_in));

		if (erreur < 0){
			perror("Erreur lors de l'appel à partieSuivante, sendto renvois une erreur");
			return -6;
		}

		erreur = definirTimeOut(cl->fdSocket, C_timeoutDemandeFichier);

		if (erreur < 0){
			perror("Erreur lors de l'appel à partieSuivante, problème lors du select");
			return -2;
		}else if(erreur == 0){
			nbrTentative++;
			initRequete(&reqSend, R_demandePartieSuivanteFichier, cl->id, sizeof(int), buffer);
			requeteToBytes(buffer,reqSend);
		}else{
			/* si le serveur à reçu la demande de connexion */
			socklen_t fromlen = sizeof(struct sockaddr_in);
			erreur = recvfrom(cl->fdSocket,buffer,R_tailleMaxReq,
				0,(struct sockaddr*) &(cl->addrRecv),&fromlen);
			if (erreur < 0){
				perror("Erreur lors de l'appel à partieSuivante, recvfrom renvois une erreur");
				return -3;
			}

			// Initialise une requète avec les octets envoyer par le serveur
			initRequeteFromBytes(&reqRecv, buffer);
			if (reqRecv.typeReq == R_okPartieSuivanteFichier){
				memcpy(buf, reqRecv.data, reqRecv.tailleData);
				return reqRecv.tailleData;
			}else if(reqRecv.typeReq == R_finFichier){
				printf("Fichier finit\n");
				return -1;
			}else if (reqRecv.typeReq == R_fichierAudioNonTrouver){
				printf("Erreur lors de partieSuivante, le serveur ne trouve pas le fichier\n");
				return -7;
			}else if(reqRecv.typeReq == R_idInexistant){
				printf("Le serveur ne reconnais pas l'id qui à été envoyer lors de la demande de fichier\n");
			}else{
				printf("La requète reçu en réponse à la demande de fichier n'est pas prévue: %d\n",reqRecv.typeReq);
				return -4;
			}
			
		}		
	} while (nbrTentative < nbrMaxTentative);
	printf("Echec de demande de fichier, aucune réponse\n");
	return -5;
}
