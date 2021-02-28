/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC209H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Mustafa Quraish, Bianca Schroeder, Karen Reid
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2021 Karen Reid
 */

#include "dectree.h"

/**
 * Load the binary file, filename into a Dataset and return a pointer to 
 * the Dataset. The binary file format is as follows:
 *
 *     -   4 bytes : `N`: Number of images / labels in the file
 *     -   1 byte  : Image 1 label
 *     - NUM_PIXELS bytes : Image 1 data (WIDTHxWIDTH)
 *          ...
 *     -   1 byte  : Image N label
 *     - NUM_PIXELS bytes : Image N data (WIDTHxWIDTH)
 *
 * You can set the `sx` and `sy` values for all the images to WIDTH. 
 * Use the NUM_PIXELS and WIDTH constants defined in dectree.h
 */
Dataset *load_dataset(const char *filename) {
    // TODO: Allocate data, read image data / labels, return
    FILE *data_file;

    data_file = fopen(filename, "rb");
    if (data_file == NULL) {
        perror("fopen");
        exit(1);
    }

    Dataset *d = malloc(sizeof(Dataset)); //sizeof(Dataset) = 24 NEED TO INITIALIZE the Dataset first
    if (d == NULL) {
        perror("malloc");
        exit(1);
    }
    int head = fread(&d->num_items, sizeof(int), 1, data_file); // reading num of images/labels in data_file
    if (head != 1) {
        fprintf(stderr, "reading num_images improperly!\n");
        exit(1);
    }

    int num_items = d->num_items;
    d->images = malloc(num_items * sizeof(Image));
    if (d->images == NULL) {
        perror("malloc");
        exit(1);
    }

    d->labels = malloc(num_items * sizeof(unsigned char));
    if (d->labels == NULL) {
        perror("malloc");
        exit(1);
    }

    for (int i = 0; i < num_items; i++) {
        int label = fread(&d->labels[i], sizeof(unsigned char), 1, data_file); // read image's label into Dataset's array of labels -> SEG FAULT
        if (label != 1) {
            fprintf(stderr, "label read improperly!\n");
            exit(1);
        }
        Image* img = malloc(sizeof(Image));
        if (img == NULL) {
            perror("malloc");
            exit(1);
        }
        img->sx = WIDTH;
        img->sy = WIDTH;
        int img_data = fread(&img->data, sizeof(char), NUM_PIXELS, data_file); // read image's data into an Image struct
        if (img_data != NUM_PIXELS) {
            fprintf(stderr, "image data read improperly!\n");
            exit(1);
        }
        d->images[i] = *img;
        free(img);
    }

    int err = fclose(data_file);
    if (err != 0) {
        perror("fclose");
        exit(1);
    }

    return d;
}

/**
 * Compute and return the Gini impurity of M images at a given pixel
 * The M images to analyze are identified by the indices array. The M
 * elements of the indices array are indices into data.
 * This is the objective function that you will use to identify the best 
 * pixel on which to split the dataset when building the decision tree.
 *
 * Note that the gini_impurity implemented here can evaluate to NAN 
 * (Not A Number) and will return that value. Your implementation of the 
 * decision trees should ensure that a pixel whose gini_impurity evaluates 
 * to NAN is not used to split the data.  (see find_best_split)
 * 
 * DO NOT CHANGE THIS FUNCTION; It is already implemented for you.
 */
double gini_impurity(Dataset *data, int M, int *indices, int pixel) {
    int a_freq[10] = {0}, a_count = 0;
    int b_freq[10] = {0}, b_count = 0;

    for (int i = 0; i < M; i++) {
        int img_idx = indices[i];

        // The pixels are always either 0 or 255, but using < 128 for generality.
        if (data->images[img_idx].data[pixel] < 128) {
            a_freq[data->labels[img_idx]]++;
            a_count++;
        } else {
            b_freq[data->labels[img_idx]]++;
            b_count++;
        }
    }

    double a_gini = 0, b_gini = 0;
    for (int i = 0; i < 10; i++) {
        double a_i = ((double)a_freq[i]) / ((double)a_count);
        double b_i = ((double)b_freq[i]) / ((double)b_count);
        a_gini += a_i * (1 - a_i);
        b_gini += b_i * (1 - b_i);
    }

    // Weighted average of gini impurity of children
    return (a_gini * a_count + b_gini * b_count) / M;
}

