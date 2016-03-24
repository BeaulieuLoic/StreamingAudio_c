#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#define portServeur 5000
#define addrServeur "127.0.0.1"


int main(int argc, char const *argv[]) {
	char *server_name , *fileName;
	int tailleArg1,tailleArg2;
	int erreur;

	if (argc != 3){
		printf("nombre d'argument invalide\n");
		return -1;
	}

	tailleArg1 = sizeof(argv[1]);
	tailleArg2 = sizeof(argv[2]);

	server_name = malloc(tailleArg1+1);
	strncpy(server_name,argv[1],tailleArg1);
	server_name[strlen(server_name)] = '\0';


	fileName = malloc(tailleArg2+1);
	strncpy(fileName,argv[2],tailleArg2);
	fileName[strlen(fileName)] = '\0';



	//printf("%s, %s\n", server_name,fileName);

	struct sockaddr_in dest;

	int fdSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (fdSocket < 0){
		perror("Erreur lors de la création d'une socket");
	}

	dest.sin_family = AF_INET;
	dest.sin_port = htons(portServeur);
	dest.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	char buffer[512] = "test";

	erreur = sendto(fdSocket, buffer,strlen(buffer)+1,0, (struct sockaddr*) &dest, sizeof(struct sockaddr_in));

	if (erreur < 0)
	{
		perror("erreur lors de l'envois d'un message");
	}

	

	struct sockaddr_in from;
	socklen_t fromlen = sizeof(struct sockaddr_in);

	erreur = recvfrom(fdSocket,buffer,sizeof(buffer),0,(struct sockaddr*) &from,&fromlen);
	if(erreur < 0){
		perror("erreur lors de la reception d'un message");
	}


	printf("msg envoyer par le serveur : %s\n", buffer);







	close(fdSocket);
	free(server_name);
	free(fileName);
	return 0;
}



