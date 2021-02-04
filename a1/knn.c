#include <stdio.h>
#include <math.h>    // Need this for sqrt()
#include <stdlib.h>
#include <string.h>

#include "knn.h"

/* Print the image to standard output in the pgmformat.  
 * (Use diff -w to compare the printed output to the original image)
 */
void print_image(unsigned char *image) {
    printf("P2\n %d %d\n 255\n", WIDTH, HEIGHT);
    for (int i = 0; i < NUM_PIXELS; i++) {
        printf("%d ", image[i]);
        if ((i + 1) % WIDTH == 0) {
            printf("\n");
        }
    }
}

/* Return the label from an image filename.
 * The image filenames all have the same format:
 *    <image number>-<label>.pgm
 * The label in the file name is the digit that the image represents
 */
unsigned char get_label(char *filename) {
    // Find the dash in the filename
    char *dash_char = strstr(filename, "-");
    // Convert the number after the dash into a integer
    return (char) atoi(dash_char + 1);
}

/* ******************************************************************
 * Complete the remaining functions below
 * ******************************************************************/


/** Read a pgm image from filename, storing its pixels
 * in the array img.
 * It is recommended to use fscanf to read the values from the file. First, 
 * consume the header information.  You should declare two temporary variables
 * to hold the width and height fields. This allows you to use the format string
 * "P2 %d %d 255 "
 * 
 * To read in each pixel value, use the format string "%hhu " which will store
 * the number it reads in an an unsigned character.
 * (Note that the img array is a 1D array of length WIDTH*HEIGHT.)
 */
void load_image(char *filename, unsigned char *img) {
    // Open corresponding image file, read in header (which we will discard)
    FILE *f2 = fopen(filename, "r");
    if (f2 == NULL) {
        perror("fopen");
        exit(1);
    }
	int width, height;
    fscanf(f2, "P2 %d %d 255 ", &width, &height); // reading header

    unsigned char pixel;
    int i = 0;
    while (fscanf(f2, "%hhu ", &pixel) == 1) {
        img[i] = pixel;
        i++;
    }
    
    fclose(f2);
}


/**
 * Load a full dataset into a 2D array called dataset.
 *
 * The image files to load are given in filename where
 * each image file name is on a separate line.
 * 
 * For each image i:
 *  - read the pixels into row i (using load_image)
 *  - set the image label in labels[i] (using get_label)
 * 
 * Return number of images read.
 */
int load_dataset(char *filename, 
                 unsigned char dataset[MAX_SIZE][NUM_PIXELS],
                 unsigned char *labels) {
    // We expect the file to hold up to MAX_SIZE number of file names
    FILE *f1 = fopen(filename, "r");
    if (f1 == NULL) {
        perror("fopen");
        exit(1);
    }
    
    char image_name[MAX_NAME];
    int i = 0;
    while (fscanf(f1, "%s", image_name) == 1) {
        load_image(image_name, dataset[i]);
        labels[i] = get_label(image_name);
        i++;
    }
    fclose(f1); 
    return i;
}

/** 
 * Return the euclidean distance between the image pixels in the image
 * a and b.  (See handout for the euclidean distance function)
 */
double distance(unsigned char *a, unsigned char *b) {
    double sum = 0.0;
    for (int i = 0; i < NUM_PIXELS; i++) {
        //printf("%d", a[i]);
        sum += pow((a[i] - b[i]), 2);
    }
    return sqrt(sum);
}

/**
 * Return index of maximum value in array
 */
int max(double *arr, int size) {
    int curr_max = 0;
    for (int i = 1; i < size; i++) {
        if (arr[i] > arr[curr_max]) {
            curr_max = i;
        }
    }
    return curr_max;
}

