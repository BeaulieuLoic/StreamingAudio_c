#include "audioserver.h"
#include "server.h"


server serveur;

/* 
	arrèt du serveur proprement lorsque le signal SIGINT (ctrl + c) est reçu 
*/
void arretServeur(int sig){
	char *msgArret = "\nArret du serveur ...\n";
	if (sig == SIGINT){
		fprintf(stdout, "%s\n", msgArret);
	}
}


int main(int argc, char const *argv[]) {
	/* handler lorsque ctrl + c est envoyer */
	struct sigaction arretSig;
	arretSig.sa_handler = arretServeur;
	sigemptyset (&arretSig.sa_mask);
	arretSig.sa_flags = 0;
	sigaction(SIGINT, &arretSig, NULL);

	int typeReq;

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
				accepterConnexion(&serveur);
				break;
			case R_fermerCo:
				fermerConnexionClient(&serveur,serveur.reqRecv->id);
				
				break;
			case R_demanderFicher:
				printf("%s\n", serveur.reqRecv->data);
				break;

			default:
			  printf("Type de requete non traité : %d", serveur.reqRecv->typeReq);
			  break;
		}
	}
	return 0;
}

