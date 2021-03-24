#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      
#include <sys/types.h>  
#include <sys/wait.h>  
#include <string.h>
#include "knn.h"

/*****************************************************************************/
/* Do not add anything outside the main function here. Any core logic other  */
/*   than what is described below should go in `knn.c`. You've been warned!  */
/*****************************************************************************/

/**
 * main() takes in the following command line arguments.
 *   -K <num>:  K value for kNN (default is 1)
 *   -d <distance metric>: a string for the distance function to use
 *          euclidean or cosine (or initial substring such as "eucl", or "cos")
 *   -p <num_procs>: The number of processes to use to test images
 *   -v : If this argument is provided, then print additional debugging information
 *        (You are welcome to add print statements that only print with the verbose
 *         option.  We will not be running tests with -v )
 *   training_data: A binary file containing training image / label data
 *   testing_data: A binary file containing testing image / label data
 *   (Note that the first three "option" arguments (-K <num>, -d <distance metric>,
 *   and -p <num_procs>) may appear in any order, but the two dataset files must
 *   be the last two arguments.
 * 
 * You need to do the following:
 *   - Parse the command line arguments, call `load_dataset()` appropriately.
 *   - Create the pipes to communicate to and from children
 *   - Fork and create children, close ends of pipes as needed
 *   - All child processes should call `child_handler()`, and exit after.
 *   - Parent distributes the test dataset among children by writing:
 *        (1) start_idx: The index of the image the child should start at
 *        (2)    N:      Number of images to process (starting at start_idx)
 *     Each child should get at most N = ceil(test_set_size / num_procs) images
 *      (The last child might get fewer if the numbers don't divide perfectly)
 *   - Parent waits for children to exit, reads results through pipes and keeps
 *      the total sum.
 *   - Print out (only) one integer to stdout representing the number of test 
 *      images that were correctly classified by all children.
 *   - Free all the data allocated and exit.
 *   - Handle all relevant errors, exiting as appropriate and printing error message to stderr
 */
void usage(char *name) {
    fprintf(stderr, "Usage: %s -v -K <num> -d <distance metric> -p <num_procs> training_list testing_list\n", name);
}

