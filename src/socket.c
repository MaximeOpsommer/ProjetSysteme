#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 256

enum http_method {
	HTTP_GET,
	HTTP_UNSUPPORTED,
};

typedef struct {
	enum http_method method;
	int major_version;
	int minor_version;
	char * url;
} http_request;

typedef struct {
	const char * extension;
	const char * mime;
} extension_to_mime;

const char *message_bienvenue = "\n<kiwi> Bonjour, bienvenue sur notre serveur\n<kiwi> Nous sommes tres heureux de vous recevoir\n<kiwi> J'adore les sucettes, et les gros calins !\n<kiwi> Je suis Charlie\n<kiwi> C'est l'histoire d'un mec qui rentre dans un cafe, et PLOUF !\n<kiwi> Tu connais la blague a deux balles ? PAN PAN !\n<kiwi> J'ai envie de me suicider parce que c'est cool la mort....... ouais c'est trop cool\n<kiwi> Vive les lamasticots !!!\n\n";

int socket_client;
int fils;
int taille;
FILE * fclient;
int analyse_ligne1;

extension_to_mime supported_mimes[] = {
	{".jpg", "image/jpeg"},
	{".jpeg", "image/jpeg"},
	{".png", "image/png"},
	{".css", "text/css"},
	{".html", "text/html"},
	{NULL, NULL}
};

char * get_extension(char *url) {
	return strrchr(url, '.');
}

const char * get_mime(char *ext) {
	int i;
	for(i=0; supported_mimes[i].extension != NULL; ++i) {
		if(strcmp(ext, supported_mimes[i].extension) == 0) {
			return supported_mimes[i].mime;
		}
	}
	return "text/plain";
}

char * analyse_extension(char *url) {
	int i = 0;
	int last_point;
	while(url[i] != '\0') {
		if(url[i] == '.') {
			last_point = i;
		}
		++i;
	}
	char * res = malloc(strlen(url) - last_point);
	for(i = 0; i<strlen(url) - last_point; ++i) {
		res[i] = url[last_point + i + 1];
	}
	res[i] = '\0';
	return res;
}

char * get_content_type(char *url) {
	char *tmp = analyse_extension(url);
	char *res;
	if(strcmp(tmp, "jpg") == 0) {
		printf("Il y a une image jpg");
		res = malloc(strlen("image/jpeg\0"));
		res = "image/jpeg";
	}
	else {
		printf("Il y a du HTML");
		res = malloc(strlen("text/plain\0"));
		res = "text/html";
	}
	return res;
}

int copy(int in, int out) {
	out = dup(in);
	return out;
}

int get_file_size(int fd) {
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}

int check_and_open(const char *url, const char *document_root) {

	/* url <=> fichier ; document_root  */
	/* On s'assure que le fichier est un fichier regulier */
	struct stat tmp_stat;
	if(stat(document_root, &tmp_stat) == -1) {
		perror("stat");
		return -1;
	}
	/*if(!S_ISREG(tmp_stat.st_mode)) {
		return -1;
	}*/
	/* On concatene les chaines pour avoir le chemin absolu */
	char * tmp = malloc(strlen(document_root) + strlen(url));
	strcpy(tmp, document_root);
	strcat(tmp, url);
	/* Ouverture du fichier en lecture seule */
	printf("document_root : %s\n", document_root);
	printf("url : %s\n", url);
	printf("chemin : %s\n", tmp);
	int res = open(tmp, O_RDONLY);
	return res;
}

char *rewrite_url(char *url) {
	int i = 0;
    while(url[i] != '\0' && url[i] != '?')
        ++i;

    char * new_url = malloc(i);
    strncpy(new_url, url, i);
	if(strcmp(new_url, "/") == 0) {
		new_url = "/index.html";
	}
    return new_url;
}

void send_status(FILE * client, int code, const char * reason_phrase) {
	fprintf(client, "HTTP/1.1 %d %s\r\n", code, reason_phrase);
}

void send_response(FILE * client, int code, const char * reason_phrase, const char * message_body) {
	send_status(client, code, reason_phrase);
	fprintf(client, "Content-Length: %zu\r\n\r\n", strlen(message_body));
	fprintf(client, "%s", message_body);
}

void ignore_entete(FILE * fclient) {
	char * buf = malloc(BUFFER_SIZE);
	fgets(buf, BUFFER_SIZE, fclient);
	while (strncmp(buf, "\r\n", 2) != 0 && strncmp(buf, "\n", 1) != 0) {
		fflush(stdout); /* on vide le flux */
        fgets(buf, BUFFER_SIZE, fclient);
	}
}

int premier_mot_GET(const char * buf, http_request * request) {
	request->method = HTTP_UNSUPPORTED;	
	if(strncmp(buf, "GET ", 4) != 0)
		return -1;
	request->method = HTTP_GET;
	return 0;
}

int trois_mots(const char * buf, http_request * request) {
	int nb_espace = 0;
	int position_espace1 = 0;
	int position_espace2;
	int i;
	for(i=0; i<strlen(buf); ++i) {
		if(buf[i] == ' ') {
			++nb_espace;
			if(nb_espace > 2)
				return -1;
			if(position_espace1 == 0)
				position_espace1 = i;
			else
				position_espace2 = i;
		}
	}
	if(position_espace2 - position_espace1 < 2)
		return -1;
	char * tmp = malloc(position_espace2 - position_espace1);

	strncpy(tmp, &buf[position_espace1+1], position_espace2 - position_espace1 - 1);
	tmp[position_espace2 - position_espace1 - 1] = '\0';
	request->url = tmp;
	printf("%s\n", request->url);
	fflush(stdout);
	return 0;
}

