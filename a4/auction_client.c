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
    char *arg1 = NULL;
    char *arg2 = NULL;
    struct auction_data *auc_data = NULL; // array of auction_data structs
    int i = 0;
    char buf[BUF_SIZE];
    int sock_fd;

    // Get the user to provide a name.
    printf("Please enter a username: ");
    fflush(stdout);
    int num_read = read(STDIN_FILENO, name, BUF_SIZE);
    if(num_read <= 0){
        fprintf(stderr, "ERROR: name read from stdin failed\n");
        exit(1);
    }
    name[num_read] = '\0';
    print_menu();
    // TODO
    
    while(1) {
        print_prompt();
        
        // check what menu command was chosen
        int menu_read = read(STDIN_FILENO, menu, BUF_SIZE);
        if (menu_read <= 0) {
            fprintf(stderr, "ERROR: menu read from stdin failed\n");
            exit(1);
        }
        menu[menu_read] = '\0';
        int com = parse_command(menu, BUF_SIZE, arg1, arg2);

        if (com == ADD) {
            printf("arg1 = %s, arg2 = %s", arg1, arg2)
            char *a2;
            long port = strtol(arg2, &a2, 10); // convert arg2 to port int
            if (port == 0) {
                printf("invalid port number");
                break;
            }
            printf("checkpoint 1\n");
            
            sock_fd = add_server(arg1, port);
            printf("sock_fd = %d\n", sock_fd);
            
            if (write(sock_fd, name, num_read) != num_read) { // write username to server
                perror("client: write");
                close(sock_fd);
                exit(1);
            } 
            int max_fd = sock_fd;
            fd_set listen_fds;
            FD_ZERO(&listen_fds);
            FD_SET(sock_fd, &listen_fds);
            int num_ready = select(max_fd + 1, NULL, &listen_fds, NULL, NULL);
            if (num_ready == -1) { // put value of fds ready to write into write_fds
                perror("select");
                close(sock_fd);
                exit(1);
            }
            if (FD_ISSET(sock_fd, &listen_fds)) {
                int num_read = read(sock_fd, buf, BUF_SIZE);
                buf[num_read] = '\0';
                printf("read from server\n");
            }

            update_auction(buf, BUF_SIZE, auc_data, i); // record new connection to auctions array 
            printf("updated %s at fd %i to bid %s\n", auc_data[i].item, auc_data[i].sock_fd, buf);
            i++;
        }
        else if (com == SHOW) {
            print_auctions(auc_data, BUF_SIZE); // print current auction data to stdout
            fflush(stdout);
        }
        else if (com == BID) {
            char *ind;
            long which = strtol(arg1, &ind, 10);
            if (write(auc_data[which].sock_fd, arg2, menu_read) != menu_read) {
                perror("client: bid write");
                //exit(1);
            }
            // send bid to auction server stored at index in auction_data array
        }
        else if (com == QUIT) { // close open sockets and exit
            close(sock_fd);
            exit(0);
        }
    }
    return 0; // Shoud never get here
}