/**
 * Given a subset of M images and the array of their corresponding indices, 
 * find and use the last two parameters (label and freq) to store the most
 * frequent label in the set and its frequency.
 *
 * - The most frequent label (between 0 and 9) will be stored in `*label`
 * - The frequency of this label within the subset will be stored in `*freq`
 * 
 * If multiple labels have the same maximal frequency, return the smallest one.
 */
void get_most_frequent(Dataset *data, int M, int *indices, int *label, int *freq) {
    // TODO: Set the correct values and return
    int i = 0;
    int most_freq_label = 0;
    int max_freq = 0;
    while (i < M) { // for each image in the Dataset
        int count = 0;
        for (int j = 0; j < M; j++) {
            if (data->labels[indices[i]] == data->labels[indices[j]]) {
                count++;
            }
        } 
        if (count > *freq) { // if current label occurs more frequently then replace it
            most_freq_label = (int)data->labels[indices[i]];
            max_freq = count;
        }
        if (count == *freq) { // if it has the same frequency, check which one's smaller 
            if (data->labels[i] < *label) {
                most_freq_label = (int)data->labels[indices[i]];
            }
        }
        i++;    
    }
    *label = most_freq_label;
    *freq = max_freq;
    return;
}

/**
 * Given a subset of M images as defined by their indices, find and return
 * the best pixel to split the data. The best pixel is the one which
 * has the minimum Gini impurity as computed by `gini_impurity()` and 
 * is not NAN. (See handout for more information)
 * 
 * The return value will be a number between 0-783 (inclusive), representing
 *  the pixel the M images should be split based on.
 * 
 * If multiple pixels have the same minimal Gini impurity, return the smallest.
 */
int find_best_split(Dataset *data, int M, int *indices) {
    // TODO: Return the correct pixel
    return 0;
}

/**
 * Create the Decision tree. In each recursive call, we consider the subset of the
 * dataset that correspond to the new node. To represent the subset, we pass 
 * an array of indices of these images in the subset of the dataset, along with 
 * its length M. Be careful to allocate this indices array for any recursive 
 * calls made, and free it when you no longer need the array. In this function,
 * you need to:
 *
 *    - Compute ratio of most frequent image in indices, do not split if the
 *      ration is greater than THRESHOLD_RATIO
 *    - Find the best pixel to split on using `find_best_split`
 *    - Split the data based on whether pixel is less than 128, allocate 
 *      arrays of indices of training images and populate them with the 
 *      subset of indices from M that correspond to which side of the split
 *      they are on
 *    - Allocate a new node, set the correct values and return
 *       - If it is a leaf node set `classification`, and both children = NULL.
 *       - Otherwise, set `pixel` and `left`/`right` nodes 
 *         (using build_subtree recursively). 
 */
DTNode *build_subtree(Dataset *data, int M, int *indices) {
    // TODO: Construct and return the tree

    return NULL;
}

/**
 * This is the function exposed to the user. All you should do here is set
 * up the `indices` array correctly for the entire dataset and call 
 * `build_subtree()` with the correct parameters.
 */
DTNode *build_dec_tree(Dataset *data) {
    // TODO: Set up `indices` array, call `build_subtree` and return the tree.
    // HINT: Make sure you free any data that is not needed anymore

    return NULL;
}

/**
 * Given a decision tree and an image to classify, return the predicted label.
 */
int dec_tree_classify(DTNode *root, Image *img) {
    // TODO: Return the correct label

    return -1;
}

/**
 * This function frees the Decision tree.
 */
void free_dec_tree(DTNode *node) {
    // TODO: Free the decision tree

    return;
}

/**
 * Free all the allocated memory for the dataset
 */
void free_dataset(Dataset *data) {
    // TODO: Free dataset (Same as A1)

    return;
}
