#include <stdio.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <pthread.h>

/*
int accepter(int socket_serveur){
	int socket_client;
	socket_client = accept(socket_serveur, NULL, NULL);
	if(socket_client == -1){
		perror("accept socket");
	}
	sleep(1);
	const char *message_bienvenue = "Bonjour, bienvenue sur notre serveur\nNous sommes tres heureux de vous recevoir\nJ'adore les sucettes, et les gros calins !\nJe suis Charlie\nC'est l'histoire d'un mec qui rentre dans un cafe, et PLOUF !\nTu connais la blague a deux balles ? PAN PAN !\nJ'ai envie de me suicider parce que c'est cool la mort....... ouais c'est trop cool\n";
	return write(socket_client, message_bienvenue, strlen( message_bienvenue));
}
*/


int ecouter(int socket_serveur){
	if(listen(socket_serveur, 10) == -1){
		perror("ecoute socket");
		return -1;
	}
	return socket_serveur;
}


int config_socket(int socket_serveur){
	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET; /* Socket ipv4 */
	saddr.sin_port = htons(8080); /* Port d ’ écoute */
	saddr.sin_addr.s_addr = INADDR_ANY; /* écoute sur toutes les interfaces */
	if(bind(socket_serveur, (struct sockaddr *)&saddr, sizeof(saddr)) == -1){
		perror("bind socket");
		return -1;
	}
	return ecouter(socket_serveur);
}


int creer_serveur(int port){
	int socket_serveur = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_serveur == -1){
		perror("socket serveur");
		return -1;
	}
	int optval = 1;
	if (setsockopt(socket_serveur, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
		perror ("Can not set SO_REUSEADDR option");
	return config_socket(socket_serveur); 	
}
