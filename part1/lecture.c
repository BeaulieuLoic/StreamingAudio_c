#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "../syr2-audio-lib/audio.h"


#define tailleBuffer 1024

int main(int argc, char const *argv[]) {
	if (argc != 2){
		printf("nombre d'argument invalide\n");
		return -1;
	}
	int rate, size, channels;
	char file[tailleBuffer], buffer[tailleBuffer];
	int speaker, fileWav;

	strncpy(file,argv[1],tailleBuffer);
	file[strlen(file)] = '\0';


	if(aud_readinit(file, &rate, &size, &channels)<0){
		perror("erreur lors de la lecture du fichier audio");
		return -1;
	}
	speaker = aud_writeinit(rate, size, channels);
	if (speaker<0){
		perror("erreur 1");
		return -1;
	}



	fileWav = open(file,O_RDONLY);
	int lectureWav = read(fileWav, buffer, tailleBuffer);

	while(lectureWav!=1){
		if(write(speaker, buffer, tailleBuffer)<0){
			perror("erreur ecriture");
		}
		lectureWav = read(fileWav, buffer, tailleBuffer);
	}




	close(fileWav);
	close(speaker);
	return 0;
}