#include "audioserver.h"


// Liste des processus enfant qui vont s'occuper des clients
// variable global pour permetre au handler de fermer la socket 
// et de tuer tout les processus enfant en cas de problème
//procFork listFork[AS_nbrMaxClient];
procFork *listFork = NULL;
int fdSocket = -1;


// Identifiant de la mémoire partagé
int idMemPartage = -1;

// Identifiant de la sémaphore et création du up() et down()
int idSemaphore = -1;
struct sembuf up = {0,1,0};
struct sembuf down = {0,-1,0};
 
// Arret du serveur proprement lorsque le signal SIGINT (ctrl + c) est reçu 
// ou lorsqu'un processus enfant s'arrete
void arretServeur(int sig){
	if (sig == SIGINT || sig == SIGCHLD){
		int i;
		pid_t pidEnfantMort = -1;
		if (sig == SIGCHLD){
			fprintf(stdout, "\nUn processus enfant est mort \n");
			// sauvegarde du pid de l'enfant mort
			pidEnfantMort = wait(NULL);
		}
		printf("Arret du serveur ...\n\n");

		for (i = 0; i < AS_nbrMaxClient; ++i){
			// On évite de tuer le processus qui vient de mourir
			if (listFork[i].pid != pidEnfantMort){
				kill(listFork[i].pid, SIGINT);
				wait(NULL);
				close(listFork[i].fdProc);
			}
		}

		if (fdSocket != -1){
			close(fdSocket);
		}
		// Détache la mémoire partager
		shmdt(listFork);
		// Supprime la mémoire partagé
		shmctl(idMemPartage, IPC_RMID, NULL);
		// Supprime la sémaphore
		semctl(idSemaphore,0, IPC_RMID);
		exit(0);
	}
}


