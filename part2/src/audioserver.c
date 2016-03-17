#include "audioserver.h"
#include "server.h"
#include "audio.h"

#include <time.h>

server serveur;

/* 
	arret du serveur proprement lorsque le signal SIGINT (ctrl + c) est reçu 
*/
void arretServeur(int sig){
	char *msgArret = "\nArret du serveur ...\n";
	if (sig == SIGINT){
		fprintf(stdout, "%s\n", msgArret);
	}
}


int main(int argc, char const *argv[]) {
	/* test paquet perdu */
	srand (time(NULL));


	/* handler lorsque ctrl + c est effectuer */
	struct sigaction arretSig;
	arretSig.sa_handler = arretServeur;
	sigemptyset (&arretSig.sa_mask);
	arretSig.sa_flags = 0;
	sigaction(SIGINT, &arretSig, NULL);

	int typeReq = -1;
	char nomFichier[R_tailleMaxData];
	int fdFichierAudio = -1;
	int rate, size, channels;
	int erreur = -1;
	char buffer[R_tailleMaxData];
	int lectureWav = -1;

	initServer(&serveur,AS_portServer);
	while(1){
		typeReq = attendreMsg(&serveur);
		if(typeReq < 0){
			perror("Erreur lors de la reception d'un message");
			fermerServer(&serveur);
			return 1;
		}

		printf("%d\n", typeReq);
		switch(typeReq){
			case R_demandeCo:
				printf("demande de connexion ...\n");
				accepterConnexion(&serveur);
				break;
			case R_fermerCo:
				printf("fermeture de connexion id : %d \n", serveur.reqRecv->id);
				fermerConnexionClient(&serveur,serveur.reqRecv->id);

				typeReq = -1;
				erreur = -1;
				close(fdFichierAudio);
				fdFichierAudio = -1;
				lectureWav = -1;
				
				break;
			case R_demanderFicherAudio:
				 memcpy(nomFichier, serveur.reqRecv->data, serveur.reqRecv->tailleData);

				printf("Recherche du fichier %s ...\n",nomFichier);
				erreur = aud_readinit(nomFichier, &rate, &size, &channels);
				fdFichierAudio = open(nomFichier,O_RDONLY);

				if (erreur < 0 || fdFichierAudio < 0){
					fichierNonTrouver(&serveur);
					printf("Fichier non trouvé\n");
				}else{
					fichierTrouver(&serveur, rate, size, channels);
					printf("Fichier trouvé\n");
				}
				break;

			case R_demandePartieSuivanteFichier:
				if (fdFichierAudio < 0){
					fichierNonTrouver(&serveur);
				}else{
					lectureWav = read(fdFichierAudio, buffer, R_tailleMaxData);
					if (lectureWav > 0){
						if (rand() % 5 != 1){
							envoyerPartieFichier(&serveur, buffer, lectureWav);
						}
					}else{
						finFichier(&serveur);
					}
				}
				break;
			case R_redemandePartieFichier:
				/* même chose que le cas de R_demandePartieSuivanteFichier, mais on n'appel pas la fonction read() */
				if (fdFichierAudio < 0){
					fichierNonTrouver(&serveur);
				}else{
					if (lectureWav > 0){
						envoyerPartieFichier(&serveur, buffer, lectureWav);
					}else{
						finFichier(&serveur);
					}
				}
				break;

			default:
			  printf("Type de requete non traité : %d", serveur.reqRecv->typeReq);
			  break;
		}
	}
	return 0;
}



