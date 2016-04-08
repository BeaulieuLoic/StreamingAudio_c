/* Auteur : 
	Loic Beaulieu
	Maël PETIT
*/
#include "audioclient.h"
#include "client.h"
#include "audio.h"


client cl;
/* 
	arrèt du client proprement lorsque le signal SIGINT (ctrl + c) est reçu.
	prévient le serveur que le client se déconnecte
*/
void arretClient(int sig){
	if (sig == SIGINT){
		printf("Arret ...\n");
		printf("Fermeture de connexion ...\n");
		fermerConnexion(&cl);
		exit(0);
	}
}

/*
	argv[1] = adresse ip serveur
	argv[2] = nom du fichier audio
	argv[3] = éventuel filtre à appliquer
*/
int main(int argc, char const *argv[]) {
	int rate, size, channels;
	int speaker;
	int lectureWav = -1;
	char buf[R_tailleMaxData];

	int partFichier = -1;


	if (argc < 3){
		printf("nombre d'argument invalide.\n");
		exit(1);
	}
	char * adresseServ =  (char *) argv[1];
	char * fichierARecup = (char *) argv[2];



	/*
		Gestion des filtre audio
	*/

	int errStringToInt = 0;
	// modifie le volume avec -vol suivit d'un nombre positif
	// par défaut, le volume est à 100%
	int volumeAudioGauche = 100;
	int volumeAudioDroite = 100;

	// le filtre -gauche suivit d'un nombre va modifié le volume gauche si le son est en stéréo
	// de même avec -droite. Par défaut, le volume est à 100%

	// modifie la vitesse de lecture du son avec -vit suivis d'un nombre positif
	// par défaut la vitesse est à 100%
	float vitesse = 100;

	// -mono force à jouer la musique en mono
	int forcerMono = 0;

	// -echo permet d'ajouter de l'echo au fichier audio
	int avecEcho = 0;

	// le filtre -inv va inversé le coté gauche et droite si le son est en stéréo
	int inverse = 0;  

	int i = 0;
	for (i = 3; i < argc; ++i){
		if (strcmp(argv[i],"-mono") == 0){
			forcerMono = 1;
		}else if (strcmp(argv[i],"-echo") == 0){
			avecEcho = 1;
		}else if(strcmp(argv[i],"-vol") == 0){
			i++;
			if (i >= argc){
				printf("L'argument -vol doit être suivit d'un nombre entier positif\n");
			}else{
				volumeAudioGauche = stringToInt((char *)argv[i], &errStringToInt);
				volumeAudioDroite = volumeAudioGauche;
				if (errStringToInt || volumeAudioGauche < 0){
					printf("L'argument -vol doit être suivit d'un nombre entier positif\n");
					volumeAudioGauche = 100;
					volumeAudioDroite = 100;
				}
			}
		}else if (strcmp(argv[i],"-vit") == 0){
			i++;
			if (i >= argc){
				printf("L'argument -vit doit être suivit d'un nombre entier positif\n");
			}
			vitesse = stringToInt((char *)argv[i], &errStringToInt);
			if (errStringToInt || vitesse < 0){
				printf("L'argument -vit doit être suivit d'un nombre entier positif\n");
				vitesse = 100;
			}

		}else if (strcmp(argv[i],"-inv") == 0){
			inverse = 1;
		}else if (strcmp(argv[i],"-gauche") == 0){
			i++;
			if (i >= argc){
				printf("L'argument -gauche doit être suivit d'un nombre entier positif\n");
			}else{
				volumeAudioGauche = stringToInt((char *)argv[i], &errStringToInt);
				if (errStringToInt || volumeAudioGauche < 0){
					printf("L'argument -gauche doit être suivit d'un nombre entier positif\n");
					volumeAudioGauche = 100;
				}
			}
		}else if (strcmp(argv[i],"-droite") == 0){
			i++;
			if (i >= argc){
				printf("L'argument -droite doit être suivit d'un nombre entier positif\n");
			}else{
				volumeAudioDroite = stringToInt((char *)argv[i], &errStringToInt);
				if (errStringToInt || volumeAudioDroite < 0){
					printf("L'argument -droite doit être suivit d'un nombre entier positif\n");
					volumeAudioDroite = 100;
				}
			}
		}else{
			printf("\n /!\\ Warning  Le filtre %s n'existe pas /!\\\n\n", argv[i]);
		}
	}


	/* 
		initialisation du client: 
			ouverture d'une socket 
			initialisation des variable du client
	*/
	if(initClient(&cl, adresseServ, AC_portServ)< 0){
		exit(1);
	}

	/* 
		demande de connexion auprès du serveur :
			envois d'une requête R_demandeCo
	*/
	if(demandeDeConnexion(&cl) < 0){
		exit(2);
	}else{

		/* 
			demande au serveur s'il possède le fichier demander
		*/
		printf("Demande de fichier : %s ...\n",fichierARecup);
		if(demanderFichierAudio(&cl,fichierARecup, &rate, &size, &channels)<0){
			printf("Fermeture de connexion ...\n");
			fermerConnexion(&cl);
			exit(3);
		}

		/*
			Récupération d'un descripteur de fichier vers les haut-parleurs
			En fonction des informations envoyer par le serveur avec d'éventuel modif définit en argument du programme
		*/
		if (forcerMono){
			if (channels == 2){
				vitesse = vitesse * 2;
				channels = 1;
			}
		}

		rate = rate * (vitesse/100);

		// Supprime les arguments inutile lorsque le son est en mono
		if (channels == 1){
			inverse = 0;

			volumeAudioGauche = (volumeAudioGauche+volumeAudioDroite)/2;
			volumeAudioDroite = volumeAudioGauche;
		}
		// création du fork qui va créer un echo
		int fdForkEcho = -1;
		if (avecEcho){
			fdForkEcho = initFork(rate, size, channels);
			if (fdForkEcho < 0){
				perror("Erreur lors de la création du processus enfant pour l'echo");
				avecEcho = 0;
			}
		}

		// Initialisation du speaker
		speaker = aud_writeinit(rate, size, channels);
		if (speaker < 0){
			perror("Erreur création speaker. Fermeture de connexion ...");
			fermerConnexion(&cl);
			exit(4);
		}

		/*
			Définit sigaction lorsque le processus reçois un SIGINT (ctrl + c)
			pour arrèter correctement le client
		*/
		struct sigaction arretSig;
		arretSig.sa_handler = arretClient;
		sigemptyset (&arretSig.sa_mask);
		arretSig.sa_flags = 0;
		sigaction(SIGINT, &arretSig, NULL);


		// à envoyer au processus enfant pour l'echo, copie de buf avec volume modifié
		char bufEcho[R_tailleMaxData];


		printf("Lecture du fichier son ...\n");
		

		// Variable temporaire pour éventuellement inversé la gauche et la droite
		char tmp1;
		char tmp2;

		// Variable permetant de stocker l'entier représenté sur 8 ou 16 bits
		int repEntier = 0;
		// Volume courant, soit gauche, soit droite
		int volAct = 0;
		do{
			/* lecture fichier */
			partFichier++;
			lectureWav = partieSuivante(&cl, buf, partFichier);

			i = 0;
			volAct = 0;
			repEntier = 0;
			while(i < R_tailleMaxData){
				/* 
					sur 8 bits
					gauche -> indice pair du buffer
					droite -> indice impair du buffer

					sur 16 bits
					gauche -> indice 0,1, 4,5, 8,9 ... autrementdit à chaque fois que i%4==0 on modifie i et i+1
					droite -> indice 2,3, 6,7 10,11 ... on modifie i et i+1 lorsque i%4==2
				*/
				if (size == 16){// échantillion sur 16 bits
					// Inverse la gauche et la droite
					if (inverse && i%4==0){
						tmp1 = buf[i];
						tmp2 = buf[i+1];
						buf[i] = buf[i+2];
						buf[i+1] = buf[i+3];

						buf[i+2] = tmp1;
						buf[i+3] = tmp2;
					}
					
					if (i%4 == 0){
						volAct = volumeAudioGauche;				
					}else{
						volAct = volumeAudioDroite;						
					}

					// Récupère l'entier représenté sur 16 bits signé
					repEntier = 0;
					repEntier = (repEntier+buf[i+1]);
					repEntier = (repEntier << 8) | (0xFF&buf[i]);
					// modifie le volume
					repEntier = repEntier*volAct / 100;

					// correction au cas ou repEntier est supérieur à se que peut contenir 16 bits signé
					//(2^15)-1 == 32767 
					if (repEntier > 32767){
						repEntier = 32767;
					}else if(repEntier < -32767){
						repEntier = -32767;
					}

					buf[i] = (repEntier << 24)>>24;
					buf[i+1] = repEntier >> 8;

						
					// modifie le volume pour l'echo
					if (avecEcho){
						// Récupère l'entier représenté sur 16 bits signé
						// modifie le volume
						repEntier = repEntier*AC_volEcho/100;
						// corection au cas ou repEntier est supérieur à se que peut contenir 16 bits signé
						//(2^15)-1 == 32767 
						if (repEntier > 32767){
							repEntier = 32767;
						}else if(repEntier < -32767){
							repEntier = -32767;
						}


						bufEcho[i] = (repEntier << 24)>>24;
						bufEcho[i+1] = repEntier >> 8;
					}

					i += 2;
				}else{// échantillion sur 8 bits
					// Inverse la gauche et la droite
					if (inverse){
						tmp1 = buf[i];
						buf[i] = buf[i+1];
						buf[i+1] = tmp1;
					}

					if (i%2 == 0){
						volAct = volumeAudioGauche;
					}else{
						volAct = volumeAudioDroite;						
					}

					// correction au cas ou repEntier est supérieur à se que peut contenir 8 bits non signé
					//(2^8)-1 == 255 
					repEntier = buf[i] * volAct / 100;
					if (repEntier > 255){
						repEntier = 255;
					}
					buf[i] = repEntier;

					// modifie le volume pour l'echo
					if (avecEcho){
						repEntier = buf[i] * AC_volEcho / 100;
						if (repEntier > 255){
							repEntier = 255;
						}
						bufEcho[i] = repEntier;
					}
					i++;
				}
			}
			if (lectureWav == -1){
				/* fin fichier */
			}else if (lectureWav < -1){
				/* autre erreur */
				printf("Fermeture de connexion ...\n");
				fermerConnexion(&cl);
				close(speaker);
				exit(5);
			}else{
				if (avecEcho){
					//envois du buffer echo au processus enfant qui gère l'écho
					if(write(fdForkEcho, bufEcho, lectureWav)<0){
						perror("Erreur ecriture dans proc enfant pour l'echo");
						avecEcho = 0;
					}
				}
				// envois du buffer aux haut-parleurs
				if(write(speaker, buf, lectureWav)<0){
					perror("Erreur ecriture dans speaker");
					printf("Fermeture de connexion ...\n");
					fermerConnexion(&cl);
					exit(6);
				}
			}
		} while (lectureWav >=0);

		// attend la fin du processus echo
		if (avecEcho){
			printf("Attente fin echo ...\n");
			wait(NULL);
			close(fdForkEcho);
		}

		printf("Fermeture de connexion ...\n");
		fermerConnexion(&cl);
	}

	
	return 0;
}


