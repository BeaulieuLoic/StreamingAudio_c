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
		perror("Erreur lors de la reception d'un message");
		return -1;
	}

	freeReq(serv->reqRecv);
	serv->reqRecv = createRequeteFromBytes(buffer);


	return serv->reqRecv->typeReq;
}


int accepterConnexion(server *serv){
	int erreur = 0;
	int idUnique = genererIdUnique();
	char buffer[R_tailleMaxReq];
	freeReq(serv->reqSend);

	if (idUnique == 0){
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
	return 0;
}

int genererIdUnique(){
	return 999;
}

void fermerConnexionClient(server *serv,int idClientAEnlever){
	printf("fermeture de connexion id : %d ...\n", idClientAEnlever);
}