int main(int argc, char *argv[]) {

    int opt;
    int K = 1;             // default value for K
    char *dist_metric = "euclidean"; // default distant metric
    int num_procs = 1;     // default number of children to create
    int verbose = 0;       // if verbose is 1, print extra debugging statements
    int total_correct = 0; // Number of correct predictions

    while((opt = getopt(argc, argv, "vK:d:p:")) != -1) {
        switch(opt) {
        case 'v':
            verbose = 1;
            break;
        case 'K':
            K = atoi(optarg);
            break;
        case 'd':
            dist_metric = optarg;
            break;
        case 'p':
            num_procs = atoi(optarg);
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if(optind >= argc) {
        fprintf(stderr, "Expecting training images file and test images file\n");
        exit(1);
    } 

    char *training_file = argv[optind];
    optind++;
    char *testing_file = argv[optind];
  
    // Set which distance function to use
    /* You can use the following string comparison which will allow
     * prefix substrings to match:
     * 
     * If the condition below is true then the string matches
     * if (strncmp(dist_metric, "euclidean", strlen(dist_metric)) == 0){
     *      //found a match
     * }
     */ 
  
    // TODO
    double (*fptr)(Image *, Image *) = NULL;
    if (strncmp(dist_metric, "euclidean", strlen(dist_metric)) == 0) {
        fptr = distance_euclidean;
    }
    if (strncmp(dist_metric, "cosine", strlen(dist_metric)) == 0) {
        fptr = distance_cosine;
    }

    // Load data sets
    if(verbose) {
        fprintf(stderr,"- Loading datasets...\n");
    }
    
    Dataset *training = load_dataset(training_file);
    if ( training == NULL ) {
        fprintf(stderr, "The data set in %s could not be loaded\n", training_file);
        exit(1);
    }

    Dataset *testing = load_dataset(testing_file);
    if ( testing == NULL ) {
        fprintf(stderr, "The data set in %s could not be loaded\n", testing_file);
        exit(1);
    }

    // Create the pipes and child processes who will then call child_handler
    if(verbose) {
        printf("- Creating children ...\n");
    }

    // TODO
    
    //int num_pipes = num_procs * 2;
    int parent_to_child[num_procs][2]; // create two pipes for each child process
    int child_to_parent[num_procs][2];
    //int fd[num_pipes][2]; 
    //printf("num_pipes = %d\n", num_pipes);

    int child_num = testing->num_items / num_procs; // divide total number of items by number of children/processes
    printf("child_num = %d\n", child_num);
    int start_idx = 0; // first start with image at index 0

    for (int i = 0; i < num_procs; i++) {
        if (pipe(parent_to_child[i]) == -1) { // first pipe
            if (verbose) {
                perror("pipe");
            }
            exit(1);
        }
        printf("piped parent_to_child[i]\n");
        if (pipe(child_to_parent[i]) == -1) { // second pipe
            if (verbose) {
                perror("pipe");
            }
            exit(1);
        }
        printf("piped child_to_parent[i]\n");

        int result = fork();
        // Distribute the work to the children by writing their starting index and
        // the number of test images to process to their write pipe

        // TODO
        if (result < 0) {
            if (verbose) {
                perror("fork");
            }
            exit(1);
        }
        else if (result > 0) { // parent
            //for (int j = 0; j < num_procs; j++) {
                // write start_idx and N to child's pipe (p_in)
            if (close(parent_to_child[i][0]) == -1) { // parent won't be reading from pipe -> but the child needs to read from this pipe later in child_handler
                if (verbose) {
                    fprintf(stderr, "Close 1 error\n");
                }
                exit(1);
            }
            if (close(child_to_parent[i][1]) == -1) {
                if (verbose) {
                    fprintf(stderr, "Close 1a error\n");
                }
                exit(1);
            }
            if (write(parent_to_child[i][1], &start_idx, sizeof(int)) != sizeof(int)) { // write start_idx to pipe
                if (verbose) {
                    fprintf(stderr, "Write 1 error\n");
                }
                exit(1);
            }
            if (write(parent_to_child[i][1], &child_num, sizeof(int)) != sizeof(int)) { // write N to pipe
                if (verbose) {
                    fprintf(stderr, "Write 2 error\n");
                }
                exit(1);
            }
            printf("wrote startidx = %d, childnum = %d to pipe\n", start_idx, child_num);
            if (close(parent_to_child[i][1]) == -1) { // close write end of pipe
                if (verbose) {
                    fprintf(stderr, "Close 2  error\n");
                }
                exit(1);
            }
            printf("closed pipe parent_to_child[i][1]\n");
            if ((start_idx + child_num) <= testing->num_items) {
                start_idx += child_num;
                printf("new start_idx = %d, now waiting for child\n", start_idx);
            }
		        //exit(0); add a break here?
            //}
        } 
        else if (result == 0) { // child
            if (close(parent_to_child[i][1]) == -1) { // close writing end of first pipe
                if (verbose) {
                    fprintf(stderr, "Close child 1 error\n");
                }
                exit(1);
            }
            for (int x = 0; x < i; x++) { // close all previously forked children pipes
               if (close(child_to_parent[x][0]) == -1) {
                   if (verbose) {
                       fprintf(stderr, "Close child 2 error\n");
                   }
               exit(1);
               }
            if (close(child_to_parent[i][0]) == -1) {
                if (verbose) {
                    fprintf(stderr, "Close child 3 error\n");
                }
                exit(1);
            }
            printf("pid %d closed pipes\n", getpid());
            printf("pid %d child start_idx = %d, child num = %d\n", getpid(), start_idx, child_num);
            //if (close(fd[i+1][0]) == -1) { // close reading end of second pipe
            //    if (verbose) {
            //        fprintf(stderr, "Close child error\n");
            //    }
            //    exit(1);
            //}
            child_handler(training, testing, K, fptr, parent_to_child[i][0], child_to_parent[i][1]); 
            printf("pid %d child handler ended\n", getpid());
            // p_in = first pipe's reading end, p_out = second pipe's writing end
            exit(0); // don't fork children on next loop iteration
        } // at this point the for loop has finished, each child's correct predictions has been written to fd[i+1];
    
        // Wait for children to finish 
        //int status;
        //int y = wait(&status);
        //if(verbose) {
        //    printf("- Waiting for children...\n");
        //}
        //if (y == -1) {
        //    if (verbose) {
        //        fprintf(stderr, "Wait error");
        //    }
        //}

        // When the children have finised, read their results from their pipe
        //for (int y = 0; y < num_pipes; y+=2) {
        printf("child terminated\n");
        //if (WIFEXITED(status)) {
        int temp_correct;
        if (read(child_to_parent[i][0], &temp_correct, sizeof(int)) != sizeof(int)) {
            fprintf(stderr, "read 1 issue\n");
            //perror("read");
            exit(1);
        }
        total_correct += temp_correct;
        //}  
        // if (close(parent_to_child[i][0]) == -1) { // close child_handler pipes
        //     if (verbose) {
        //         fprintf(stderr, "Close child 4 error\n");
        //     }
        //     exit(1);
        // }
        if (close(child_to_parent[i][1]) == -1) {
            if (verbose) {
                fprintf(stderr, "Close child 5 error\n");
            }
            exit(1);
        }
        if (close(child_to_parent[i][0]) == -1) {
            if (verbose) {
                fprintf(stderr, "close child 6 error\n");
            }
            exit(1);
        }
    }
    //}
    // This is the only print statement that can occur outside the verbose check
    printf("%d\n", total_correct);

    // Clean up any memory, open files, or open pipes
    // TODO
    free(testing);
    free(training);

    return 0;
}
