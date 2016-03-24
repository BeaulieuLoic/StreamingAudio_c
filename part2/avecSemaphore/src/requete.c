#include "requete.h"

// Liste et documentation des types de requète dans requete.h


/*
	Initialise une requète avec typeReq, id, tailleData 
	et le contenu de buf dans data.

	Si tailleData est supérieur à R_tailleMaxData, il est remis à R_tailleMaxData
*/
void initRequete(requete *req,int typeReq, int id,unsigned int tailleData, char *buf){
	if (req == NULL){
		perror("Erreur lors de l'appel à initRequete, req est null");
		return;
	}
	req -> typeReq = typeReq;
	req -> id = id;
	req -> tailleData = tailleData;

	if (req -> tailleData > R_tailleMaxData){
		req -> tailleData = R_tailleMaxData;
		printf("Warning initRequete, tailleData > R_tailleMaxData\n");
	}else if(req -> tailleData < 0){
		printf("Warning initRequete, tailleData < 0\n");
		req -> tailleData = 0;
	}

	if (buf == NULL){
		req -> tailleData = 0;	
	}
	memcpy(req -> data, buf, req -> tailleData);
}

/*
	Initialise une requète en fonction d'une liste d'octet
	bytes est divisé en 4 partie :

		1ere partie correspond au type de la requète. Codé sur 'sizeof(int)' octets
		
		2eme partie à l'éventuel identifiant du client. Codé sur 'sizeof(int)' octets

		3eme partie à la taille de la zone data qui se trouve en partie 4. Codé sur 'sizeof(int)' octets

		4eme partie qui sera copier dans la partie data de la requete. Cette 4eme à la longueur qui est indiqué dans la 3eme partie

	bytes doit avoir une taille égal ou supérieur à sizeofReq(req)
	Possibilité d'utilisé la constante R_tailleMaxReq pour être sur d'avoir aucun problème de débordement
*/
void initRequeteFromBytes(requete *req, char *bytes){
	if (bytes == NULL){
		perror("Erreur lors de l'appel à initRequeteFromBytes, bytes est null");
		return ;
	}
	if (req == NULL){
		perror("Erreur lors de l'appel à initRequeteFromBytes, req est null");
		return ;
	}

	int tailleInt = sizeof(int);
	req -> typeReq = bytesToInt(bytes);
	req -> id = bytesToInt(bytes+tailleInt);
	req -> tailleData = bytesToInt(bytes+(tailleInt*2));
	

	if (req -> tailleData > R_tailleMaxData){
		req -> tailleData = R_tailleMaxData;
		printf("Warning initRequeteFromBytes, tailleData > R_tailleMaxData\n");
	}else if(req -> tailleData < 0){
		printf("Warning initRequeteFromBytes, tailleData < 0\n");
		req -> tailleData = 0;
	}
	memcpy(req -> data, bytes+(tailleInt*3), req -> tailleData);


	return ;
}

/*
	Renvois la taille en octet que devra prendre la requète req
	soit : sizeof(int)*3 + req->tailleReq
*/
int sizeofReq(requete req){
	return sizeof(int)*3+req.tailleData;
}

/*
	Convertie une requète en sa représentation en octet.
	dest doit être égal ou supérieur à 
	sizeofReq(reqAConvertir)

	Possibilité d'utilisé la constante R_tailleMaxReq pour définir 
	la taille de dest et être assuré qu'il possède une taille suffisante
*/
int requeteToBytes(char *dest, requete req){
	if (dest == NULL){
		perror("Erreur lors de l'appel à requeteToBytes, dest est NULL");
		return -1;
	}

	char tmp[sizeof(int)];

	// 1er partie correspond au type de la requète. Codé sur 'sizeof(int)' octets
	intToBytes(tmp ,req.typeReq);
	memcpy(dest, tmp, sizeof(int));

	// 2eme partie est l'éventuel identifiant du client. Codé sur 'sizeof(int)' octets
	intToBytes(tmp ,req.id);
	memcpy(dest+sizeof(int), tmp, sizeof(int));

	// 3eme partie est la taille de la zone data qui se trouve en partie 4. Codé sur 'sizeof(int)' octets
	intToBytes(tmp ,req.tailleData);
	memcpy(dest+sizeof(int)*2, tmp, sizeof(int));

	// 4eme qui correspond a la partie data. Cette 4eme à la longueur qui est indiqué dans la 3eme partie
	memcpy(dest+sizeof(int)*3, req.data, req.tailleData);
	
	return 1;

}

/* 
	Converti les 'sizeof(int)' 1er octet de bytes en int
*/
int bytesToInt(char *bytes){
	int tailleInt = sizeof(int);
	int i = 0;
	int aRetourner = 0;

	for (i = 0; i < tailleInt; ++i){
		aRetourner = aRetourner | ((unsigned char)bytes[i] << (8*i));
	}

	return aRetourner;
}

/* 
	Convertie un int en sa représentation en octet. 
	dest doit avoir une taille d'au moin sizeof(int) 
*/
void intToBytes(char *dest, int n){
	int tailleInt = sizeof(int);
	int i = 0;
	int tmp = 0;
	for (i = 0; i < tailleInt; ++i){
		tmp = (n >> 8*i) & 0xFF;
		dest[i] = (char) (tmp);
	}
}