/**
 * Return the most frequent label of the K most similar images to "input"
 * in the dataset
 * Parameters:
 *  - input - an array of NUM_PIXELS unsigned chars containing the image to test
 *  - K - an int that determines how many training images are in the 
 *        K-most-similar set
 *  - dataset - a 2D array containing the set of training images.
 *  - labels - the array of labels that correspond to the training dataset
 *  - training_size - the number of images that are actually stored in dataset
 * 
 * Steps
 *   (1) Find the K images in dataset that have the smallest distance to input
 *         When evaluating an image to decide whether it belongs in the set of 
 *         K closest images, it will only replace an image in the set if its
 *         distance to the test image is strictly less than all of the images in 
 *         the current K closest images.
 *   (2) Count the frequencies of the labels in the K images
 *   (3) Return the most frequent label of these K images
 *         In the case of a tie, return the smallest value digit.
 */ 

int knn_predict(unsigned char *input, int K,
                unsigned char dataset[MAX_SIZE][NUM_PIXELS],
                unsigned char *labels,
                int training_size) {

    //unsigned char closest_k[K];
    unsigned char closest_k_labels[K];
    double closest_k_distances[K];

    double curr_min = distance(input, dataset[0]);
    int curr_max = 0;

    for (int n = 0; n < K; n++) { // initializing first k images as the initial closest k images to have smth to compare to
        // closest_k[n] = dataset[n];
        closest_k_labels[n] = labels[n]; // -> 5, 0 (this is working)
        closest_k_distances[n] = distance(input, dataset[n]); // -> 2395, 2650 (not working? giving 91, then 90, is it because this isn't an array of doubles?)
        if (closest_k_distances[n] < curr_min) { // should not go satisfy either if at first 
            curr_min = closest_k_distances[n];
        }
        if (closest_k_distances[n] > closest_k_distances[curr_max]) { // should satisfy this for second, curr_max = 1
            curr_max = n;
        }
    }
    // DEBUG: printing first K labels and distances
    //for (int d = 0; d < K; d++) {
    //    printf("label at %d is: %d", d, (int)closest_k_labels[d]);
    //    printf("distance at %d is: %d", d, (int)closest_k_distances[d]);
    //}

    double diff;
    for (int i = K; i < training_size; i++) { // compare with the rest of training dataset from K onwards
        diff = distance(input, dataset[i]); // distance between test image and training image (2436 when i = 2)
        // printf("curr_min is %f, new diff is %f", curr_min, diff); // DEBUG
        if (diff < curr_min) {
            curr_min = diff;
            closest_k_distances[curr_max] = diff;
            closest_k_labels[curr_max] = labels[i];
            // closest_k[curr_max] = dataset[i]; 
            curr_max = max(closest_k_distances, K);
        }    
    }
    // DEBUG: printing new closest K labels and distances
    //for (int p = 0; p < K; p++) {
    //    printf("label at %d is: %d", p, (int)closest_k_labels[p]);
    //    printf("distance at %d is: %d", p, (int)closest_k_distances[p]);
    //}

    char freqs[K]; // counting frequencies of labels
    for (int x = 0; x < K; x++) {
        int reps = 1; // number of repetitions
        for (int y = x + 1; y < K; y++) {
            // DEBUG check what labels are being compared
            //printf("y is %d, x is %d", (int)closest_k_labels[y], (int)closest_k_labels[x]); 
            if (closest_k_labels[y] == closest_k_labels[x]) { 
                reps++;
            }
        }
        freqs[x] = reps;
    }
    // DEBUG: printing frequencies of labels:
    //for (int s = 0; s < K; s++) {
    //    printf("freq of %d is %d", (int)closest_k_labels[s], (int)freqs[s]);
    //}
    
    int most = 0; // index of most frequently occuring label
    for (int c = 1; c < K; c++) {
        //printf("label at %d is %d", c, (int)freqs[c]); // DEBUG check what label at freq
        //printf("current most is %d", most); // DEBUG check what curr most is
        if (freqs[c] > freqs[most]) {
            most = c;
        }
        if (freqs[c] == freqs[most]) {
            if (closest_k_labels[c] > closest_k_labels[most]) {
                most = c;
            }
        }
    }

    // DEBUG
    //printf("most occuring label is %d", most);

    return (int)closest_k_labels[most];
}
