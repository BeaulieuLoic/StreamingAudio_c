#include "client.h"

int definirTimeOut(int fd, int tempEnMicros){
	fd_set read_set;
	struct timeval timeout;
	FD_ZERO(&read_set);
	FD_SET(fd,&read_set);
	timeout.tv_sec = 0;
	timeout.tv_usec = tempEnMicros;

	return select(fd+1,&read_set, NULL,NULL,&timeout);
}

int initClient(client *cl, char * adrServ, int portServ){
	int fdSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (fdSocket < 0){
		perror("Erreur lors de la création d'une socket");
		return -1;
	}
	cl->addrSend.sin_family = AF_INET;
	cl->addrSend.sin_port = htons(portServ);
	cl->addrSend.sin_addr.s_addr = inet_addr(adrServ);

	cl->reqSend = NULL;
	cl->reqRecv = NULL;
	cl->id = R_idNull;
	cl->fdSocket = fdSocket;
	return 0;
}


int demandeDeConnexion(client *cl){
	printf("Ouverture d'une connexion avec le serveur ... \n");
	int nbrtentativeReco = 0;
	int nbrMaxTentative = 3;
	int erreur;
	char buffer[R_tailleMaxReq];

	freeReq(cl -> reqSend);
	
	cl -> reqSend = createRequete(R_demandeCo, R_idNull, 0,NULL);
	requeteToBytes(buffer,cl->reqSend); 

	/* 
		tentative de connexion au serveur.
		Si le serveur ne répond pas, réessaye un certain nombre de fois
	*/
	do{
		erreur = sendto(cl->fdSocket, buffer, sizeofReq(cl->reqSend),
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
			printf("le serveur ne répond pas, tentative n°%d\n", nbrtentativeReco);
		}else{/* si le serveur à reçu la demande de connexion */
			socklen_t fromlen = sizeof(struct sockaddr_in);

			erreur = recvfrom(cl->fdSocket,buffer,R_tailleMaxReq,
				0,(struct sockaddr*) &(cl->addrRecv),&fromlen);
			
			if (erreur < 0){
				perror("Erreur lors de l'appel à demandeDeConnexion, recvfrom renvois une erreur");
				return -3;
			}

			freeReq(cl -> reqRecv);
			cl -> reqRecv = createRequeteFromBytes(buffer);

			int id = 0;
			/* si le serveur répond ok à la demande de connexion, récupérer l'id 
				se trouvant dans la partie data de la requète reçu*/
			if (cl->reqRecv->typeReq == R_okDemandeCo){
				id = bytesToInt(cl->reqRecv->data);
				cl->id = id;
				printf("Connexion effectuée\n");
				return 0;
			}else if(cl->reqRecv->typeReq == R_serverPlein){
				freeReq(cl -> reqRecv);
				freeReq(cl -> reqSend);
				close(cl->fdSocket);
				printf("Impossible de ce connecter au serveur, le serveur est plein\n");
				return -4;
			}else{
				printf("Requète reçu non prévue lors de la demande de connexion : %d",cl->reqRecv->typeReq);
				return -5;
			}
		}

	} while (nbrtentativeReco < nbrMaxTentative);
	printf("Echec de demande de connexion au serveur, aucune réponse\n");
	return -6;
}


void fermerConnexion(client *cl){
	arreterConnexion(cl);
	cl->id = 0;
	freeReq(cl-> reqRecv);
	freeReq(cl-> reqSend);
	close(cl -> fdSocket);
}

void arreterConnexion(client *cl){
	printf("Fermeture de connexion ...\n");
	int nbrtentativeDeco = 0;
	int nbrMaxTentative = 5;
	int erreur;
	char buffer[R_tailleMaxReq];

	freeReq(cl->reqSend);
	cl->reqSend = createRequete(R_fermerCo, cl->id, 0,NULL);
	requeteToBytes(buffer,cl->reqSend);


	do{
		erreur = sendto(cl->fdSocket, buffer, sizeofReq(cl->reqSend),
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

			freeReq(cl->reqRecv);
			cl->reqRecv = createRequeteFromBytes(buffer);
			if (cl->reqRecv->typeReq != R_okFermerCo){
				printf("La requète reçu en réponse à la fermeture de connexion n'est pas prévue: %d\n",cl->reqRecv->typeReq);
			}else{
				printf("Fermeture effectuée\n");
			}
			return;
		}		
	} while (nbrtentativeDeco < nbrMaxTentative);
	printf("Echec de fermeture de connexion au serveur, aucune réponse\n");
}

