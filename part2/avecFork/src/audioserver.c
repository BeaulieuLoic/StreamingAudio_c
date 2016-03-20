#include "audioserver.h"
#include "server.h"
#include "audio.h"

#include <time.h> /*random, à enlever !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

procFork listFork[AS_nbrMaxClient];
server serveur;

 
// Arret du serveur proprement lorsque le signal SIGINT (ctrl + c) est reçu 
// ou lorsqu'un processus enfant s'arrete
void arretServeur(int sig){
	int i;
	if (sig == SIGINT || sig == SIGCHLD){
		if (sig == SIGCHLD){
					fprintf(stdout, "\nUn processus enfant est mort\n");
		}


		fprintf(stdout, "Arret du serveur ...\n\n");
		for (i = 0; i < AS_nbrMaxClient; ++i){
			kill(listFork[i].pid, SIGKILL);
		}
		exit(0);
	}
}


int main(int argc, char const *argv[]) {
	initServer(&serveur,AS_portServer);
	initListeFork(serveur.fdSocket);


	/* Handler lorsque ctrl + c est effectuer */
	struct sigaction arretSig;
	arretSig.sa_handler = arretServeur;
	sigemptyset (&arretSig.sa_mask);
	arretSig.sa_flags = 0;
	sigaction(SIGINT, &arretSig, NULL);

	// Handler lorsqu'un enfant meurs
	struct sigaction arretSigCHLD;
	arretSigCHLD.sa_handler = arretServeur;
	sigemptyset (&arretSigCHLD.sa_mask);
	arretSigCHLD.sa_flags = 0;
	sigaction(SIGCHLD, &arretSigCHLD, NULL);

	int placeLibre = -1;
	int typeReq = 0;
	char buf[R_tailleMaxReq];
	char tmp[sizeof(int)];
	int tailleMsg = 0;
	int i = 0;
	int id = -1;
	char * port = NULL;

	while(1){
		socklen_t fromlen = sizeof(struct sockaddr_in);
		tailleMsg = recvfrom(serveur.fdSocket,buf,R_tailleMaxReq,0, 
				(struct sockaddr*) &(serveur.addrClient),&fromlen);
		if (tailleMsg < 0){
			perror("Erreur lors de la reception d'un message");
			exit(1);
		}

		/*
			Récupération du type de la requète se trouvant forcément
			entre [0, sizeof(int)[ de buf
		*/
		typeReq = bytesToInt(buf);
		switch(typeReq){

			case R_demandeCo:
				printf("demande de connexion ...\n");
				// Cherche parmis la liste des processus si il y en à un qui n'est pas attribué à un client
				for (i = 0; i < AS_nbrMaxClient; ++i){
					if(!(listFork[i].utiliser)){
						placeLibre = i;
						listFork[i].utiliser = 1;
						break;
					}
				}
				if (placeLibre != -1){
					printf("Connexion accepter, id : %d \n",placeLibre);

					// Converti l'adresse ip du client en bytes pour l'envoyer au processus qui va s'occuper du client
					char adrClient[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &(serveur.addrClient.sin_addr), adrClient, INET_ADDRSTRLEN);
					write(listFork[placeLibre].fdProc, adrClient,INET_ADDRSTRLEN);


					// Converti le num de port en bytes pour l'envoyer au processus qui va s'occuper du client
					port = (void *)(&serveur.addrClient.sin_port);
					write(listFork[placeLibre].fdProc, port,sizeof(unsigned short));

					// Envoi la taille de la requète au processus fils
					intToBytes(tmp, tailleMsg);
					write(listFork[placeLibre].fdProc, tmp, sizeof(int));

					
					// Envois la requète du client au processus secondaire qui va la traité
					write(listFork[placeLibre].fdProc, buf,tailleMsg);
				}else{
					// Plus de place
					printf("Connexion refusé.\n");
				}

				break;
			case R_fermerCo:
				if ((id >= 0 || id < AS_nbrMaxClient) && listFork[i].utiliser){
					/*
						Convertie le num de port en bytes pour l'envoyer au processus
					*/
					port = (void *)(&serveur.addrClient.sin_port);
					write(listFork[placeLibre].fdProc, port,sizeof(unsigned short));

					// Envoi la taille de la requète au processus fils
					intToBytes(tmp, tailleMsg);
					write(listFork[placeLibre].fdProc, tmp, sizeof(int));


					write(listFork[id].fdProc, buf,tailleMsg);
					listFork[id].utiliser = 0;
				}else{
					// Erreur id
				}

				break;
			default:
				/*
					Accède à l'id du client qui se trouve forcément
					dans l'interval [sizeof(int), sizeof(int)[ de buf
				*/
				id = bytesToInt(buf+sizeof(int));

				if ((id >= 0 || id < AS_nbrMaxClient) && listFork[i].utiliser){
					/*
						Convertie le num de port en bytes pour l'envoyer au processus
					*/
					port = (void *)(&serveur.addrClient.sin_port);
					write(listFork[placeLibre].fdProc, port,sizeof(unsigned short));

					// Envoi la taille de la requète au processus fils
					intToBytes(tmp, tailleMsg);
					write(listFork[placeLibre].fdProc, tmp, sizeof(int));


					write(listFork[id].fdProc, buf,tailleMsg);
				}else{
					// Erreur id
				}
				break;
		}
	}
	return 0;
}


