#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

#define MAX_CLIENTS	20
#define BUFLEN 256

struct client{
	char nume[12];
	char prenume[12];
	int numar_card;
	int pin;
	char parola_secreta[8];
	double sold;
	int logat;
	int incercari_logare;
	int blocat;
};

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
     int tcpsockfd, udpsockfd, newsockfd, portno, clilen, stop = 0;

     char pass_aux[BUFLEN];
     char buffer[BUFLEN], aux_buffer[BUFLEN], return_buffer[BUFLEN], aux_nr_card[BUFLEN], sold_str[BUFLEN], cmd[BUFLEN], amount[BUFLEN], pass_string[BUFLEN], unlock_buffer[BUFLEN];
     struct sockaddr_in serv_addr, cli_addr;
     int n, i, j, client_nr, nr_card_recv, pin_recv, indice_client, flag_nr_card = 0, unlock_flag = 0, unlock_index = 0, correct_unlock = 0;
     int correct_nr_card = 0, transf_dest, transf_flag = 0, transf_index_dest, transf_index_src, correct_transaction = 0, card_to_unlock = 0;
     char c[100];
     double transf_amount;
     int servsize = sizeof(struct sockaddr_in);
     fd_set read_fds;	//multimea de citire folosita in select()
     fd_set tmp_fds;	//multime folosita temporar 
     int fdmax;		//valoare maxima file descriptor din multimea read_fds

     if (argc < 2) {
         fprintf(stderr,"Usage : %s <port_server> <users_data_file>\n", argv[0]);
         exit(1);
     }


    FILE *fptr;
    char *token, *cmd_token, *pass_token;
    int k = 0;
    int field_counter = 0;

    if ((fptr = fopen(argv[2], "r")) == NULL) {
        printf("Eroare la deschiderea fisierului de clienti!\n");
        exit(1);         
    }

    if( fgets(c, 100, fptr) != NULL ) {
    	client_nr = atoi(c);
   	}

   	struct client client_vector[client_nr];

   	for(int i = 0; i < client_nr; i++){
   		if( fgets(c, 100, fptr) != NULL ) {
    		token = strtok(c, " ");

    		if(token != NULL){
    			if(strlen(token) <= 12)
    				strcpy(client_vector[k].nume, token);
    			else{
    				printf("Nume prea lung!\n");
    				exit(1);
    			}
    			token = strtok(NULL, " ");
    			field_counter++;
    		}

    		if(token != NULL){
    			if(strlen(token) <= 12)
    				strcpy(client_vector[k].prenume, token);
    			else{
    				printf("Prenume prea lung!\n");
    				exit(1);
    			}
    			token = strtok(NULL, " ");
    			field_counter++;
    		}

    		if(token != NULL){
    			if(strlen(token) <= 6)
    				client_vector[k].numar_card = atoi(token);
    			else{
    				printf("Numar card prea lung!\n");
    				exit(1);
    			}
    			token = strtok(NULL, " ");
    			field_counter++;
    		}

    		if(token != NULL){
    			if(strlen(token) <= 4)
    				client_vector[k].pin = atoi(token);
    			else{
    				printf("Pin prea lung!\n");
    				exit(1);
    			}
    			token = strtok(NULL, " ");
    			field_counter++;
    		}

    		if(token != NULL){
    			if(strlen(token) <= 8)
    				strcpy(client_vector[k].parola_secreta, token);
    			else{
    				printf("Parola secreta prea lunga!\n");
    				exit(1);
    			}
    			token = strtok(NULL, " ");
    			field_counter++;
    		}

    		if(token != NULL){
    			sscanf(token, "%lf", &client_vector[k].sold);
    			token = strtok(NULL, " ");
    			field_counter++;
    		}
    		if(field_counter < 6) {
    			printf("Fisier de input incorect, contine o eroare!\n");
    			exit(1);
    		}
    		client_vector[k].logat = 0;
    		client_vector[k].blocat = 0;
    		client_vector[k].incercari_logare = 0;
    		k++;
   		}
   	}

   	fclose(fptr);

   	printf("Lista clientilor: \n==============================================\n");
   	for(int i = 0; i < client_nr; i++){
   		printf("%s %s %d %d %s %.2f\n", client_vector[i].nume, client_vector[i].prenume, client_vector[i].numar_card, client_vector[i].pin, client_vector[i].parola_secreta, client_vector[i].sold);
   	}
   	printf("==============================================\n\n");
    

    //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
     
    tcpsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpsockfd < 0) 
        error("ERROR opening socket");
    udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpsockfd < 0) 
        error("ERROR opening socket");
     
    portno = atoi(argv[1]);

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
    serv_addr.sin_port = htons(portno);
    
    if (bind(tcpsockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
        error("ERROR on binding TCP");
    if (bind(udpsockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
        error("ERROR on binding UDP");
    
    listen(tcpsockfd, MAX_CLIENTS);

    //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
    FD_SET(tcpsockfd, &read_fds);
    FD_SET(udpsockfd, &read_fds);
    FD_SET(0, &read_fds);

    if(tcpsockfd >= udpsockfd)
    	fdmax = tcpsockfd;
    else
    	fdmax = udpsockfd;

    // main loop
	while (1) {
		if(stop)
			break;
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("ERROR in select");
	
		for(i = 0; i <= fdmax && !stop; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				

				if(i == 0){
					//citesc de la tastatura
					memset(cmd, 0, BUFLEN);
	    			fgets(cmd, BUFLEN-1, stdin);
	    			if(strcmp(cmd, "quit\n") == 0){
	    				//printf("%d\n", fdmax);
	    				stop = 1;
	    				for(int k = 0; k <= fdmax; k++){
	    					if (FD_ISSET(k, &read_fds) && k != 0 && k != tcpsockfd && k != udpsockfd){
	    						n = send(k, "Conexiune inchisa de server\n", strlen("Conexiune inchisa de server\n"), 0);
		    					if (n < 0) 
		        					error("ERROR writing to socket");
	    						close(k);
	    					}
	    				}
	    			}

				} else if(i == udpsockfd){
					memset(buffer, 0, BUFLEN);
					if( (n = recvfrom(udpsockfd, buffer, BUFLEN, 0, (struct sockaddr*) &serv_addr, (socklen_t *) &servsize)) <= 0 ) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("[CLIENT, SOCKET %d] a parasit sesiunea de lucru\n", i);
						} else {
							error("ERROR in receive from UDP socket");
						}
						close(i);
						FD_CLR(i, &read_fds);
					} else { //recvfrom intoarce > 0
						printf ("[CLIENT, SOCKET_UDP %d] -> cerere: %s\n", i, buffer);
						memset(return_buffer, 0, BUFLEN);
						strcat(return_buffer, "UNLOCK> ");
						strcpy(aux_buffer, buffer);
						strcpy(unlock_buffer, return_buffer);
						cmd_token = strtok(buffer, " ");
						if(cmd_token != NULL){

							if(correct_unlock == 0){
								if(strcmp(cmd_token, "unlock") == 0){
									cmd_token = strtok(NULL, " ");
									if(cmd_token != NULL){
										card_to_unlock = atoi(cmd_token);

										for(int p = 0; p < client_nr; p++){
											if(client_vector[p].numar_card == card_to_unlock){
												unlock_flag = 1;
												unlock_index = p;
											}
										}

										if(unlock_flag == 0){
											strcat(return_buffer, "-4 : Numar card inexistent\n");
											n = sendto(i, return_buffer, BUFLEN, 0, (struct sockaddr*) &serv_addr, servsize);
					    					if (n < 0) 
					        					error("ERROR writing to socket");
										} else if(client_vector[unlock_index].blocat == 0){
											strcat(return_buffer, "-6 : Operatie esuata\n");
											n = sendto(i, return_buffer, BUFLEN, 0, (struct sockaddr*) &serv_addr, servsize);
					    					if (n < 0) 
					        					error("ERROR writing to socket");
										} else{
											correct_unlock = 1;
											strcat(return_buffer, "Trimite parola secreta\n");
											n = sendto(i, return_buffer, BUFLEN, 0, (struct sockaddr*) &serv_addr, servsize);
					    					if (n < 0) 
					        					error("ERROR writing to socket");
										}
									}
								}
							} else {
								memset(pass_string, 0, BUFLEN);
								strcpy(pass_string, aux_buffer);
								pass_token = strtok(pass_string, " ");
								if(pass_token != NULL){
									card_to_unlock = atoi(pass_token);
									pass_token = strtok(NULL, " ");
									if(pass_token != NULL){
										for(int l = 0; l < client_nr; l++){
											if(client_vector[l].numar_card == card_to_unlock){
												strcpy(pass_aux, client_vector[l].parola_secreta);
												strcat(pass_aux, "\n");
												if(strcmp(pass_aux, pass_token) == 0){
													client_vector[l].blocat = 0;
													client_vector[l].incercari_logare = 0;
													strcat(unlock_buffer, "Card deblocat\n");
													n = sendto(i, unlock_buffer, BUFLEN, 0, (struct sockaddr*) &serv_addr, servsize);
							    					if (n < 0) 
							        					error("ERROR writing to socket");
												} else {
													strcat(unlock_buffer, "-7 : Deblocare esuata\n");
													n = sendto(i, unlock_buffer, BUFLEN, 0, (struct sockaddr*) &serv_addr, servsize);
							    					if (n < 0) 
							        					error("ERROR writing to socket");
												}
											}
										}
									}
									unlock_flag = 0;
									correct_unlock = 0;
								}
							}
						}
					}

				} else if (i == tcpsockfd) {
					// a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(tcpsockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					} 
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
				} else {
					// am primit date pe unul din socketii cu care vorbesc cu clientii
					//actiunea serverului: recv()
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("[CLIENT, SOCKET %d] a parasit sesiunea de lucru\n", i);
						} else {
							error("ERROR in recv");
						}
						close(i); 
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care s-a constatat o eroare
					} else { //recv intoarce >0
						printf ("[CLIENT, SOCKET %d] -> cerere: %s\n", i, buffer);
						memset(return_buffer, 0, BUFLEN);
						strcat(return_buffer, "IBANK> ");
						strcpy(aux_buffer, buffer);
						cmd_token = strtok(buffer, " ");


						if(strcmp(aux_buffer, "listsold\n") == 0){
							for(int j = 0; j < client_nr; j++){
								if(client_vector[j].numar_card == nr_card_recv){
									sprintf(sold_str, "%.2f", client_vector[j].sold);
								}
							}
							strcat(return_buffer, sold_str);
							n = send(i, return_buffer, strlen(return_buffer), 0);
	    					if (n < 0) 
	        					error("ERROR writing to socket");
						}

						if(strcmp(aux_buffer, "logout\n") == 0){
							for(int j = 0; j < client_nr; j++){
								if(client_vector[j].numar_card == nr_card_recv){
									client_vector[j].logat = 0;
								}
							}
							strcat(return_buffer, "Clientul a fost deconectat\n");
							n = send(i, return_buffer, strlen(return_buffer), 0);
	    					if (n < 0) 
	        					error("ERROR writing to socket");
						}

						if(strcmp(aux_buffer, "quit\n") == 0){
							for(int j = 0; j < client_nr; j++){
								if(client_vector[j].numar_card == nr_card_recv){
									client_vector[j].logat = 0;
								}
							}
						}


						if(cmd_token != NULL){
							if(strcmp(cmd_token, "login") == 0){
								cmd_token = strtok(NULL, " ");
								if(cmd_token != NULL){
									nr_card_recv = atoi(cmd_token);
									cmd_token = strtok(NULL, " ");
									if(cmd_token != NULL){
										pin_recv = atoi(cmd_token);
										for(int j = 0; j < client_nr; j++){
											if(client_vector[j].numar_card == nr_card_recv){
												flag_nr_card = 1;
												indice_client = j;
											}
										}
										if(flag_nr_card == 0){
											strcat(return_buffer, "-4 : Numar card inexistent\n");
											n = send(i, return_buffer, strlen(return_buffer), 0);
					    					if (n < 0) 
					        					error("ERROR writing to socket");
					        			} else {
											if(client_vector[indice_client].logat == 0){
												if(client_vector[indice_client].incercari_logare == 3){
													strcat(return_buffer, "-5 : Card blocat\n");
													n = send(i, return_buffer, strlen(return_buffer), 0);
							    					if (n < 0) 
							        					error("ERROR writing to socket");
												} else {
													correct_nr_card = nr_card_recv;
													if(client_vector[indice_client].pin == pin_recv){
														client_vector[indice_client].incercari_logare = 0;
														client_vector[indice_client].logat = 1;
														strcat(return_buffer, "Welcome ");
														strcat(return_buffer, client_vector[indice_client].nume);
														strcat(return_buffer, " ");
														strcat(return_buffer, client_vector[indice_client].prenume);
														strcat(return_buffer, " ");
														sprintf(aux_nr_card, "%d", client_vector[indice_client].numar_card);
														strcat(return_buffer, aux_nr_card);
														strcat(return_buffer, "\n");
														n = send(i, return_buffer, strlen(return_buffer), 0);
							    						if (n < 0) 
							        						error("ERROR writing to socket");
													} else {
														client_vector[indice_client].incercari_logare++;
														if(client_vector[indice_client].incercari_logare == 3){
															client_vector[indice_client].blocat = 1;
															strcat(return_buffer, "-5 : Card blocat\n");
															n = send(i, return_buffer, strlen(return_buffer), 0);
									    					if (n < 0) 
									        					error("ERROR writing to socket");
									        			} else {
									        				strcat(return_buffer, "-3 : Pin gresit\n");
															n = send(i, return_buffer, strlen(return_buffer), 0);
									    					if (n < 0) 
									        					error("ERROR writing to socket");
									        			}
													}
												}
											} else {
												strcat(return_buffer, "-2 : Sesiune deja deschisa\n");
												n = send(i, return_buffer, strlen(return_buffer), 0);
						    					if (n < 0) 
						        					error("ERROR writing to socket");
											}
										}

										
									}
								}	
								if(nr_card_recv != correct_nr_card && correct_nr_card != 0){
									for(int j = 0; j < client_nr; j++){
										if(client_vector[j].numar_card == correct_nr_card){
											if(client_vector[j].blocat != 1)
												client_vector[j].incercari_logare = 0;
										}
									}
								}
								flag_nr_card = 0;
							}


							if(correct_transaction == 0){
								if(strcmp(cmd_token, "transfer") == 0){

									cmd_token = strtok(NULL, " ");

									if(cmd_token != NULL){
										transf_dest = atoi(cmd_token);
										cmd_token = strtok(NULL, " ");
										if(cmd_token != NULL){
											sscanf(cmd_token, "%lf", &transf_amount);
											strcpy(amount, cmd_token);
										}
									}

									printf("%d %0.2f %d\n", transf_dest, transf_amount, nr_card_recv);
									for(int j = 0; j < client_nr; j++){
										if(client_vector[j].numar_card == transf_dest){
											transf_flag = 1;
											transf_index_dest = j;
										}
										if(client_vector[j].numar_card == nr_card_recv){
											transf_index_src = j;
										}

									}
									if(transf_flag == 0){
										strcat(return_buffer, "-4 : Numar card inexistent\n");
										n = send(i, return_buffer, strlen(return_buffer), 0);
				    					if (n < 0) 
				        					error("ERROR writing to socket");
						        	} else if(transf_amount > client_vector[transf_index_src].sold){
						        		strcat(return_buffer, "-8 : Fonduri insuficiente\n");
										n = send(i, return_buffer, strlen(return_buffer), 0);
				    					if (n < 0) 
				        					error("ERROR writing to socket");
						        	} else {
						        		correct_transaction = 1;

						        		strcat(return_buffer, "Transfer ");
						        		strncat(return_buffer, amount, strlen(amount) - 1);
						        		strcat(return_buffer, " catre ");
						        		strcat(return_buffer, client_vector[transf_index_dest].nume);
						        		strcat(return_buffer, " ");
						        		strcat(return_buffer, client_vector[transf_index_dest].prenume);
						        		strcat(return_buffer, "? [y/n]\n");

										n = send(i, return_buffer, strlen(return_buffer), 0);
				    					if (n < 0) 
				        					error("ERROR writing to socket");
						        	}			
								}
							} else {
								if(cmd_token[0] == 'y'){
									client_vector[transf_index_src].sold -= transf_amount;
									client_vector[transf_index_dest].sold += transf_amount;
									strcat(return_buffer, "Transfer realizat cu succes\n");
									n = send(i, return_buffer, strlen(return_buffer), 0);
			    					if (n < 0) 
			        					error("ERROR writing to socket");
				        			transf_flag = 0;
				        			correct_transaction = 0;
								} else if (cmd_token[0] != 'y'){
									strcat(return_buffer, "-9 : Operatie anulata\n");
									n = send(i, return_buffer, strlen(return_buffer), 0);
			    					if (n < 0) 
			        					error("ERROR writing to socket");
				        			transf_flag = 0;
				        			correct_transaction = 0;
								}
							}

						}

					}
				} 
			}
		}
     }


     close(tcpsockfd);
     close(udpsockfd);
   
     return 0; 
}


