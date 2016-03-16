#ifndef _AUDIOCLIENT_H_
#define _AUDIOCLIENT_H_


#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

/* port utilisé par le serveur */
#define AC_portServ 5000


/* timeout en microseconde */
#define AC_timeoutDemandeCo 1000000 // = 1 seconde


//int initSocket(struct sockaddr_in *addrSend, const char * adrServ);

//int demandeDeConnexion(int fdSocket,struct sockaddr_in *dest,struct sockaddr_in *from);

//int fermetureConnexion(int id, int fdSocket, struct sockaddr_in *dest, struct sockaddr_in *from);

#endif