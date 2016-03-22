#include "audioserver.h"

#include <time.h> /*random, à enlever !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

procFork listFork[AS_nbrMaxClient];

 
// Arret du serveur proprement lorsque le signal SIGINT (ctrl + c) est reçu 
// ou lorsqu'un processus enfant s'arrete
void arretServeur(int sig){
	int i;
	if (sig == SIGINT || sig == SIGCHLD){
		if (sig == SIGCHLD){
					fprintf(stdout, "\nUn processus enfant est mort :(\n");
		}


		fprintf(stdout, "Arret du serveur ...\n\n");
		for (i = 0; i < AS_nbrMaxClient; ++i){
			kill(listFork[i].pid, SIGKILL);
			close(listFork[i].fdProc);
		}
		exit(0);
	}
}


int main(int argc, char const *argv[]) {
	int i = 0;
	struct sockaddr_in addrServeur;
	struct sockaddr_in addrClient;
	int fdSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (fdSocket < 0){
		perror("Erreur lors de la création d'une socket");
		exit(1);
	}

	addrServeur.sin_family = AF_INET;
	addrServeur.sin_port = htons(AS_portServer);
	addrServeur.sin_addr.s_addr = htonl(INADDR_ANY);

	socklen_t addrlen = sizeof(struct sockaddr_in);
	if(bind(fdSocket, (struct sockaddr *)  &addrServeur,addrlen) < 0){
		perror("Erreur lors du bind du serveur");
		close(fdSocket);
		exit(2);
	}

	if(initListeFork(fdSocket)<0){
		exit(3);
	}


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
	
	char tmp[sizeof(int)];
	memset(tmp,0,sizeof(int));


	char adrClient[INET_ADDRSTRLEN];
	memset(adrClient,0,INET_ADDRSTRLEN);

	char buf[R_tailleMaxReq];
	memset(buf,0,R_tailleMaxReq);

	int tailleMsg = 0;
	
	int id = -1;

	char * port = NULL;

	requete req;

	while(1){

		socklen_t fromlen = sizeof(struct sockaddr_in);
		tailleMsg = recvfrom(fdSocket,buf,R_tailleMaxReq,0, 
				(struct sockaddr*) &(addrClient),&fromlen);
		if (tailleMsg < 0){
			perror("Erreur lors de la reception d'un message");
			exit(1);
		}

		id = bytesToInt(buf+sizeof(int));
		
		initRequeteFromBytes(&req,buf);


		switch(req.typeReq){

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
					
					inet_ntop(AF_INET, &(addrClient.sin_addr), adrClient, INET_ADDRSTRLEN);
					write(listFork[placeLibre].fdProc, adrClient, INET_ADDRSTRLEN);


					// Converti le num de port en bytes pour l'envoyer au processus qui va s'occuper du client
					port = (void *)(&addrClient.sin_port);
					write(listFork[placeLibre].fdProc, port,sizeof(unsigned short));

					// Envoi la taille de la requète au processus fils
					intToBytes(tmp, sizeofReq(req));
					write(listFork[placeLibre].fdProc, tmp, sizeof(int));

					
					requeteToBytes(buf,req);
					// Envois la requète du client au processus secondaire qui va la traité
					write(listFork[placeLibre].fdProc, buf,sizeofReq(req));
				}else{
					// Plus de place
					printf("Connexion refusé.\n");
					initRequete(&req, R_serverPlein, R_idNull, 0, NULL);
					requeteToBytes(buf, req);
					if(sendto(fdSocket, buf,sizeofReq(req),0,
						(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in)) < 0){
					}
					
				}
				placeLibre = -1;
				break;
			case R_fermerCo:
				if ((id >= 0 || id < AS_nbrMaxClient) && listFork[id].utiliser){
					/*
						Convertie le num de port en bytes pour l'envoyer au processus
					*/
					port = (void *)(&addrClient.sin_port);
					write(listFork[id].fdProc, port,sizeof(unsigned short));

					// Envoi la taille de la requète au processus fils
					intToBytes(tmp, sizeofReq(req));
					write(listFork[id].fdProc, tmp, sizeof(int));


					requeteToBytes(buf,req);
					write(listFork[id].fdProc, buf,sizeofReq(req));
					listFork[id].utiliser = 0;
				}else{
					// Erreur id non attribué
					printf("Erreur id non attribué :%d\n", id);
					initRequete(&req, R_idInexistant, R_idNull, 0, NULL);
					requeteToBytes(buf, req);
					if(sendto(fdSocket, buf,sizeofReq(req),0,
						(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in)) < 0){
					}
				}

				break;
			default:
				/*
					Accède à l'id du client qui se trouve forcément
					dans l'interval [sizeof(int), sizeof(int)[ de buf
				*/
				if ((id >= 0 || id < AS_nbrMaxClient) && listFork[id].utiliser){
					/*
						Convertie le num de port en bytes pour l'envoyer au processus
					*/
					port = (void *)(&addrClient.sin_port);
					write(listFork[id].fdProc, port,sizeof(unsigned short));

					// Envoi la taille de la requète au processus fils
					intToBytes(tmp, sizeofReq(req));
					write(listFork[id].fdProc, tmp, sizeof(int));


					requeteToBytes(buf,req);
					write(listFork[id].fdProc, buf,sizeofReq(req));
				}else{
					// Erreur id non attribué
					printf("Erreur id non attribué :%d\n", id);
					initRequete(&req, R_idInexistant, R_idNull, 0, NULL);
					requeteToBytes(buf, req);
					if(sendto(fdSocket, buf,sizeofReq(req),0,
						(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in)) < 0){
					}
				}
				break;
		}
	}
	perror("Erreur, while(true) arreté\n");
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

	requete req;

	char buf[R_tailleMaxReq];

	struct sockaddr_in addrClient;
	addrClient.sin_family = AF_INET;

	read(fdMain, buf, sizeof(int));
	int idFork = bytesToInt(buf);
	int tailleReq = 0;

	int blocFichierAct = -1;
	
	while (1){
		if (!occuper){
			/* 
				Récupère l'adresse client, données envoyer à l'origine
				par le processus principal
			*/
			if(read(fdMain, buf, INET_ADDRSTRLEN)<0){
				perror("Erreur proc enfant lors du read pour récupérer addrClient");
				exit(1);
			}
			inet_pton(AF_INET, buf, &(addrClient.sin_addr));
			
			blocFichierAct = -1;

		}

		// Récupère le numéros de port que le processus devra utilisé pour comuniquer avec le client
		if(read(fdMain, buf, sizeof(unsigned short))){
			perror("Erreur proc enfant lors du read pour récupérer port");
			exit(1);
		}
		
		unsigned short *port = (void *) buf;
		addrClient.sin_port = *port;


		if(read(fdMain, buf, sizeof(int)) < 0){
			perror("Erreur proc enfant lors du read pour récupérer tailleReq");
			exit(1);
		}	
		tailleReq = bytesToInt(buf);

		//Récupère la requète envoyer par le client, relayé via le processus principal
		read(fdMain, buf, tailleReq);


		// Convertie buf en requète
		initRequeteFromBytes(&req,buf);
		//printf("fork %d:typeReq : %d, id : %d , tailleData %d\n",idFork,req.typeReq,req.id,req.tailleData);
		switch(req.typeReq){

			// ################################################### Accepter connexion ###################################################
			case R_demandeCo:
				if (!occuper)
				{
					intToBytes(buf, idFork);
					// Envoi une confirmation au client que le serveur à bien reçus la demande de connexion
					initRequete(&req,R_okDemandeCo, R_idNull, sizeof(int), buf);
					requeteToBytes(buf,req);

					if(sendto(fdSocket, buf,sizeofReq(req),0,
							(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in))< 0){
						perror("Erreur proc enfant envoi R_okDemandeCo");
						exit(1);
					}
					occuper = 1;
				}else{
					// Erreur, le processus à reçus une demande de connexion alors qu'il s'occupe déja d'un autre client

				}		
				break;

			// ################################################### ferme la connexion d'un client ###################################################	
			case R_fermerCo:
				close(fdFichierAudio);
				occuper = 0;
				// Envoi d'une requète au client pour confirmer que le serveur à bien reçus la fermeture de connexion
				initRequete(&req,R_okFermerCo, R_idNull, 0, NULL);
				requeteToBytes(buf, req);
				if(sendto(fdSocket, buf,sizeofReq(req),0,
						(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in)) < 0){
					perror("Erreur proc enfant lors de l'envois du message fermerConnexionClient");
					exit(2);
				}

				break;

			// ################################################### Envoi info fichier ###################################################
			case R_demanderFicherAudio:
				// Vérifie que le fichier existe, qu'il s'agis bien d'un fichier audio et récupération les info du fichier audio
				err = aud_readinit(req.data, &rate, &size, &channels);
				fdFichierAudio = open(req.data,O_RDONLY);
				if (err < 0 || fdFichierAudio < 0){
					// Le fichier n'existe pas ou n'est pas un fichier audio
					initRequete(&req,R_fichierAudioNonTrouver, R_idNull,0, NULL);
					requeteToBytes(buf, req);

					if (sendto(fdSocket, buf,sizeofReq(req), 0, (struct sockaddr*) &(addrClient), 
								sizeof(struct sockaddr_in)) < 0){
						perror("Erreur proc enfant lors de l'envois du message fichierNonTrouver");
						exit(3);
					}
					printf("Fichier non trouvé\n");
				}else{
					// Insère dans le buf qui va être envoyer au client, la fréquence, si c'est sur 8 ou 16 bit et mono ou stéréo
					intToBytes(buf, rate);
					intToBytes(buf+sizeof(int), size);
					intToBytes(buf+sizeof(int)*2, channels);

					initRequete(&req, R_okDemanderFichierAudio, R_idNull,sizeof(int)*3, buf);
					requeteToBytes(buf, req);
					
					if (sendto(fdSocket, buf,sizeofReq(req), 0, (struct sockaddr*) &(addrClient), 
								sizeof(struct sockaddr_in)) < 0){
						perror("Erreur proc enfant lors de l'envois du message fichierTrouver");
						exit(3);
					}
					printf("Fichier trouvé\n");
				}
				break;
			// ################################################### Lecture partie suivante ###################################################
			case R_demandePartieSuivanteFichier:
				if (fdFichierAudio < 0){
					// Problème, le client demande le fichier sans avoir donnée d'abord le nom du fichier
					initRequete(&req, R_fichierAudioNonTrouver, R_idNull,0, NULL);
					requeteToBytes(buf, req);

					if (sendto(fdSocket, buf,sizeofReq(req), 0, (struct sockaddr*) &(addrClient), 
								sizeof(struct sockaddr_in)) < 0){
						perror("Erreur proc enfant lors de l'envois du message fichierNonTrouver");
						exit(3);
					}
				}else{
					// Si blocFichierAct == bytesToInt(req.data)
					// cela signifie que le client à déja envoyer un paquet mais qu'il à été perdu
					if (blocFichierAct != bytesToInt(req.data)){
						lectureWav = read(fdFichierAudio, buf, R_tailleMaxData);
						if (lectureWav < 0){
							perror("Erreur ");
						}
						blocFichierAct++;
					}
					if (lectureWav > 0){
						// Envoi d'une partie du fichier au client
						initRequete(&req, R_okPartieSuivanteFichier, R_idNull, lectureWav, buf);
						requeteToBytes(buf, req);

						if (sendto(fdSocket, buf,sizeofReq(req), 0, (struct sockaddr*) &(addrClient), 
									sizeof(struct sockaddr_in)) < 0){
							perror("Erreur proc enfant lors de l'envois du message R_okPartieSuivanteFichier");
							exit(3);
						}
					}else{
						// Le fichier à été entièrement lu
						initRequete(&req, R_finFichier, R_idNull, 0, NULL);
						requeteToBytes(buf, req);

						if (sendto(fdSocket, buf,sizeofReq(req), 0, (struct sockaddr*) &(addrClient), 
									sizeof(struct sockaddr_in)) < 0){
							perror("Erreur proc enfant lors de l'envois du message R_okPartieSuivanteFichier");
							exit(3);
						}
					}
				}
				break;

			default:
				printf("Type de requete non prévue : %d\n", req.typeReq);
				break;
		}
	}
	close(fdMain);
	close(fdSocket);

	perror("Erreur proc enfant, while(true) arreté\n");
	exit(0);
}
