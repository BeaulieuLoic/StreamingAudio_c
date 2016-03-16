#ifndef _AUDIOSERVER_H_
#define _AUDIOSERVER_H_

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#define AS_portServer 5000

void arretServeur(int sig);

#endif