#include "audioclient.h"
#include "client.h"



client cl;
/* 
	arrèt du client proprement lorsque le signal SIGINT (ctrl + c) est reçu.
	prévient le serveur que le client se déconnecte
*/
void arretClient(int sig){
	printf("Arret ...\n");
	fermerConnexion(&cl);
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

	if (argc < 3){
		printf("nombre d'argument invalide %d\n",argc);
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
		fermerConnexion(&cl);
		exit(2);
	}

	if(demanderFichier(&cl,fichierARecup)<0){
		fermerConnexion(&cl);
		exit(3);
	}

	/* 
		Fermeture de connexion :
			envois d'une requète R_fermerCo au serveur
			libère la mémoire alloué
 */
	fermerConnexion(&cl);
	return 0;
}

