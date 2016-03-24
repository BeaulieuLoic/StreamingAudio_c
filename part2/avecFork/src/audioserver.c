
#include "audioserver.h"

// Liste des processus enfant qui vont s'occuper des clients
procFork listFork[AS_nbrMaxClient];
int fdSocket = -1;
 
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
			if (listFork[i].pid != -1){
				kill(listFork[i].pid, SIGKILL);
				close(listFork[i].fdProc);
			}
		}
		if (fdSocket != -1){
			close(fdSocket);
		}
		exit(0);
	}
}


int main(int argc, char const *argv[]) {

	// Initialisation des pid des futur processus à 1
	int i;
	for (i = 0; i < AS_nbrMaxClient; ++i){
		listFork[i].pid = -1;
	}

	// Initialisation du serveur
	struct sockaddr_in addrServeur;
	struct sockaddr_in addrClient;
	fdSocket = socket(AF_INET, SOCK_DGRAM, 0);
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
		perror("Erreur lors de l'initialisation des processus enfants");
		arretServeur(SIGINT);
		exit(3);
	}


	/* Handler lorsque ctrl + c est effectuer */
	struct sigaction arretSigInt;
	arretSigInt.sa_handler = arretServeur;
	sigemptyset (&arretSigInt.sa_mask);
	arretSigInt.sa_flags = 0;
	sigaction(SIGINT, &arretSigInt, NULL);

	// Handler lorsqu'un enfant meurs
	struct sigaction arretSigCHLD;
	arretSigCHLD.sa_handler = arretServeur;
	sigemptyset (&arretSigCHLD.sa_mask);
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

		switch(req.typeReq){
			case R_demandeCo:
				printf("demande de connexion ...\n");
				// Cherche parmis la liste des processus si il y en à un qui n'est pas occupé par un autre client
				for (i = 0; i < AS_nbrMaxClient; ++i){
					if(!(listFork[i].utiliser)){
						placeLibre = i;
						listFork[i].utiliser = 1;
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
				}else{
					// Plus de place, envoi d'une requète R_serverPlein au client
					printf("Demande de connexion d'un client refusé.\n");
					initRequete(&req, R_serverPlein, R_idNull, 0, NULL);
					requeteToBytes(buf, req);
					if(sendto(fdSocket, buf,sizeofReq(req),0,
						(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in)) < 0){
					}
					
				}
				placeLibre = -1;
				break;
			case R_fermerCo:
				// L'id est correcte si elle ce trouve dans l'inertaval [0,AS_nbrMaxClient[ et que le processus en question est occupé par un client
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
					// Le client se déconecte, le processus enfant n'est donc plus occupé
					listFork[req.id].utiliser = 0;
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
		if(read(fdMain, buf, sizeof(unsigned short))<0){
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
		
	
		switch(req.typeReq){

			// ################################################### Accepter connexion ###################################################
			case R_demandeCo:
				if (!occuper){
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
					initRequete(&req, R_serverPlein, R_idNull, 0, NULL);
					requeteToBytes(buf,req);

					if(sendto(fdSocket, buf,sizeofReq(req),0,
							(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in))< 0){
						perror("Erreur proc enfant envoi R_serverPlein");
						exit(1);
					}
				}		
				break;

			// ################################################### ferme la connexion d'un client ###################################################	
			case R_fermerCo:
				close(fdFichierAudio);
				fdFichierAudio = -1;
				occuper = 0;
				// Envoi d'une requète au client pour confirmer que le serveur à bien reçus la fermeture de connexion
				initRequete(&req,R_okFermerCo, R_idNull, 0, NULL);
				requeteToBytes(buf, req);
				if(sendto(fdSocket, buf,sizeofReq(req),0,
						(struct sockaddr*) &(addrClient), sizeof(struct sockaddr_in)) < 0){
					perror("Erreur proc enfant lors de l'envois du message fermerConnexionClient");
					exit(2);
				}

				printf("Client %d deconnecté\n", idFork);

				break;

			// ################################################### Envoi info fichier ###################################################
			case R_demanderFicherAudio:
				if (fdFichierAudio != -1){
					close(fdFichierAudio);
					fdFichierAudio = -1;
				}

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
