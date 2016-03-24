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







int main(int argc, char const *argv[]) {
	int fdSocket = socket(AF_INET, SOCK_DGRAM, 0);
	int erreur;
	char buffer[512] = "test";
	if (fdSocket < 0){
		perror("Erreur lors de la création d'une socket");
	}

	struct sockaddr_in serveur;
	serveur.sin_family = AF_INET;
	serveur.sin_port = htons(portServeur);
	serveur.sin_addr.s_addr = inet_addr("127.0.0.1");
	socklen_t addrlen = sizeof(struct sockaddr_in);

	erreur = bind(fdSocket, (struct sockaddr *)  &serveur,addrlen);
	if (erreur <0)
	{
		perror("erreur lors du bind");
	}



	struct sockaddr_in client;
	socklen_t fromlen = sizeof(struct sockaddr_in);

	erreur = recvfrom(fdSocket,buffer,sizeof(buffer),0,(struct sockaddr*) &client,&fromlen);
	if(erreur < 0){
		perror("erreur lors de la reception d'un message");
	}
	printf("msg recu%s\n",buffer);




	char msg[512] = "msg bien recu";

	erreur = sendto(fdSocket, msg,strlen(msg)+1,0, (struct sockaddr*) &client, sizeof(struct sockaddr_in));

	if (erreur < 0)
	{
		perror("erreur lors de l'envois d'un message");
	}

}