/* 
	Initialise un processus enfant qui va s'occupé de l'echo
	renvois le descripteur de fichier permetant de lui envoyer le buffer réserver pour l'echo
*/
int initFork(int rate, int size, int channels){
	pid_t pid = -1;
	int err = 0;
	int fdPipe[2];

	err = pipe(fdPipe);
	if (err < 0){
		// Erreur lors du pipe
		return -1;
	}
	pid = fork();
	if (pid != 0){
		err = close(fdPipe[0]);
	if (err < 0){
		// Erreur lors du close
		return -1;
	}
		return fdPipe[1];
	}else if (pid == 0){
		err = close(fdPipe[1]);
		if (err < 0){
			// Erreur lors du close
			perror("Erreur lors du close du pipe[1] dans le processus echo");
			exit(0);
			return -1;
		}
		mainForkEcho(fdPipe[0],rate,size,channels);
		return 0;
	}else{
		// Erreur lors de la création de fork
		return -2;
	}
}



/* 
	fonction pour définir un usleep non bloquand dans le processus qui s'occupe de l'echo
*/
int decalageFait = 0;
void finDelaiEcho(int sig){
	if (sig == SIGCHLD){
		decalageFait = 1;
		wait(NULL);
	}
}

/* 
	fonction appelé pour le processus qui va géré l'echo
	fdMain est le descripteur de fichier permetant de recevoir le buffer de l'echo envoyer par le processus principal
*/
void mainForkEcho(int fdMain, int rate, int size, int channels){

	// Création du processus enfant qui va servir de sleep non bloquant
	pid_t pid = fork();
	if (pid < 0){
		perror("Erreur dans le proc enfant lors dela création du processus pour définir le délais");
		close(fdMain);
		exit(0);
	}else if (pid == 0){
		usleep(AC_decEcho*100000);
		exit(0);
	}

	// définit le handler, lorsque le processus enfant s'arrète après AC_decEcho micro seconde, finDelaiEcho est appelé
	struct sigaction finDelai;
	finDelai.sa_handler = finDelaiEcho;
	sigemptyset (&finDelai.sa_mask);
	finDelai.sa_flags = 0;
	sigaction(SIGCHLD, &finDelai, NULL);

	// calcule le nombre d'octet lu (approximatif) en 1 seconde
	int octetParSec = (rate * size * channels);

	// convertie le nb d'octet par seconde en fonction du délais de l'echo en dixième de seconde
	float nbMiniSec = AC_decEcho;
	int octetParXSec =  (((nbMiniSec/10 * octetParSec))/R_tailleMaxData)*R_tailleMaxData;

	// Le buffer est utilisé comme un tableau circulaire:
	// iDeb peut etre supérieur à iFin et inversement
	// iDeb et iFin sont forcément < octetParXSec
	// si iDeb == iFin soit la lecture vient tout juste de commencé, soit le fichier à été entièrement lu
	char * buf = malloc(octetParXSec);

	char bufTmp[R_tailleMaxData];

	int iDeb = 0;
	int iFin = 0;

	int iFinal = -1;

	// taille des données envoyées par le processus principal
	int recu = R_tailleMaxData;
	int recuLecAct = 0;
	
	// coupure indique l'endroit ou un packquet à du être coupé en 2 car le nb d'octet recu + iFin dépassais la taille du buffer
	// il faudra donc faire 2 write : 
	// 1 del'indice  de buf iFin jusqu'à octetParXSec
	// 1 autre de l'indice de buf 0 jusqu'à avoir lu le reste des données reçu
	int coupure = -1;

	int taillePacketCoupe = 0;

	// récupération du descirpteur de fichier pour les haut-parleurs
	int speaker = aud_writeinit(rate, size,channels);
	if (speaker <0){
		perror("Erreur lors de la création d'un speaker dans proc echo");
		close(fdMain);
		free(buf);
		exit(0);
	}

	// on continue à lire ce qu'envois le proc principal tant que recu == R_tailleMaxData
	// on continue à écrire dans le speaker tant que iDeb != iFin et recu == R_tailleMaxData
	while (iDeb != iFin || recu == R_tailleMaxData){
		if (!decalageFait){

			// si le décalage n'est pas encore fait et que recu != R_tailleMaxData
			// cela signifie que le fichier audio à été lu entièrement avant que l'echo commence
			if (recu == R_tailleMaxData){
				if (iFin + R_tailleMaxData > octetParXSec){
					printf("Problème dans proc echo, le buffer n'est pas suffisament grand pour contenir toute les données du décalage\n");
					iFin = 0;
				}

				recu = read(fdMain, buf+iFin, R_tailleMaxData);

				// read interrompu par le handler qui détecte la mort du proc enfant qui sert de usleep non bloquand
				if (recu == -1 && decalageFait){
					recu = read(fdMain, buf+iFin, R_tailleMaxData);
					if (recu < 0){
						perror("Erreur lors du read lorsque que le décalage vient tout juste de ce finir.");
						break;
					}
					if (iFin + recu > octetParXSec){
						printf("Problème dans proc echo, le buffer n'est pas suffisament grand pour contenir toute les données du décalage\n");
					}
					iFin = (iFin + recu)%octetParXSec;
				}else if ( recu < -1){
					perror("Erreur lors du read avant que le décalage soit finit");
					break;
				}else{
					if (iFin + recu > octetParXSec){
						printf("Problème dans proc echo, le buffer n'est pas suffisament grand pour contenir toute les données du décalage\n");
					}
					iFin = (iFin + recu)%octetParXSec;
				}
			}
		}else{// décalage finit
			// si recu est différent de R_tailleMaxData, recu possède le nombre d'octet du dernier packet du fichier, le fichier à donc entièrement été récupérer
			if (recu == R_tailleMaxData){
				recu = read(fdMain, bufTmp, R_tailleMaxData);
				if (recu < 0){
					perror("Erreur dans proc echo lors du read");
					break;
				}

				// coupure à faire
				if (iFin + recu > octetParXSec){
					taillePacketCoupe = octetParXSec - iFin ;
					memcpy(buf+iFin, bufTmp, taillePacketCoupe);
					memcpy(buf, bufTmp, recu-taillePacketCoupe);
					coupure = iFin;
				}else{
					memcpy(buf+iFin, bufTmp, recu);
				}
				iFin = (iFin + recu)%octetParXSec;
			}else{
				iFinal = iFin;
			}

						
			//regarde si iDeb se trouve au niveau du dernier packet du fichier
			if(iDeb+recu == iFinal){
				recuLecAct = recu;
			}else{
				recuLecAct = R_tailleMaxData;
			}

			// coupure à cette endroit
			if (coupure == iDeb){
				taillePacketCoupe = octetParXSec - iDeb ;

				//lecture jusqu'à la fin du buffer
				if(write(speaker,buf+iDeb, taillePacketCoupe)< 0){
					perror("Erreur lors du write dans le speaker du proc echo");
					break;
				}

				//lecture depuis le début du buffer jusqu'à la fin du packet du fichier
				if(write(speaker,buf, recuLecAct - taillePacketCoupe) <0 ){
					perror("Erreur lors du write dans le speaker du proc echo");
					break;
				}
				coupure = -1;
			}else{
				if(write(speaker,buf+iDeb, recuLecAct) < 0){
					perror("Erreur lors du write dans le speaker du proc echo");
					break;
				}
			}
			iDeb = (iDeb + recuLecAct)%octetParXSec;				
		}
	}

	close(speaker);
	close(fdMain);
	free(buf);
	exit(0);
}


