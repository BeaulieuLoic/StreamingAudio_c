#include "audioserver.h"
#include "requete.h"

/* 
	arrèt du serveur proprement lorsque le signal SIGINT (ctrl + c) est reçu 
*/
void arretServeur(int sig){
	printf("Arret du serveur ...\n");
}


int main(int argc, char const *argv[]) {
	struct sigaction arretSig;
	arretSig.sa_handler = arretServeur;
	sigemptyset (&arretSig.sa_mask);
	arretSig.sa_flags = 0;
	sigaction(SIGINT, &arretSig, NULL);

	int erreur;
	char buffer[R_tailleMaxReq];
	requete *reqRecv = NULL;
	requete *reqSend = NULL;

	struct sockaddr_in client;
	socklen_t fromlen = sizeof(struct sockaddr_in);

	int fdSocket = initSocket();
	while(1){
		erreur = recvfrom(fdSocket,buffer,R_tailleMaxReq,0,
			(struct sockaddr*) &client,&fromlen);
		if(erreur < 0){
			perror("Erreur lors de la reception d'un message");
			freeReq(reqRecv);
			close(fdSocket);
			return 1;
		}

		freeReq(reqRecv);
		reqRecv = createRequeteFromBytes(buffer);

		switch(reqRecv->typeReq){
			case R_demandeCo:
				printf("demande de connexion ...\n");
				accepterConnexion(fdSocket,&client);
				break;
			case R_fermerCo:
				fermerConnexionClient(reqRecv->id);
				reqSend = createRequete(R_okFermerCo, R_idNull, 0, NULL);
				requeteToBytes(buffer, reqSend);
				erreur = sendto(fdSocket, buffer,sizeofReq(reqSend),0,
					(struct sockaddr*) &client, sizeof(struct sockaddr_in));
				freeReq(reqSend);
				break;
			case R_demanderFicher:
				printf("%s\n", reqRecv->data);
				break;

			default:
			  printf("Type de requete non traité : %d", reqRecv->typeReq);
			  break;
		}
	}
	freeReq(reqRecv);
	close(fdSocket);
	return 0;
}

void fermerConnexionClient(int idClientAEnlever){
	printf("fermeture de connexion id : %d ...\n", idClientAEnlever);
}


int accepterConnexion(int fdSocket,struct sockaddr_in *client){
	int erreur = 0;
	int idUnique = genererIdUnique();
	char buffer[R_tailleMaxReq];
	requete *reqSend;

	if (idUnique == 0){
		reqSend = createRequete(R_serverPlein, R_idNull, 0, NULL);
	}else{
		intToBytes(buffer, idUnique);
		reqSend = createRequete(R_okDemandeCo, R_idNull, sizeof(int), buffer);
	}
	requeteToBytes(buffer, reqSend);


	erreur = sendto(fdSocket, buffer,sizeofReq(reqSend),0,
		(struct sockaddr*) client, sizeof(struct sockaddr_in));
	if (erreur < 0){
		perror("Erreur lors de l'envois d'un message d'acceptation de connexion");
		freeReq(reqSend);
		return -1;
	}

	freeReq(reqSend);
	return 0;
}

/*

*/
int genererIdUnique(){
	return 999;
}

/*
	initialise la socket du serveur

	renvoi le descripteur de fichier correspondant à la socket du serveur
*/
int initSocket(){
	int fdSocket = socket(AF_INET, SOCK_DGRAM, 0);
	int erreur;
	if (fdSocket < 0){
		perror("Erreur lors de la création d'une socket");
		exit(1);
	}

	struct sockaddr_in serveur;
	serveur.sin_family = AF_INET;
	serveur.sin_port = htons(S_portServer);
	serveur.sin_addr.s_addr = htonl(INADDR_ANY);

	socklen_t addrlen = sizeof(struct sockaddr_in);

	erreur = bind(fdSocket, (struct sockaddr *)  &serveur,addrlen);
	if (erreur <0){
		perror("Erreur lors du bind");
		close(fdSocket);
		exit(1);
	}
	return fdSocket;
}

