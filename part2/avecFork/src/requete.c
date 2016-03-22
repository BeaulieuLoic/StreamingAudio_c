#include "requete.h"


void initRequete(requete *req,int typeReq, int id,unsigned int tailleData, char *buf){
	//requete* req = malloc(sizeof(requete));
	if (req == NULL){
		perror("Erreur lors de l'appel à initRequete, req est null");
		return;
	}
	req -> typeReq = typeReq;
	req -> id = id;
	req -> tailleData = tailleData;

	if (req -> tailleData > R_tailleMaxData){
		req -> tailleData = R_tailleMaxData;
		printf("Warning lors de createRequeteFromBytes, req -> tailleData >R_tailleMaxData\n");
	}else if(req -> tailleData < 0){
		printf("Warning lors de createRequeteFromBytes, req -> tailleData < 0\n");
		req -> tailleData = 0;
	}

	if (buf == NULL){
		req -> tailleData = 0;	
	}
	memcpy(req -> data, buf, req -> tailleData);
}

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
		printf("Warning lors de createRequeteFromBytes, req -> tailleData >R_tailleMaxData\n");
	}else if(req -> tailleData < 0){
		printf("Warning lors de createRequeteFromBytes, req -> tailleData < 0\n");
		req -> tailleData = 0;
	}
	memcpy(req -> data, bytes+(tailleInt*3), req -> tailleData);


	return ;
}

void freeReq(requete* req){
	if (req != NULL){
		free(req);
	}
}

int sizeofReq(requete req){
	return sizeof(int)*3+req.tailleData;
}

int requeteToBytes(char *dest, requete req){
	if (dest == NULL){
		perror("Erreur lors de l'appel à requeteToBytes, dest est NULL");
		return -1;
	}

	char tmp[sizeof(int)];

	intToBytes(tmp ,req.typeReq);
	memcpy(dest, tmp, sizeof(int));

	intToBytes(tmp ,req.id);
	memcpy(dest+sizeof(int), tmp, sizeof(int));

	intToBytes(tmp ,req.tailleData);
	memcpy(dest+sizeof(int)*2, tmp, sizeof(int));

	memcpy(dest+sizeof(int)*3, req.data, req.tailleData);
	
	return 1;

}

int bytesToInt(char *bytes){
	int tailleInt = sizeof(int);
	int i = 0;
	int aRetourner = 0;

	for (i = 0; i < tailleInt; ++i){
		aRetourner = aRetourner | ((unsigned char)bytes[i] << (8*i));
	}

	return aRetourner;
}

void intToBytes(char *dest, int n){
	int tailleInt = sizeof(int);
	int i = 0;
	int tmp = 0;
	for (i = 0; i < tailleInt; ++i){
		tmp = (n >> 8*i) & 0xFF;
		dest[i] = *(char *) (&tmp);
	}
}