int troisieme_mot_HTTP(const char * buf, http_request * request) {
	int i = 0;
	int nb_espace = 0;
	while(nb_espace < 2){
        if(buf[i] == ' ') {
			++nb_espace;
		}
        	++i;
	}
	if(strncmp(&buf[i], "HTTP/1.1", 8) == 0 || strncmp(&buf[i], "HTTP/1.0", 8) == 0) {
		request->major_version = buf[i+5];
		request->minor_version = buf[i+7]; /* on obtient 49 <=> 1 en #html (cf. table ascii) */
		printf("%s\n", buf);
		printf("major_version : %d\nminor_version : %d\n", request->major_version, request->minor_version);
		return 0;
	}
	printf("major_version : %d\nminor_version : %d\n", request->major_version, request->minor_version);
	return -1;
}

int analyse_premiere_ligne(const char * buf, http_request * request) {
	if(premier_mot_GET(buf, request)!=0 || trois_mots(buf, request)!=0 || troisieme_mot_HTTP(buf, request)!=0)
		return -1;
	return 0;
}

/* serveur ---> client */
void envoie_reponse(FILE * fclient, const char * phrase) {
	fprintf(fclient, "<kiwi> %s", phrase);
}

/* client ---> serveur */
char * fgets_or_exit(char * buffer , int size , FILE * stream) {

	/* Réinitialisation du buffer */
	int i;
	for(i = 0; i<size; ++i) {
		buffer[i] = '\0';
	}

	fgets(buffer, size, stream); /* Lecture des donnees envoyees par le client */
	printf("<kiwi> %s", buffer);
	fflush(stdout); /* Vider les buffers du flux */

	/* Quitte si l'on envoie une donnee vide */
	if(buffer[0] == '\0')
		exit(1);

	return buffer;
}

void traitement_signal(int sig){


	int statut;
/*	printf("Signal %d reçu \n", sig);
	if(waitpid(sig) == -1) {
		perror("zombie");
		return;
	}
	*/
	if(sig==SIGCHLD)
		while(waitpid(-1,&statut,WNOHANG)>0)
		{
			if (WIFSIGNALED(statut))
			{
				printf("Terminé par signal %d\n", WTERMSIG(statut));
			}
		}

}

void initialiser_signaux(void){
	struct sigaction sa;
	sa.sa_handler = traitement_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART ;
	if(sigaction(SIGCHLD, &sa, NULL) == -1){
		perror("sigaction(SIGCHLD)");
	}
}

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

void accept_client(int socket_serveur, const char * dossier) {

	socket_client = accept(socket_serveur, NULL, NULL);
	if(socket_client == -1) {
		perror("accept");
	}
	else {
		printf("CONNEXION ACCEPTEE\n");
		fils = fork();
		if(fils == 0) {				//DEBUT DU FILS

			fclient = fdopen(socket_client, "w+");
			char * buf = malloc(BUFFER_SIZE);

			while(fgets_or_exit(buf, BUFFER_SIZE, fclient)){

				//char *url = malloc(BUFFER_SIZE);
				http_request request;
				analyse_ligne1 = analyse_premiere_ligne(buf, &request);
				ignore_entete(fclient);
				request.url = rewrite_url(request.url);
				printf("request.url = %s\n", request.url);
				printf("\trésultat analyse première ligne : %d\n", analyse_ligne1);
				fflush(stdout);

				if(analyse_ligne1 == -1) {
					printf("Bad Request\n");
					send_response(fclient, 400, "Bad Request", "Bad Request\r\n");
				}

				if(request.method == HTTP_UNSUPPORTED) {
					printf("HTTP UNSUPPORTED\n");
					send_response(fclient, 405, "Method Not Allowed", "Method Not Allowed\r\n");
				}

				int fd = check_and_open(request.url, dossier);
				printf("fd = %d\n", fd);
				if(fd == -1) {
					printf("NOT FOUND\n");
					send_response(fclient, 404, "Not Found", "Not Found\r\n");
				}

				else {
					//printf("size = %d\n", get_file_size(fd));
					send_status(fclient, 200, "OK");
					int fdout = copy(fd, fdout);
					int size = get_file_size(fdout);
					//fprintf(fclient, "Content-Type: %s\r\n", get_content_type(request.url));
					const char * mime = get_mime(get_extension(request.url));
					printf("mime : %s\n", mime);
					fprintf(fclient, "Content-Type: %s\r\n", mime);
					printf("sie : %d\n", size);
					fprintf(fclient, "Content-Length: %zu\r\n\r\n", size);
					char *tmp = malloc(size);
					read(fdout, tmp, size);
					fprintf(fclient, "%s", tmp);
					close(fdout);
					//send_response(fclient, 200, "OK", message_bienvenue);
				}

				/*else {
					printf("NOT FOUND\n");
					send_response(fclient, 404, "Not Found", "Not Found\r\n");
				}*/
				close(fd);
				exit(0);
			}
		}					//FIN DU FILS
		else {
			close(socket_client);
		}
	}
}