/*
	Convertie une chaine de caractère composé de chiffre.
	Si ce n'est pas un nombre dans str (ex : "12ab", "a", "a12", ...), erreur est mis à 1
*/
int stringToInt(char * str, int *erreur){
	int aRetourner = 0;
	
	*erreur = 0;
	int nbChiffre = strlen(str);
	if (str == NULL || nbChiffre < 0){
		*erreur = 1;
		return aRetourner;
	}

	int tmp = 0;
	int estNegatif = str[0] == '-';
	if (estNegatif){
		nbChiffre -= 1;
	}


	int i = 0;
	while(str[i] != '\0' && !(*erreur)){
		if (estNegatif){
			tmp = puissance(10,nbChiffre);
		}else{
			tmp = puissance(10,nbChiffre-1);
		}
		switch(str[i]){
			case '0':
				aRetourner += 0*tmp;
				break;
			case '1':
				aRetourner += 1*tmp;
				break;
			case '2':
				aRetourner += 2*tmp;
				break;
			case '3':
				aRetourner += 3*tmp;
				break;
			case '4':
				aRetourner += 4*tmp;
				break;
			case '5':
				aRetourner += 5*tmp;
				break;
			case '6':
				aRetourner += 6*tmp;
				break;
			case '7':
				aRetourner += 7*tmp;
				break;
			case '8':
				aRetourner += 8*tmp;
				break;
			case '9':
				aRetourner += 9*tmp;
				break;
			case '-':
				if (i != 0){
					*erreur = 1;
				}
				break;
			default:
				*erreur = 1;
				break;
		}
		nbChiffre--;
		i++;
	}
	if (i == 0){
		//la chaine de caractère est vide si i = 0 alors que l'on se trouve ici
		*erreur = 1;
	}

	if (str[0] == '-'){
		aRetourner = -aRetourner;
	}
	return aRetourner;
}

/*
	renvois a puissance b 
*/
int puissance(int a, int b){
	int aRetourner = 1;
	int i = 0;
	for (i = 0; i < b; ++i){
		aRetourner *= a;
	}
	return aRetourner;
}