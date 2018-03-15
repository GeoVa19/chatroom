#include "server.h"

//Προγραμματισμός Συστημάτων
//it21409, Βάσιος Γεώργιος
//Sources: https://en.wikipedia.org/wiki/Select_(Unix)
//Chat Room στη γλώσσα C
/*Έγιναν δοκιμές σε Ubuntu, Debian, και Solaris.
Στο ΛΣ Solaris για το compilation έγραψα: πχ. gcc server.c -o server -lnsl -lsocket
*/

//Η σταθερά PORT είναι στο αρχείο server.h.

/*Προτίμησα να κάνω χρήση της κλήσης συστήματος select(),
	γιατί με την fork() δε λειτουργούσε σωστά το chat room
	(τουλάχιστον έτσι όπως υλοποίησα στην αρχή το πρόγραμμα).
	
	Μετά από αναζήτηση στο Internet, η μία λύση ήταν η χρήση νημάτων
	και η άλλη η κλήση συστήματος select(). Η select() δε δημιουργεί 
	διεργασίες-παιδιά, παρά αναμένει έναν καινούργιο file descriptor.
*/

int main(void) {
    fd_set master; //master file descriptor
    fd_set read_fds; //file descriptors έτοιμοι για ανάγνωση
    int fdmax; //max file descriptors
	
    int sockfd; //socket του server
    int newsockfd; //socket του(-ων) client(-s)
    struct sockaddr_in client_addr; //για την accept
    int clilen; //για την accept

    char message[MESSAGE_SIZE];
	
	/*Μηχανισμός fault-tolerance: 
		Αφού η εφαρμογή είναι πρακτικά ένα δημόσιο chat room,
		για κάθε έναν client που συνδέεται στον server (είτε νέος
		client είτε client που έπεσε/έφυγε και συνδέθηκε ξανά*), ο server
		τού στέλνει το ιστορικό των μηνυμάτων (εφόσον, βέβαια, δεν έχει 
		γεμίσει	ο πίνακας history).
		
		*ο server δεν κρατάει λίστα με παλαιότερους clients, έτσι τους
		θεωρεί όλους νέους (το τόνισα παραπάνω απευθυνόμενος σε όσους πράγματι
		θα ξαναμπούν, ως πραγματικοί χρήστες της εφαρμογής!)
	*/
	char history[HISTORY_SIZE];
	
	int isFull;
	
    int nbytes;

    int optval = 1;
    int i, j, error_code;

    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; //stream socket
	hints.ai_protocol = IPPROTO_TCP; //TCP πρωτόκολλο
    hints.ai_flags = AI_PASSIVE;
	
	/*http://man7.org/linux/man-pages/man3/getaddrinfo.3.html*/
	if ((error_code = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        error(gai_strerror(error_code)); //τύπωσε μήνυμα σφάλματος σε μορφή κατανοητή από τον άνθρωπο
    }
    
    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		
        if (sockfd < 0) { 
            continue;
        }
        
        //για να αποφύγουμε το μήνυμα "address already in use"
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }

        break;
    }
	
    if (p == NULL)
        error("Server failed to bind.\n");

    freeaddrinfo(res); //ελευθέρωσε την μνήμη για την λίστα res.

    //listen
    if (listen(sockfd, MAX_CLIENTS) == -1) 
        error("ERROR: listen");
	
	//μηδένισε τα δύο fd_set
	FD_ZERO(&master);
    FD_ZERO(&read_fds);
	
    FD_SET(sockfd, &master);
	
	//ο sockfd είναι o fdmax για τώρα
    fdmax = sockfd;

	memset(message, 0, sizeof(message));
	memset(history, 0, sizeof(history));
	
	strcpy(history, message);
	
    while (TRUE) {
        read_fds = master;
		//http://man7.org/linux/man-pages/man2/select.2.html
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			close(sockfd);
            error("ERROR: select");
        }

        //διέτρεξε τις συνδέσεις
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { //κοίταξε αν ένας file descriptor είναι μέρος του read set
                if (i == sockfd) { //νέα σύνδεση
                    clilen = sizeof(client_addr);
                    newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &clilen);

                    if (newsockfd == -1) {
                        error("ERROR: accept");
                    } else {
                        FD_SET(newsockfd, &master); //πρόσθεσε το newsockfd στο master set
                        if (newsockfd > fdmax) { 
                            fdmax = newsockfd; //όρισε νέο fdmax
                        }
						
						isFull = is_full(history); //δες αν ο πίνακας history είναι γεμάτος
						if (isFull) 
							memset(history, 0, sizeof(history)); //άδειασε τον πίνακα history
						else {
							if (send(newsockfd, history, strlen(history), 0) == -1) { //στείλε όλα τα προηγούμενα μηνύματα
								close(sockfd);
								close(newsockfd);
								error("Failure to send old conversations.");
							}
						}
                    }
                } else {
                    //από υπάρχον client
					nbytes = recv(i, message, sizeof(message), 0);
                    if (nbytes <= 0) {
                        if (nbytes == 0) {
                            //ο client έφυγε
                            printf("Socket %d closed.\n", i);
                        } else {
							close(sockfd);
							close(newsockfd);
                            error("Failure to receive message!");
                        }
                        close(i);
                        FD_CLR(i, &master); //αφαίρεσέ τον από το master set
                    } else { //ο client έστειλε δεδομένα
						strcat(history, message); //ανανέωσε τον πίνακα history
                        
                        for (j = 0; j <= fdmax; j++) {
                            if (FD_ISSET(j, &master)) {
                                if (j != sockfd && j != i) { //μη στείλεις το μήνυμα στον server και στον αποστολέα
                                    if (send(j, message, nbytes, 0) == -1) {
										close(sockfd);
										close(newsockfd);
                                        error("Failure to send message!");
									}
                                }
                            }
                        }
						//αφού το μήνυμα στάλθηκε σε όλους, καθάρισέ το (για να μην ξαναγραφτεί στον πίνακα history).
						memset(message, 0, sizeof(message));
                    }
                }
            }
        }
    }
    
	close(sockfd);
	close(newsockfd);
	
    return EXIT_SUCCESS; //τερματισμός προγράμματος με επιτυχία
}

void error(const char *error_message) {
    fprintf(stderr, "%s\n", error_message);
    exit(EXIT_FAILURE); //τερματισμός προγράμματος εξαιτίας σφάλματος
}

int is_full(char *history) {
	if (history[HISTORY_SIZE - 1] != '\0') { 
		return TRUE; //είναι γεμάτος
	}
	
	return FALSE; //δεν είναι γεμάτος
}