int main(int argc, char const *argv[]) {
	// Initialisation des pid des futur processus à 1
	int i;

	// Initialisation du serveur
	struct sockaddr_in addrServeur;
	struct sockaddr_in addrClient;
	fdSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (fdSocket < 0){
		perror("Erreur lors de la création d'une socket");
		exit(1);
	}

	addrServeur.sin_family = AF_INET;
	addrServeur.sin_port = htons(AS_portServer); // Définit le numéros de port
	addrServeur.sin_addr.s_addr = htonl(INADDR_ANY);

	socklen_t addrlen = sizeof(struct sockaddr_in);
	if(bind(fdSocket, (struct sockaddr *)  &addrServeur,addrlen) < 0){
		perror("Erreur lors du bind du serveur");
		close(fdSocket);
		exit(2);
	}

	// Création de la mémoire partagé qui contiendra la liste des fork
	// Seul le processus et ces enfant on accès à cette mémoire.
	idMemPartage = shmget(IPC_PRIVATE, sizeof(procFork)*AS_nbrMaxClient, IPC_CREAT | 0600);
	if (idMemPartage < 0){
		perror("Erreur lors de la création de la mémoire partagé");
		exit(10);
	}


	// Attache la liste de fork avec la mémoire partager
	listFork = (procFork *) shmat(idMemPartage,0,0);
	if (listFork == NULL){
		perror("Erreur lors de l'attachement de listFork à la mémoire partagé");
		exit(11);
	}


	// Création de la sémaphore pour controler les écriture dans idMemPartage
	idSemaphore = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
	if (idSemaphore < 0){
		perror("Erreur lors de la création de la sémaphore");
		exit(12);
	}

	// Initialisation à down le sémaphore
	semop(idSemaphore,&up,1);
	if(initListeFork(fdSocket)<0){
		perror("Erreur lors de l'initialisation des processus enfants");
		arretServeur(SIGINT);
		exit(3);
	}

	/* Handler lorsque ctrl + c est effectuer */
	struct sigaction arretSigInt;
	sigemptyset (&arretSigInt.sa_mask);
	arretSigInt.sa_handler = arretServeur;
	arretSigInt.sa_flags = 0;
	sigaction(SIGINT, &arretSigInt, NULL);

	// Handler lorsqu'un enfant meurs
	struct sigaction arretSigCHLD;
	sigemptyset (&arretSigCHLD.sa_mask);
	arretSigCHLD.sa_handler = arretServeur;
	arretSigCHLD.sa_flags = 0;
	sigaction(SIGCHLD, &arretSigCHLD, NULL);
	

	
	
	// Buffer utilisé pour envoyer la taille de la requète au processus enfant qui va traité la requète
	char tmp[sizeof(int)];
	memset(tmp,0,sizeof(int));


	// Buffer utilisé pour envoyer l'adresse ip du client au processus enfant qui va traité la requète de ce client
	char adrClient[INET_ADDRSTRLEN];
	memset(adrClient,0,INET_ADDRSTRLEN);

	// Buffer utilisé pour stocké se qu'envois le client. Le client envois forcément une requète.
	char buf[R_tailleMaxReq];
	memset(buf,0,R_tailleMaxReq);

	// Variable utilisé pour définir l'id d'un nouveau client
	int placeLibre = R_idNull;

	char * port = NULL;

	requete req;
	socklen_t fromlen = sizeof(struct sockaddr_in);
	
	while(1){

		// Attent qu'un client lui envoi une requète
		
		if (recvfrom(fdSocket,buf,R_tailleMaxReq,0, 
			(struct sockaddr*) &(addrClient),&fromlen) < 0){
			perror("Erreur lors de la reception d'un message");
			arretServeur(SIGINT);
			exit(1);
		}

		// Convertie les octets reçu en requète
		initRequeteFromBytes(&req,buf);


		// down pour écrire dans a liste des fork
		semop(idSemaphore,&down,1);
		switch(req.typeReq){
			case R_demandeCo:
				printf("demande de connexion ...\n");
				// Cherche parmis la liste des processus si il y en à un qui n'est pas occupé par un autre client
				for (i = 0; i < AS_nbrMaxClient; ++i){
					if(!(listFork[i].utiliser)){
						placeLibre = i;
						break;
					}
				}


				// Si il y à une place, on envoi la requète vers le processus enfant qui n'est pas occupé
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
					listFork[placeLibre].utiliser = 1;
				}else{
					// Plus de place, envoi d'une requète R_serverPlein au client
					printf("Demande de connexion d'un client refusé.\n");
					initRequete(&req, R_serverPlein, R_idNull, 0, NULL);
					requeteToBytes(buf, req);
					if(sendto(fdSocket, buf,sizeofReq(req),0,
						(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in)) < 0){
						perror("Erreur lors de l'envois R_serverPlein");
						exit(1);
					}
				}
				placeLibre = -1;
				break;
			default:
				// L'id est correcte si elle ce trouve entre 0 et AS_nbrMaxClient-1 et que le processus en question est occupé par un client
				if ((req.id >= 0 || req.id < AS_nbrMaxClient) && listFork[req.id].utiliser){
					
					//Convertie le num de port en bytes pour l'envoyer au processus
					port = (void *)(&addrClient.sin_port);
					write(listFork[req.id].fdProc, port,sizeof(unsigned short));

					// Envoi la taille de la requète au processus fils
					intToBytes(tmp, sizeofReq(req));
					write(listFork[req.id].fdProc, tmp, sizeof(int));

					// Envoi la requète au processus fils
					requeteToBytes(buf,req);
					write(listFork[req.id].fdProc, buf,sizeofReq(req));
				}else{
					// Erreur id non attribué
					printf("Erreur id non attribué :%d\n", req.id);
					initRequete(&req, R_idInexistant, R_idNull, 0, NULL);
					requeteToBytes(buf, req);
					if(sendto(fdSocket, buf,sizeofReq(req),0,
						(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in)) < 0){
					}
				}
				break;
		}
		// up 
		semop(idSemaphore,&up,1);

	}
	// Impossible de passé par la
	perror("Erreur, while(true) arreté\n");
	arretServeur(SIGINT);
	return 0;
}



// initialise les processus enfant qui devront s'occuper des clients
int initListeFork(int fdSocket){
	// pid du future processus enfant
	pid_t pid = -1;
	// fdPipe sera le descripteur de fichier permetant au processus principal de communiqué avec les processus enfant
	int fdPipe[2], i = 0;
	// future id du processus enfant
	char futureId[sizeof(int)];
	for ( i = 0; i < AS_nbrMaxClient; ++i){
		if (pipe(fdPipe) < 0){
			// Erreur lors du pipe
			return -1;
		}
		
		pid = fork();
		if (pid != 0){
			close(fdPipe[0]);
			listFork[i].pid = pid;
			listFork[i].fdProc = fdPipe[1];
			listFork[i].utiliser = 0;


			//Envoi l'id du processus au processus enfant en question
			intToBytes(futureId, i);
			write(listFork[i].fdProc, futureId,sizeof(int));
		}else if (pid == 0){
			close(fdPipe[1]);
			mainFork(fdPipe[0], fdSocket);
		}else{
			// Erreur lors de la création de fork
			return -2;
		}
	}
	return 0;
}

