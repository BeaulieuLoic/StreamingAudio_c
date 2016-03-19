#include "audioserver.h"
#include "server.h"
#include "audio.h"

#include <time.h> /*random, à enlever !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

server serveur;
int serveurOccuper = 0;
/* 
	arret du serveur proprement lorsque le signal SIGINT (ctrl + c) est reçu 
*/
void arretServeur(int sig){
	char *msgArret = "\nArret du serveur ...\n";
	if (sig == SIGINT){
		fprintf(stdout, "%s\n", msgArret);
		exit(0);
	}
}


int main(int argc, char const *argv[]) {
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
	int lectureWav = 0;

	initServer(&serveur,AS_portServer);
	while(1){
		typeReq = attendreMsg(&serveur);
		if(typeReq < 0){
			perror("Erreur lors de la reception d'un message");
			fermerServer(&serveur);
			return 1;
		}

		switch(typeReq){
			case R_demandeCo:
				printf("demande de connexion ...\n");
				if(accepterConnexion(&serveur, genererIdUnique())<0){
					printf("Client refusé\n");
				}
				break;
			case R_fermerCo:
				printf("fermeture de connexion id : %d \n", serveur.reqRecv->id);
				fermerConnexionClient(&serveur,serveur.reqRecv->id);

				typeReq = -1;
				erreur = -1;
				fdFichierAudio = -1;
				lectureWav = -1;
				close(fdFichierAudio);
				serveurOccuper = 0;
				
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
					lectureWav = read(fdFichierAudio, buffer, R_tailleMaxData);
					printf("Fichier trouvé\n");
				}
				break;

			case R_demandePartieSuivanteFichier:
				if (fdFichierAudio < 0){
					fichierNonTrouver(&serveur);
				}else{
					if (lectureWav > 0){
						envoyerPartieFichier(&serveur, buffer, lectureWav);
						lectureWav = read(fdFichierAudio, buffer, R_tailleMaxData);
					}else{
						// Indique au client que le fichier est finit
						// Le fichier est fermé lorsque le client ferme sa connexion avec le serveur
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
						// Indique au client que le fichier est finit
						// Le fichier est fermé lorsque le client ferme sa connexion avec le serveur
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



int genererIdUnique(){
	if (serveurOccuper){
		return -1;
	}
	serveurOccuper = 1;
	return 999;
}