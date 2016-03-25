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
	struct sigaction arretSig;
	arretSig.sa_handler = arretClient;
	sigemptyset (&arretSig.sa_mask);
	arretSig.sa_flags = 0;
	sigaction(SIGINT, &arretSig, NULL);

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
			Récupération d'un descripteur de fichier vers les haut-parleur
		*/
		speaker = aud_writeinit(rate, size, channels);
		if (speaker < 0){
			printf("Fermeture de connexion ...\n");
			fermerConnexion(&cl);
			exit(4);
		}


		printf("Lecture du fichier son ...\n");
		do{
			/* lecture fichier */
			partFichier++;
			lectureWav = partieSuivante(&cl, buf, partFichier);
			if (lectureWav == -1){
				/* fin fichier */
			}else if (lectureWav < -1){
				/* autre erreur */
				printf("Fermeture de connexion ...\n");
				fermerConnexion(&cl);
				exit(5);
			}else{
				if(write(speaker, buf, lectureWav)<0){
					perror("Erreur ecriture dans speaker");
					printf("Fermeture de connexion ...\n");
					fermerConnexion(&cl);
					exit(6);
				}
			}
		} while (lectureWav >=0);

		printf("Fermeture de connexion ...\n");
		fermerConnexion(&cl);
	}
	return 0;
}

