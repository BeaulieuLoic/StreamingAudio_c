CC = gcc
CFLAGS = -g -Wall

SRCDIR = src/
BINDIR = bin/
OBJDIR = obj/
INCDIR = includes/


$(BINDIR)audioclient: $(OBJDIR)audioclient.o $(OBJDIR)requete.o $(OBJDIR)client.o $(OBJDIR)audio.o
	$(CC) -o $(BINDIR)audioclient $(OBJDIR)audioclient.o $(OBJDIR)requete.o $(OBJDIR)client.o $(OBJDIR)audio.o

$(BINDIR)audioserver: $(OBJDIR)audioserver.o $(OBJDIR)requete.o $(OBJDIR)server.o $(OBJDIR)audio.o
	$(CC) -o $(BINDIR)audioserver $(OBJDIR)audioserver.o $(OBJDIR)requete.o $(OBJDIR)server.o $(OBJDIR)audio.o

$(OBJDIR)audioserver.o: $(SRCDIR)audioserver.c $(OBJDIR)requete.o $(OBJDIR)server.o 
	$(CC) $(CFLAGS) -c -I $(INCDIR) $(SRCDIR)audioserver.c -o $(OBJDIR)audioserver.o

$(OBJDIR)audioclient.o: $(SRCDIR)audioclient.c $(OBJDIR)requete.o $(OBJDIR)client.o 
	$(CC) $(CFLAGS) -c -I $(INCDIR) $(SRCDIR)audioclient.c -o $(OBJDIR)audioclient.o



$(OBJDIR)client.o: $(SRCDIR)client.c $(INCDIR)client.h $(OBJDIR)requete.o
	$(CC) $(CFLAGS) -c -I $(INCDIR) $(SRCDIR)client.c -o $(OBJDIR)client.o

$(OBJDIR)server.o: $(SRCDIR)server.c $(INCDIR)server.h $(OBJDIR)requete.o
	$(CC) $(CFLAGS) -c -I $(INCDIR) $(SRCDIR)server.c -o $(OBJDIR)server.o

$(OBJDIR)requete.o: $(SRCDIR)requete.c $(INCDIR)requete.h
	$(CC) $(CFLAGS) -c -I $(INCDIR) $(SRCDIR)requete.c -o $(OBJDIR)requete.o

$(OBJDIR)audio.o: $(SRCDIR)audio.c
	gcc -Wall -c -I $(INCDIR) $(SRCDIR)audio.c -o $(OBJDIR)audio.o

t: test
	./a

test: test.c $(OBJDIR)requete.o
	$(CC) $(CFLAGS) -I $(INCDIR) test.c -o a $(OBJDIR)requete.o

clean:
	rm $(OBJDIR)*;rm $(BINDIR)*