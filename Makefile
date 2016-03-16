lecture: part1/lecture.o syr2-audio-lib/audio.o
	gcc -o part1/lecture part1/lecture.o syr2-audio-lib/audio.o
	
lecture.o: part1/lecture.c syr2-audio-lib/audio.c
	gcc -g -Wall -c part1/lecture.c -o part1/lecture.o

audio.o: syr2-audio-lib/audio.c
	gcc -Wall -c syr2-audio-lib/audio.c -o syr2-audio-lib/audio.o




client: part2/audioclient.o
	gcc -o part2/client part2/audioclient.o

server: part2/audioserver.o
	gcc -o part2/server part2/audioserver.o

audioclient.o: part2/audioclient.c
	gcc -g -Wall -c part2/audioclient.c -o part2/audioclient.o

audioserver.o: audioserver.c
	gcc -g -Wall -c part2/audioserver.c -o part2/audioserver.o