int demanderFichierAudio(client *cl, char *nomFichier, int *rate, int *size, int *channels){
	printf("Demande de fichier : %s ...\n",nomFichier);
	int nbrtentativeDeco = 0;
	int nbrMaxTentative = 5;
	int erreur;
	char buffer[R_tailleMaxReq];

	freeReq(cl->reqSend);
	cl->reqSend = createRequete(R_demanderFicherAudio, cl->id, strlen(nomFichier)+1,nomFichier);
	requeteToBytes(buffer,cl->reqSend);


	do{
		erreur = sendto(cl->fdSocket, buffer, sizeofReq(cl->reqSend),
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

			freeReq(cl->reqRecv);
			cl->reqRecv = createRequeteFromBytes(buffer);
			if (cl->reqRecv->typeReq == R_okDemanderFichierAudio){
				/* 
					Le serveur possède bien ce fichier 
					Demande d'information sur le fichier son
				*/

				*rate = bytesToInt(cl->reqRecv->data);
				*size = bytesToInt(cl->reqRecv->data+sizeof(int));
				*channels = bytesToInt(cl->reqRecv->data+sizeof(int)*2);
				return 0;
			}else if(cl->reqRecv->typeReq == R_fichierAudioNonTrouver){
				printf("Le serveur ne possède pas ce fichier\n");
				return -6;
			}else{
				printf("La requète reçu en réponse à la demande de fichier n'est pas prévue: %d\n",cl->reqRecv->typeReq);
				return -4;
			}
			
		}		
	} while (nbrtentativeDeco < nbrMaxTentative);
	printf("Echec de demande de fichier, aucune réponse\n");
	return -5;
}

int partieSuivante(client *cl, char *buf){
	srand(time(NULL));



	int nbrTentative = 0;
	int nbrMaxTentative = 5;
	int erreur = 0;
	char buffer[R_tailleMaxReq];

	freeReq(cl->reqSend);
	cl->reqSend = createRequete(R_demandePartieSuivanteFichier, cl->id, 0, NULL);
	requeteToBytes(buffer,cl->reqSend);


	do{
		erreur = sendto(cl->fdSocket, buffer, sizeofReq(cl->reqSend),
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
			freeReq(cl->reqSend);
			cl->reqSend = createRequete(R_redemandePartieFichier, cl->id, 0, NULL);
			requeteToBytes(buffer,cl->reqSend);
			//printf("Le serveur ne répond pas, tentative n°%d\n", nbrTentative);
		}else{/* si le serveur à reçu la demande de connexion */
			socklen_t fromlen = sizeof(struct sockaddr_in);
			erreur = recvfrom(cl->fdSocket,buffer,R_tailleMaxReq,
				0,(struct sockaddr*) &(cl->addrRecv),&fromlen);
			if (erreur < 0){
				perror("Erreur lors de l'appel à partieSuivante, recvfrom renvois une erreur");
				return -3;
			}

			freeReq(cl->reqRecv);
			cl->reqRecv = createRequeteFromBytes(buffer);
			if (cl->reqRecv->typeReq == R_okPartieSuivanteFichier){
				memcpy(buf, cl->reqRecv->data, cl->reqRecv->tailleData);
				return cl->reqRecv->tailleData;
			}else if(cl->reqRecv->typeReq == R_finFichier){
				printf("Fichier finit\n");
				return -1;
			}else if (cl->reqRecv->typeReq == R_fichierAudioNonTrouver){
				printf("Erreur lors de partieSuivante, le serveur ne trouve pas le fichier\n");
				return -7;
			}else{
				printf("La requète reçu en réponse à la demande de fichier n'est pas prévue: %d\n",cl->reqRecv->typeReq);
				return -4;
			}
			
		}		
	} while (nbrTentative < nbrMaxTentative);
	printf("Echec de demande de fichier, aucune réponse\n");
	return -5;
}