/* 
	Fonction directement appeler dans un processus enfant.
	fdMain correspond au descripteur de fichier permetant au processus enfant de recevoir des messages envoyer par le processus principal
	fdSocket est le descripteur de fichier correspondant à la socket du serveur
*/
void mainFork(int fdMain, int fdSocket){
	int fdFichierAudio = -1, err = 0;
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

	int result = -1;
	
	while (1){
		if (!listFork[idFork].utiliser){
			
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

		// définit un timeout pour déconecter le client automatiquement au bout d'un certain temps sans recevoir de message
		fd_set read_set;
		struct timeval timeout;
		FD_ZERO(&read_set);
		FD_SET(fdMain,&read_set);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		result = select(fdMain+1,&read_set, NULL,NULL,&timeout);
		if (result < 0){
			perror("Erreur lors du select dans un processus enfant");
			exit(10);
		}else if(result == 0){ // le client n'à pas donnée signe de vie depuis 5 seconde, déconnection automatique
			if (fdFichierAudio !=-1){
				close(fdFichierAudio);
			}
			fdFichierAudio = -1;
			// Down de la mémoire partagé
			semop(idSemaphore,&down,1);
			listFork[idFork].utiliser = 0;
			// Up de la mémoire partagé
			semop(idSemaphore,&up,1);
			
			printf("Client %d deconnecté car il n'à pas comuniquer avec le serveur depuis plus de 5 seconde\n", idFork);
		}else{// tout se passe bien

			// Récupère le numéros de port que le processus devra utilisé pour comuniquer avec le client
			if(read(fdMain, buf, sizeof(unsigned short))<0){
				perror("Erreur proc enfant lors du read pour récupérer port");
				exit(1);
			}
			// Convertie les octets reçu en unsigned short 
			unsigned short tmp = *((unsigned short *) buf);
			addrClient.sin_port = *((unsigned short *) buf);


			// Récupération de la taille de la requète qui va etre envoyer
			if(read(fdMain, buf, sizeof(int)) < 0){
				perror("Erreur proc enfant lors du read pour récupérer tailleReq");
				exit(1);
			}	
			tailleReq = bytesToInt(buf);
			if (tailleReq>R_tailleMaxReq){
				printf("!!!!!!!!!!!!!!!!!%d!!!!!!!!!!!!!!!!!!!!!!\n",tmp);
			}

			//Récupère la requète envoyer par le client, relayé via le processus principal
			if(read(fdMain, buf, tailleReq) < 0){
				perror("Erreur proc enfant lors du read pour récupérer");
				exit(1);
			}


			// Convertie buf en requète
			initRequeteFromBytes(&req,buf);
		
			switch(req.typeReq){

				// ################################################### Accepter connexion ###################################################
				case R_demandeCo:
					intToBytes(buf, idFork);
					// Envoi une confirmation au client que le serveur à bien reçus la demande de connexion
					initRequete(&req,R_okDemandeCo, R_idNull, sizeof(int), buf);
					requeteToBytes(buf,req);

					if(sendto(fdSocket, buf,sizeofReq(req),0,
							(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in))< 0){
						perror("Erreur proc enfant envoi R_okDemandeCo");
						exit(1);
					}

					break;

				// ################################################### ferme la connexion d'un client ###################################################	
				case R_fermerCo:
					// down
					semop(idSemaphore,&down,1);

					close(fdFichierAudio);
					fdFichierAudio = -1;
					listFork[idFork].utiliser = 0;
					// Envoi d'une requète au client pour confirmer que le serveur à bien reçus la fermeture de connexion
					initRequete(&req,R_okFermerCo, R_idNull, 0, NULL);
					requeteToBytes(buf, req);
					if(sendto(fdSocket, buf,sizeofReq(req),0,
							(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in)) < 0){
						perror("Erreur proc enfant lors de l'envois du message fermerConnexionClient");
						exit(2);
					}

					printf("Client %d deconnecté\n", idFork);
					// up 
					semop(idSemaphore,&up,1);

					break;

				// ################################################### Envoi info fichier ###################################################
				case R_demanderFicherAudio:
					if (fdFichierAudio != -1){
						close(fdFichierAudio);
						fdFichierAudio = -1;
					}
					blocFichierAct = -1;

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
	}
	close(fdMain);

	perror("Erreur proc enfant, while(true) arreté\n");
	exit(0);
}