int initListeFork(int fdSocket){
	pid_t pid = 1, i;
	int fdPipe[2];
	char futureId[sizeof(int)];
	for ( i = 0; i < AS_nbrMaxClient; ++i){
		pipe(fdPipe);
		pid = fork();
		if (pid != 0){
			close(fdPipe[0]);
			listFork[i].pid = pid;
			listFork[i].fdProc = fdPipe[1];
			listFork[i].utiliser = 0;


			//Envoi l'id du processus au processus en question
			intToBytes(futureId, i);
			write(listFork[i].fdProc, futureId,sizeof(int));
		}else if (pid == 0){
			close(fdPipe[1]);
			mainFork(fdPipe[0], fdSocket);
		}else{
			printf("Erreur lors de la création de fork\n");
			return -1;
		}
	}
	return 0;
}

void mainFork(int fdMain, int fdSocket){
	int occuper = 0, fdFichierAudio = -1, err = 0;
	int rate, size, channels;
	int lectureWav = -1;

	requete *req = NULL;

	char buf[R_tailleMaxReq];

	struct sockaddr_in addrClient;
	addrClient.sin_family = AF_INET;

	read(fdMain, buf, sizeof(int));
	int idFork = bytesToInt(buf);
	int tailleReq = 0;
	
	while (1){
		if (!occuper){
			/* 
				Récupère l'adresse client, données envoyer à l'origine
				par le processus principal pour indiquer l'adresse du client
			*/
			read(fdMain, buf, INET_ADDRSTRLEN);
			inet_pton(AF_INET, buf, &(addrClient.sin_addr));
			
			occuper = 1;
		}

		// Récupère le numéros de port que le processus devra utilisé pour comuniquer avec le client
		read(fdMain, buf, sizeof(unsigned short));
		unsigned short *port = (void *) buf;
		addrClient.sin_port = *port;

		read(fdMain, buf, sizeof(int));
		tailleReq = bytesToInt(buf);

		//Récupère la requète envoyer par le client, relayé via le processus principal
		read(fdMain, buf, tailleReq);
		

		// Convertie buf en requète
		freeReq(req);
		req = createRequeteFromBytes(buf);
		switch(req->typeReq){

			// ################################################### Accepter connexion ###################################################
			case R_demandeCo:			
				freeReq(req);
				intToBytes(buf, idFork);
				// Envoi une confirmation au client que le serveur à bien reçus la demande de connexion
				req = createRequete(R_okDemandeCo, R_idNull, sizeof(int), buf);
				requeteToBytes(buf,req);
				if(sendto(fdSocket, buf,sizeofReq(req),0,
						(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in))< 0){
					perror("Erreur envoi R_okDemandeCo");
					exit(1);
				}
				break;

			// ################################################### ferme la connexion d'un client ###################################################	
			case R_fermerCo:
				close(fdFichierAudio);
				occuper = 0;
				// Envoi d'une requète au client pour confirmer que le serveur à bien reçus la fermeture de connexion
				req = createRequete(R_okFermerCo, R_idNull, 0, NULL);
				requeteToBytes(buf, req);
				if(sendto(fdSocket, buf,sizeofReq(req),0,
						(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in)) < 0){
					perror("Erreur lors de l'envois du message fermerConnexionClient");
					exit(2);
				}

				break;

			// ################################################### Envoi info fichier ###################################################
			case R_demanderFicherAudio:
				// Vérifie que le fichier existe, qu'il s'agis bien d'un fichier audio et récupération les info du fichier audio
				err = aud_readinit(req->data, &rate, &size, &channels);
				fdFichierAudio = open(req->data,O_RDONLY);
				freeReq(req);
				if (err < 0 || fdFichierAudio < 0){
					// Le fichier n'existe pas ou n'est pas un fichier audio
					req = createRequete(R_fichierAudioNonTrouver, R_idNull,0, NULL);
					requeteToBytes(buf, req);

					if (sendto(fdSocket, buf,sizeofReq(req), 0, (struct sockaddr*) &(addrClient), 
								sizeof(struct sockaddr_in)) < 0){
							perror("Erreur lors de l'envois du message fichierNonTrouver");
							exit(3);
					}
					printf("Fichier non trouvé\n");
				}else{
					// Insère dans le buf qui va être envoyer au client, la fréquence, si c'est sur 8 ou 16 bit et mono ou stéréo
					intToBytes(buf, rate);
					intToBytes(buf+sizeof(int), size);
					intToBytes(buf+sizeof(int)*2, channels);

					req = createRequete(R_okDemanderFichierAudio, R_idNull,sizeof(int)*3, buf);
					requeteToBytes(buf, req);
					
					if (sendto(fdSocket, buf,sizeofReq(req), 0, (struct sockaddr*) &(addrClient), 
								sizeof(struct sockaddr_in)) < 0){
							perror("Erreur lors de l'envois du message fichierTrouver");
							exit(3);
					}
					printf("Fichier trouvé\n");
				}
				break;
			// ################################################### Lecture partie suivante ###################################################
			case R_demandePartieSuivanteFichier:
				freeReq(req);
				if (fdFichierAudio < 0){
					// Problème, le client demande le fichier sans avoir donnée d'abord le nom du fichier
					req = createRequete(R_fichierAudioNonTrouver, R_idNull,0, NULL);
					requeteToBytes(buf, req);

					if (sendto(fdSocket, buf,sizeofReq(req), 0, (struct sockaddr*) &(addrClient), 
								sizeof(struct sockaddr_in)) < 0){
							perror("Erreur lors de l'envois du message fichierNonTrouver");
							exit(3);
					}
				}else{
					lectureWav = read(fdFichierAudio, buf, R_tailleMaxData);
					if (lectureWav > 0){
						// Envoi d'une partie du fichier au client
						req = createRequete(R_okPartieSuivanteFichier, R_idNull, lectureWav, buf);
						requeteToBytes(buf, req);

						if (sendto(fdSocket, buf,sizeofReq(req), 0, (struct sockaddr*) &(addrClient), 
									sizeof(struct sockaddr_in)) < 0){
								perror("Erreur lors de l'envois du message R_okPartieSuivanteFichier");
								exit(3);
						}
					}else{
						// Le fichier à été entièrement lu
						req = createRequete(R_finFichier, R_idNull, 0, NULL);
						requeteToBytes(buf, req);

						if (sendto(fdSocket, buf,sizeofReq(req), 0, (struct sockaddr*) &(addrClient), 
									sizeof(struct sockaddr_in)) < 0){
								perror("Erreur lors de l'envois du message R_okPartieSuivanteFichier");
								exit(3);
						}
					}
				}
				break;

			default:
				printf("Type de requete non prévue : %d\n", req->typeReq);
				break;
		}
	}
	freeReq(req);
	close(fdMain);
	close(fdSocket);
	exit(0);
}
