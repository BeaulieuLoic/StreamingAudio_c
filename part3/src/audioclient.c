/* Auteur : 
	Loic Beaulieu
	Maël PETIT
*/
#include "audioclient.h"
#include "client.h"
#include "audio.h"


client cl;
pid_t pidEcho = -1;
/* 
	arrèt du client proprement lorsque le signal SIGINT (ctrl + c) est reçu.
	prévient le serveur que le client se déconnecte
*/
void arretClient(int sig){
	if (sig == SIGINT){
		printf("Arret ...\n");
		printf("Fermeture de connexion ...\n");
		fermerConnexion(&cl);
		if (pidEcho != -1){
			kill(pidEcho, SIGKILL);
		}
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
			if (i > argc){
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
			if (i > argc){
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
			if (i > argc){
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
			if (i > argc){
				printf("L'argument -droite doit être suivit d'un nombre entier positif\n");
			}else{
				volumeAudioDroite = stringToInt((char *)argv[i], &errStringToInt);
				if (errStringToInt || volumeAudioDroite < 0){
					printf("L'argument -droite doit être suivit d'un nombre entier positif\n");
					volumeAudioDroite = 100;
				}
			}
		}else{
			printf("Le filtre %s n'existe pas \n", argv[i]);
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
			pour arrèter correctement le serveur
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
		do{
			/* lecture fichier */
			partFichier++;
			lectureWav = partieSuivante(&cl, buf, partFichier);

			// si il y a 2 channel on modifie le son de la gauche et la droite
			if (channels == 2){
				/* 
					sur 8 bits
					gauche -> indice pair du buffer
					droite -> indice impair du buffer

					sur 16 bits
					gauche -> indice 0,1, 4,5, 8,9 ...
					droite -> indice 2,3, 6,7 10,11 ... 

				*/
				i = 0;
				int volAct = 0;
				while(i < R_tailleMaxData){
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
						
						if (i%4 == 0)
							volAct = volumeAudioGauche;
						else
							volAct = volumeAudioDroite;

						// Récupère l'entier représenté sur 16 bits signé
						int repEntier = 0;
						repEntier = (repEntier+buf[i+1]);
						repEntier = (repEntier << 8) | (0xFF&buf[i]);

						// modifie le volume
						repEntier = repEntier*volAct / 100;

						// corection au cas ou repEntier est supérieur à se que peut contenir 16 bits signé
						//(2^15)-1 == 32767 
						if (repEntier > 32767){
							repEntier = 32767;
						}else if(repEntier < -32767){
							repEntier = -32767;
						}


						buf[i] = (repEntier << 24)>>24;
						buf[i+1] = repEntier >> 8;

						
						// baisse du volume pour l'echo
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

						if (i%2 == 0)
							volAct = volumeAudioGauche;
						else
							volAct = volumeAudioDroite;

						buf[i] = buf[i] * volAct / 100;


						// baisse du volume pour l'echo
						if (avecEcho){
							bufEcho[i] = buf[i] * AC_volEcho/100;
						}
						i++;
					}
				}

			}else{// size = 8
				// modifie le volume, si channel = 1, le volume du coté gauche est égal au coté droite
				for (i = 0; i < R_tailleMaxData; ++i){
					buf[i] = buf[i] * volumeAudioGauche/100;
					if (avecEcho){
						bufEcho[i] = buf[i] * AC_volEcho/100;
					}
				}
			}


			if (lectureWav == -1){
				/* fin fichier */
			}else if (lectureWav < -1){
				/* autre erreur */
				printf("Fermeture de connexion ...\n");
				fermerConnexion(&cl);
				exit(5);
			}else{
				if (avecEcho){
					if(write(fdForkEcho, bufEcho, lectureWav)<0){
						perror("Erreur ecriture dans proc enfant pour l'echo");
						avecEcho = 0;
					}
				}

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
			wait(NULL);
			close(fdForkEcho);
		}


		printf("Fermeture de connexion ...\n");
		fermerConnexion(&cl);
	}
	return 0;
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
		// Erreur lors du pipe
		return -1;
	}
		pidEcho = pid;
		return fdPipe[1];
	}else if (pid == 0){
		err = close(fdPipe[1]);
		if (err < 0){
			// Erreur lors du pipe
			return -1;
		}
		mainFork(fdPipe[0],rate,size,channels);
		return 0;
	}else{
		// Erreur lors de la création de fork
		return -2;
	}
}

void mainFork(int fdMain, int rate, int size, int channels){
	int speaker = aud_writeinit(rate, size,channels);
	if (speaker <0){
		perror("Erreur lors de la création d'un speaker dans proc echo");
		exit(0);
	}

	// calcule le nombre d'octet lu par les haut parleur en 1 seconde
	int octetParSec = rate * (size/8);


	int decalageFait = 0;
	int recu = R_tailleMaxData;
	
	int nbRecu = 0;
	printf("%d\n", octetParSec);
	char * buf = malloc(octetParSec*100);


	while (recu == R_tailleMaxData){


		if (!decalageFait){
			int i;
			for (i = 0; i < AC_decEcho/1000; ++i){
				if (recu == R_tailleMaxData){
					recu = read(fdMain, buf+nbRecu, R_tailleMaxData);
					if (recu < 0){
						perror("Erreur dans proc echo lors du read");
						break;
					}
					nbRecu = nbRecu + recu;	
				}
				usleep(AC_decEcho/1000);
			}
			decalageFait = 1;
		}else{
			recu = read(fdMain, buf, R_tailleMaxData);
			if (recu < 0){
				perror("Erreur dans proc echo lors du read");
				break;
			}
			nbRecu = recu;
		}
		if(write(speaker,buf, nbRecu) < 0){
			perror("Erreur lors du write dans le speaker du proc echo");
			break;
		}
	}

	close(speaker);
	close(fdMain);
	exit(0);
}