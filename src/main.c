#include <stdio.h>

#include "socket.h"

int main(int argc, char **argv) {
	/* Arnold Robbins in the LJ of February ’95 , describing RCS */

	int res = creer_serveur(8000);
	if(res != -1){
	
		while(1){
			accepter(res);
			printf("success\n");
		}
		
	} else {
		printf("fail\n");
		return -1;
	}
	return 0;



}




