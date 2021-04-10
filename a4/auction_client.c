#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define BUF_SIZE 128

#define MAX_AUCTIONS 5
#ifndef VERBOSE
#define VERBOSE 0
#endif

#define ADD 0
#define SHOW 1
#define BID 2
#define QUIT 3

/* Auction struct - this is different than the struct in the server program
 */
struct auction_data {
    int sock_fd;
    char item[BUF_SIZE];
    int current_bid;
};

/* Displays the command options available for the user.
 * The user will type these commands on stdin.
 */

void print_menu() {
    printf("The following operations are available:\n");
    printf("    show\n");
    printf("    add <server address> <port number>\n");
    printf("    bid <item index> <bid value>\n");
    printf("    quit\n");
}

/* Prompt the user for the next command 
 */
void print_prompt() {
    printf("Enter new command: ");
    fflush(stdout);
}


/* Unpack buf which contains the input entered by the user.
 * Return the command that is found as the first word in the line, or -1
 * for an invalid command.
 * If the command has arguments (add and bid), then copy these values to
 * arg1 and arg2.
 */
int parse_command(char *buf, int size, char *arg1, char *arg2) {
    int result = -1;
    char *ptr = NULL;
    if(strncmp(buf, "show", strlen("show")) == 0) {
        return SHOW;
    } else if(strncmp(buf, "quit", strlen("quit")) == 0) {
        return QUIT;
    } else if(strncmp(buf, "add", strlen("add")) == 0) {
        result = ADD;
    } else if(strncmp(buf, "bid", strlen("bid")) == 0) {
        result = BID;
    } 
    ptr = strtok(buf, " "); // first word in buf

    ptr = strtok(NULL, " "); // second word in buf
    if(ptr != NULL) {
        strncpy(arg1, ptr, BUF_SIZE);
    } else {
        return -1;
    }
    ptr = strtok(NULL, " "); // third word in buf
    if(ptr != NULL) {
        strncpy(arg2, ptr, BUF_SIZE);
        return result;
    } else {
        return -1;
    }
    return -1;
}

/* Connect to a server given a hostname and port number.
 * Return the socket for this server
 */
int add_server(char *hostname, int port) {
        // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }
    
    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    struct addrinfo *ai;
    
    /* this call declares memory and populates ailist */
    if(getaddrinfo(hostname, NULL, NULL, &ai) != 0) {
        close(sock_fd);
        return -1;
    }
    /* we only make use of the first element in the list */
    server.sin_addr = ((struct sockaddr_in *) ai->ai_addr)->sin_addr;

    // free the memory that was allocated by getaddrinfo for this list
    freeaddrinfo(ai);

    // Connect to the server.
    if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("client: connect");
        close(sock_fd);
        return -1;
    }
    if(VERBOSE){
        fprintf(stderr, "\nDebug: New server connected on socket %d.  Awaiting item\n", sock_fd);
    }
    return sock_fd;
}
/* ========================= Add helper functions below ========================
 * Please add helper functions below to make it easier for the TAs to find the 
 * work that you have done.  Helper functions that you need to complete are also
 * given below.
 */

/* Print to standard output information about the auction
 */
void print_auctions(struct auction_data *a, int size) {
    printf("Current Auctions:\n");

    /* TODO Print the auction data for each currently connected 
     * server.  Use the follosing format string:
     *     "(%d) %s bid = %d\n", index, item, current bid
     * The array may have some elements where the auction has closed and
     * should not be printed. <- **TO DO!!!**
     */
    if (size == 0) {
        printf("No current auctions\n");
    }
    else {
        for (int i = 0; i < size; i++) {
            printf("(%d) %s bid = %d\n", i, a[i].item, a[i].current_bid);
        }
    }
}

/* Process the input that was sent from the auction server at a[index].
 * If it is the first message from the server, then copy the item name
 * to the item field.  (Note that an item cannot have a space character in it.)
 */
void update_auction(char *buf, int size, struct auction_data *a, int index) {
    
    // TODO: Complete this function
    
    if (index == 0) {
        strncpy(a[index].item, buf, size); // copy item name to item field
    }
    char *ptr;
    long bid = strtol(buf, &ptr, 10);
    if (bid == 0) {
        fprintf(stderr, "ERROR malformed bid: %s", buf);
    }
    else {
        a[index].current_bid = bid;
        printf("\nNew bid for %s [%d] is %d\n", a[index].item, index, a[index].current_bid);
    }
    

    // fprintf(stderr, "ERROR malformed bid: %s", buf);
    // printf("\nNew bid for %s [%d] is %d (%d seconds left)\n",           );
}

