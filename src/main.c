#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "socket.h"

int main(int argc, char **argv) {
	int res = creer_serveur(8080);
	if(res != -1){
		while(1){
			int socket_client;
			socket_client = accept(res, NULL, NULL);
			printf("CONNEXION ACCEPTEE\n");
			int fils;
			fils = fork();
			if(fils == 0) {				//DEBUT DU FILS
				sleep(1);
				const char *message_bienvenue = "Bonjour, bienvenue sur notre serveur\nNous sommes tres heureux de vous recevoir\nJ'adore les sucettes, et les gros calins !\nJe suis Charlie\nC'est l'histoire d'un mec qui rentre dans un cafe, et PLOUF !\nTu connais la blague a deux balles ? PAN PAN !\nJ'ai envie de me suicider parce que c'est cool la mort....... ouais c'est trop cool\nVive les lamasticots !!!\n";
				write(socket_client, message_bienvenue, strlen(message_bienvenue));
				int quit = 0;				
				while(quit == 0){
					char buf[100];
					int taille;
					taille = read(socket_client, buf, sizeof(buf));
					if(taille == 0)
						quit = 1;						
					else if(taille != -1){
						printf("message reçu :-D\n");					
						write(socket_client, buf, taille);
						sleep(1);
						printf("			message envoyé\n");
					} else{
						printf("message pas reçu :-(\n");
					}
				}
			}					//FIN DU FILS
		}
	} else {
		printf("fail\n");
		return -1;
	}
	return 0;
}




