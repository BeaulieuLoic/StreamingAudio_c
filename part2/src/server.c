#include "server.h"

/*
	initialise la socket du serveur

	renvoi le descripteur de fichier correspondant à la socket du serveur
*/
int initServer(server *serv, int portAUtilise){
	serv->reqSend = NULL;
	serv->reqRecv = NULL;

	int fdSocket = socket(AF_INET, SOCK_DGRAM, 0);
	int erreur;
	if (fdSocket < 0){
		perror("Erreur lors de la création d'une socket");
		return -1;
	}

	struct sockaddr_in serveur;
	serveur.sin_family = AF_INET;
	serveur.sin_port = htons(portAUtilise);
	serveur.sin_addr.s_addr = htonl(INADDR_ANY);

	socklen_t addrlen = sizeof(struct sockaddr_in);

	erreur = bind(fdSocket, (struct sockaddr *)  &serveur,addrlen);
	if (erreur <0){
		perror("Erreur lors du bind du serveur");
		close(fdSocket);
		return -2;
	}
	serv->fdSocket = fdSocket;
	return 0;
}

void fermerServer(server *serv){
	freeReq(serv-> reqRecv);
	freeReq(serv-> reqSend);
	close(serv->fdSocket);
}

int attendreMsg(server *serv){
	socklen_t fromlen = sizeof(struct sockaddr_in);
	int erreur;
	char buffer[R_tailleMaxReq];

	erreur = recvfrom(serv->fdSocket,buffer,R_tailleMaxReq,0,
			(struct sockaddr*) &(serv->addrClient),&fromlen);
	if(erreur < 0){
		return erreur;
	}

	freeReq(serv->reqRecv);
	serv->reqRecv = createRequeteFromBytes(buffer);


	return serv->reqRecv->typeReq;
}


int accepterConnexion(server *serv, int idUnique){
	int erreur = 0;
	char buffer[R_tailleMaxReq];
	freeReq(serv->reqSend);

	if (idUnique <= 0){
		serv->reqSend = createRequete(R_serverPlein, R_idNull, 0, NULL);
	}else{
		intToBytes(buffer, idUnique);
		serv->reqSend = createRequete(R_okDemandeCo, R_idNull, sizeof(int), buffer);
	}
	requeteToBytes(buffer, serv->reqSend);


	erreur = sendto(serv->fdSocket, buffer,sizeofReq(serv->reqSend),0,
		(struct sockaddr*) &(serv->addrClient), sizeof(struct sockaddr_in));
	if (erreur < 0){
		perror("Erreur lors de l'envois d'un message d'acceptation de connexion");
		return -1;
	}

	if (serv->reqSend->typeReq == R_serverPlein){
		return -2;
	}
	return 0;
}


void fermerConnexionClient(server *serv,int idClientAEnlever){
	int erreur = 0;
	char buffer[R_tailleMaxReq];
	freeReq(serv->reqSend);
	serv->reqSend = createRequete(R_okFermerCo, R_idNull, 0, NULL);

	requeteToBytes(buffer, serv->reqSend);

	erreur = sendto(serv->fdSocket, buffer,sizeofReq(serv->reqSend),0,
		(struct sockaddr*) &(serv->addrClient), sizeof(struct sockaddr_in));
	if (erreur < 0){
		printf("Erreur lors de l'envois du message fermerConnexionClient");
	}
}


int fichierNonTrouver(server *serv){
	int erreur = 0;
	char buffer[R_tailleMaxReq];
	freeReq(serv->reqSend);
	serv->reqSend = createRequete(R_fichierAudioNonTrouver, R_idNull,0, NULL);

	requeteToBytes(buffer, serv->reqSend);


	erreur = sendto(serv->fdSocket, buffer,sizeofReq(serv->reqSend),0,
		(struct sockaddr*) &(serv->addrClient), sizeof(struct sockaddr_in));
	if (erreur < 0){
		perror("Erreur lors de l'envois du message fichierNonTrouver");
		return -1;
	}
	return 0;
}

int fichierTrouver(server *serv, int rate, int size, int channels){
	int erreur = 0;
	char buffer[R_tailleMaxReq];
	freeReq(serv->reqSend);

	intToBytes(buffer, rate);
	intToBytes(buffer+sizeof(int), size);
	intToBytes(buffer+sizeof(int)*2, channels);
	serv->reqSend = createRequete(R_okDemanderFichierAudio, R_idNull,sizeof(int)*3, buffer);

	requeteToBytes(buffer, serv->reqSend);


	erreur = sendto(serv->fdSocket, buffer,sizeofReq(serv->reqSend),0,
		(struct sockaddr*) &(serv->addrClient), sizeof(struct sockaddr_in));
	if (erreur < 0){
		perror("Erreur lors de l'envois du message fichierTrouver");
		return -1;
	}
	return 0;
}

int envoyerPartieFichier(server *serv, char *buf, int tailleBuf){
	int erreur = 0;
	char buffer[R_tailleMaxReq];
	freeReq(serv->reqSend);
	serv->reqSend = createRequete(R_okPartieSuivanteFichier, R_idNull, tailleBuf, buf);

	requeteToBytes(buffer, serv->reqSend);


	erreur = sendto(serv->fdSocket, buffer,sizeofReq(serv->reqSend),0,
		(struct sockaddr*) &(serv->addrClient), sizeof(struct sockaddr_in));
	if (erreur < 0){
		perror("Erreur lors de l'envois du message envoyerPartieFichier");
		return -1;
	}
	return 0;
}

int finFichier(server *serv){
	int erreur = 0;
	char buffer[R_tailleMaxReq];
	freeReq(serv->reqSend);
	serv->reqSend = createRequete(R_finFichier, R_idNull,0, NULL);

	requeteToBytes(buffer, serv->reqSend);

	erreur = sendto(serv->fdSocket, buffer,sizeofReq(serv->reqSend),0,
		(struct sockaddr*) &(serv->addrClient), sizeof(struct sockaddr_in));
	if (erreur < 0){
		perror("Erreur lors de l'envois du message finFichier");
		return -1;
	}
	return 0;
}

int definirTimeOut(int fd, int tempEnMicros){
	fd_set read_set;
	struct timeval timeout;
	FD_ZERO(&read_set);
	FD_SET(fd,&read_set);
	timeout.tv_sec = 0;
	timeout.tv_usec = tempEnMicros;

	return select(fd+1,&read_set, NULL,NULL,&timeout);
}