int main(void) {

    char name[BUF_SIZE];
    // Declare and initialize necessary variables
    // TODO
    char menu[BUF_SIZE]; // menu option (change BUF_SIZE to smaller?)
    char arg1[BUF_SIZE];
    char arg2[BUF_SIZE];
    struct auction_data *auc_data = NULL; // array of auction_data structs
    int i = 0;
    char buf[BUF_SIZE];
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("server: socket");
        exit(1);
    }

    // Get the user to provide a name.
    printf("Please enter a username: ");
    fflush(stdout);
    int num_read = read(STDIN_FILENO, name, BUF_SIZE); // returns number of bytes of username
    if(num_read <= 0){
        fprintf(stderr, "ERROR: name read from stdin failed\n");
        exit(1);
    }
    name[num_read] = '\0';
    print_menu();
    // TODO
    // initialize set of file descs
    int max_fd = sock_fd;
    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(sock_fd, &all_fds);

    while(1) {
        fd_set listen_fds = all_fds;
        print_prompt();
        
        // select gives which fds are ready to be read from
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (nready == -1) {
            perror("server: select1");
            exit(1);
        }

        // if original socket, create a new connection
        if (FD_ISSET(STDIN_FILENO, &listen_fds)) { // if stdin is readable from, read the string and set 
            int menu_read = read(STDIN_FILENO, menu, BUF_SIZE);
            if (menu_read == 0) {
                break;
            }
        }

        int com = parse_command(menu, BUF_SIZE, arg1, arg2); // check what menu command was chosen
        
        if (com == ADD) {
            printf("arg1 = %s, arg2 = %s", arg1, arg2);
            char *a2;
            long port = strtol(arg2, &a2, 10); // convert arg2 to port int
            if (port == 0) {
                printf("invalid port number");
                break;
            }
            printf("checkpoint 1\n");
            
            sock_fd = add_server(arg1, port);
            printf("sock_fd = %d\n", sock_fd);
            fd_set write_fds = all_fds;
            int write_ready = select(max_fd + 1, NULL, &write_fds, NULL, NULL); // check which fds ready to write to
            if (write_ready == -1) {
                perror("server: select2");
                exit(1);
            }

            if (FD_ISSET(sock_fd, &write_fds)) {
                if (write(sock_fd, name, strlen(name) + 1) == -1) { // write username to server through sock_fd
                    perror("client: write");
                    close(sock_fd);
                    exit(1);
                } 
            }
        }
        else if (com == SHOW) {
            print_auctions(auc_data, BUF_SIZE); // print current auction data to stdout
            fflush(stdout);
        }
        else if (com == BID) {
            printf("arg1 = %s, arg2 = %s", arg1, arg2);
            char *ind;
            long which = strtol(arg1, &ind, 10);

            fd_set write_fds = all_fds;
            int write_ready = select(max_fd + 1, NULL, &write_fds, NULL, NULL); // check which fds ready to write to
            if (write_ready == -1) {
                perror("server: select3");
                exit(1);
            }

            if (FD_ISSET(sock_fd, &write_fds)) {
                if (write(auc_data[which].sock_fd, arg2, strlen(arg2) + 1) == -1) { // write bid (arg2) to auction server stored at index (which) in auction_data array
                    perror("client: bid write");
                    close(sock_fd);
                    exit(1);
                }
            }
        }
        else if (com == QUIT) { // close open sockets and exit
            close(sock_fd);
            exit(0);
        }

        for (int c = 0; c < MAX_AUCTIONS; c++) { // update each auction after each command
            int client_fd = auc_data[c].sock_fd;
            if (client_fd > -1 && FD_ISSET(client_fd, &listen_fds)) { // if client_fd is readable from
                char buf[BUF_SIZE];
                int r = read(client_fd, buf, BUF_SIZE);
                if (r == 0) {
                    break;
                }
                buf[r] = '\0';
                if (strncmp("Auction closed", buf, BUF_SIZE)) {
                    break;
                }
                else {
                    update_auction(buf, BUF_SIZE, auc_data, c);
                }
            }
        }
    }
    return 0; // Shoud never get here
}
