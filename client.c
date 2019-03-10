#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

#define BUFLEN 256

void error(char *msg)
{
	char error_buffer[BUFLEN];
	strcat(error_buffer, "-10 : ");
	strcat(error_buffer, msg);
    perror(error_buffer);
    exit(1);
}

int main(int argc, char *argv[])
{
    int tcp_sockfd, udp_sockfd, n, fdmax, logged_flag = 0, nr_card_logat = 0, stop = 0, transfer_flag_print = 0, transfer_flag = 0, close_flag = 0;
    int try_card = 0, unlock_running = 0;

    struct sockaddr_in serv_addr;
    struct hostent *server;
    int servsize = sizeof(struct sockaddr_in);
    FILE *fptr;

    char cmd[BUFLEN], aux_cmd[BUFLEN], aux_res[BUFLEN], logged_aux[BUFLEN], logout_aux[BUFLEN], nr_card_string[50], unlock_string[BUFLEN];
    char result[BUFLEN];
    char pid[BUFLEN];
    char log_name[50] = "client-";
    char *token, *res_token, *logged_token;
    char tok[20];

    fd_set read_fds;
    fd_set tmp_fds;

    if (argc < 3) {
       fprintf(stderr,"Usage %s <IP_server> <port_server>\n", argv[0]);
       exit(0);
    }  

    sprintf(pid, "%d", getpid());
    strcat(log_name, pid);
    strcat(log_name, ".log");

    if ((fptr = fopen(log_name, "a")) == NULL) {
        error("Eroare la deschiderea fisierului de output!");
    }

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    
	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(tcp_sockfd < 0)
		error("Eroare deschidere socket TCP!");
	

	udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sockfd < 0) 
        error("Eroare deschidere socket UDP!");


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    
    
    if (connect(tcp_sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");    
    
    FD_SET(tcp_sockfd, &read_fds);
    FD_SET(udp_sockfd, &read_fds);
    FD_SET(0, &read_fds);

    if(tcp_sockfd >= udp_sockfd)
    	fdmax = tcp_sockfd;
    else
    	fdmax = udp_sockfd;

   

    while(1){
    	if(stop)
    		break;

    	tmp_fds = read_fds;
    	if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("ERROR in select");

		for(int z = 0; (z <= fdmax) && !stop ; z++){
			if(FD_ISSET(z, &tmp_fds)){
				if(z == 0){
			  		//citesc de la tastatura
			    	memset(cmd, 0, BUFLEN);
			    	fgets(cmd, BUFLEN-1, stdin);
			    	strcpy(aux_cmd, cmd);

			    	token = strtok(cmd, " ");

			    	if(transfer_flag == 1){
			    		transfer_flag_print = 1;
			    		n = send(tcp_sockfd, aux_cmd, strlen(aux_cmd), 0);
				    	if (n < 0) 
				        	 error("ERROR writing to socket");
				       	transfer_flag = 0;
			    	}

			    	if(unlock_running == 1){
			    		memset(unlock_string, 0, BUFLEN);
			    		sprintf(nr_card_string, "%d", try_card);
			    		strcat(unlock_string, nr_card_string);
			    		strcat(unlock_string, " ");
			    		strcat(unlock_string, aux_cmd);
			    		strcat(unlock_string, "\0");
				        n = sendto(udp_sockfd, unlock_string, strlen(unlock_string), 0, (struct sockaddr*) &serv_addr, servsize);	
		    			if (n < 0) 
		    				error("ERROR writing to socket");
				       	unlock_running = 0;
			    	}

			    	/* Comanda de login */
			    	if(token != NULL){
			    		if (strcmp(token, "login") == 0){
			    			token = strtok(NULL, " ");
			    			if(token != NULL)
			    				try_card = atoi(token);

							if(logged_flag == 1){
								printf("%s", "IBANK> -2 : Sesiune deja deschisa\n");
					    		fprintf(fptr, "%s", aux_cmd);
					    		fprintf(fptr, "%s", "IBANK> -2 : Sesiune deja deschisa\n");
					    	} else{
					    	//trimit mesaj la server
						    	n = send(tcp_sockfd,aux_cmd,strlen(aux_cmd), 0);
						    	if (n < 0) 
						        	 error("ERROR writing to socket");
						    }
			    		}

			    		if (strcmp(token, "transfer") == 0) {
			    			if (logged_flag == 1) {
			    				transfer_flag_print = 1;

				    			n = send(tcp_sockfd, aux_cmd, strlen(aux_cmd), 0);	
				    			if (n < 0) 
				    				error("ERROR writing to socket");
			    			} else {
			    				fprintf(fptr, "%s", aux_cmd);
				    			printf("%s", "IBANK> -1 : Clientul nu este autentificat\n");
				    			fprintf(fptr, "%s", "IBANK> -1 : Clientul nu este autentificat\n");
			    			}
			    		}
			    	}



			    	/* Comanda de logout */

			    	if (strcmp(aux_cmd, "logout\n") == 0){
			    		if (logged_flag == 1) {
			    			logged_flag = 0;

			    			n = send(tcp_sockfd, aux_cmd,strlen(aux_cmd), 0);	
			    			if (n < 0) 
			    				error("ERROR writing to socket");
			    		} else {
			    			fprintf(fptr, "%s", aux_cmd);
			    			printf("%s", "IBANK> -1 : Clientul nu este autentificat\n");
			    			fprintf(fptr, "%s", "IBANK> -1 : Clientul nu este autentificat\n");
			    		}
			    	}

			    	/* Comanda de listsold */

			    	if (strcmp(aux_cmd, "listsold\n") == 0){

			    		if (logged_flag == 1) {
			    			n = send(tcp_sockfd, aux_cmd,strlen(aux_cmd), 0);	
			    			if (n < 0) 
			    				error("ERROR writing to socket");
			    		} else {
			    			fprintf(fptr, "%s", aux_cmd);
			    			printf("%s", "IBANK> -1 : Clientul nu este autentificat\n");
			    			fprintf(fptr, "%s", "IBANK> -1 : Clientul nu este autentificat\n");
			    		}
			    	}

			    	/* Comanda de quit */

			    	if (strcmp(aux_cmd, "quit\n") == 0){

		    			n = send(tcp_sockfd, aux_cmd, strlen(aux_cmd), 0);
		    			if (n < 0) 
		    				error("ERROR writing to socket");
		    			fprintf(fptr, "%s", aux_cmd);
		    			stop = 1;
		    			close(tcp_sockfd);
				    	break;
			    	}

			    	if (strcmp(aux_cmd, "unlock\n") == 0){

		    			memset(logout_aux, 0, BUFLEN);
		    			strncat(logout_aux, aux_cmd, strlen(aux_cmd) - 1);
		    			sprintf(nr_card_string, " %d\n", try_card);
		    			strcat(logout_aux, nr_card_string);
		    			n = sendto(udp_sockfd, logout_aux, strlen(logout_aux), 0, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr));	
		    			if (n < 0) 
		    				error("ERROR writing to socket");
			    	}

			    } else if(z == tcp_sockfd){
			    	memset(result, 0, BUFLEN);
			       	n = recv(tcp_sockfd, result, BUFLEN, 0);
			       	if (n < 0)
			       		error("ERROR receiving message");
			       	if(n == 0){
			       		stop = 1;
			       		break;
			       	}

			       	strcpy(aux_res, result);
			       	res_token = strtok(aux_res, " ");

			       	while(res_token != NULL){
			       		if(strcmp(res_token, "Welcome") == 0)
			       			logged_flag = 1;

			       		if(strcmp(res_token, "Transfer") == 0 && isdigit(result[16]))
			       			transfer_flag = 1;

			       		if(strcmp(res_token, "Conexiune") == 0)
			       			close_flag = 1;
			       		
			       		if( isdigit(res_token[0]) && result[7] == 'W')
			       			nr_card_logat = atoi(res_token);
			       		
			       		res_token = strtok(NULL, " ");
			       	}

			       	if(close_flag == 1){
			       		printf("%s", result);
			       	} else {
				       	if(!logged_flag || transfer_flag_print == 1){
					       	printf("%s", result);
					       	fprintf(fptr, "%s", aux_cmd);
					       	fprintf(fptr, "%s", result);
					       	transfer_flag_print = 0;
					    } else {
					    	fprintf(fptr, "%s", aux_cmd);
					    	memset(logged_aux, 0, BUFLEN);
					    	logged_token = strtok(result, " ");
					    	for(int k = 0; k < 4; k++){
					    		if(logged_token != NULL){
					    			strcat(logged_aux, logged_token);
					    			strcat(logged_aux, " ");
					    			logged_token = strtok(NULL, " ");
					    		}
					    	}
					    	logged_aux[strlen(logged_aux) - 1] = '\n';
							printf("%s", logged_aux);
							fprintf(fptr, "%s", logged_aux);
					    }
					}
			    } else if (z = udp_sockfd){
			    	memset(result, 0, BUFLEN);
			       	n = recvfrom(udp_sockfd, result, BUFLEN, 0, (struct sockaddr*) &serv_addr, (socklen_t *) &servsize);
			       	if (n < 0)
			       		error("ERROR receiving message");
			       	if(n == 0){
			       		stop = 1;
			       		break;
			       	}

			       	strcpy(aux_res, result);
			       	res_token = strtok(aux_res, " ");
			       	printf("%s", result);
			       	fprintf(fptr, "%s", aux_cmd);
					fprintf(fptr, "%s", result);

					while(res_token != NULL){
						if(strcmp(res_token, "Trimite") == 0){
							unlock_running = 1;
						}
						res_token = strtok(NULL, " ");
					}

			    }

			}
		}
    }

    close(tcp_sockfd);
    close(udp_sockfd);
    fclose(fptr);
   // Comanda send pentru UDP: sendto(udp_sockfd, buffer, 13, 0, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr));
    return 0;
}


