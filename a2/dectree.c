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
    FILE *data_file;
    data_file = fopen(filename, "rb");
    if (data_file == NULL) {
        perror("fopen");
        exit(1);
    }

    Dataset *d = malloc(sizeof(Dataset)); 
    if (d == NULL) {
        //perror("malloc");
        fprintf(stderr, "dataset malloc wrong\n");
        exit(1);
    }
    int head = fread(&d->num_items, sizeof(int), 1, data_file); // reading num of images/labels in data_file
    if (head != 1) {
        fprintf(stderr, "reading num_images improperly!\n");
        exit(1);
    }

    d->images = malloc(d->num_items * sizeof(Image));
    if (d->images == NULL) {
        // perror("malloc");
        fprintf(stderr, "images malloc wrong\n");
        exit(1);
    }

    d->labels = malloc(d->num_items * sizeof(unsigned char));
    if (d->labels == NULL) {
        // perror("malloc");
        fprintf(stderr, "labels malloc wrong\n");
        exit(1);
    }

    for (int i = 0; i < d->num_items; i++) {
        int label = fread(&d->labels[i], sizeof(unsigned char), 1, data_file); // read image's label into Dataset's array of labels -> SEG FAULT
        if (label != 1) {
            fprintf(stderr, "label read improperly\n");
            exit(1);
        }
        Image* img = malloc(sizeof(Image)); // allocate's space for an Image struct 
        if (img == NULL) {
            // perror("malloc");
            fprintf(stderr, "img malloc wrong\n");
            exit(1);
        }
        img->sx = WIDTH;
        img->sy = WIDTH;
        img->data = malloc(NUM_PIXELS * sizeof(unsigned char)); // allocate memory for array of size num_pixels for image's data 
        if (img->data == NULL) {
                // perror("malloc");
                fprintf(stderr, "img->data malloc wrong\n");
                exit(1);
            }
        for (int pixel = 0; pixel < NUM_PIXELS; pixel++) { 
            int img_data = fread(&img->data[pixel], sizeof(unsigned char), 1, data_file); // read image's data into an Image struct
            if (img_data != 1) {
                fprintf(stderr, "image pixel read improperly!\n");
                exit(1);
            }
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
    int labels[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // array of counts for each label from 0 to 9
    for (int i = 0; i < M; i++) {
        int temp_label = data->labels[indices[i]];
        labels[temp_label]++;
    }
    *label = -1;
    *freq = -1;
    for (int j = 0; j < 10; j++) { // for each label from 0 to 9
        if (labels[j] > *freq) { // if curr label occurs more than current most frequent label's frequency
            *label = j;
            *freq = labels[j];
        }
    }
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
    int curr_pixel = -1;
    double min_gini = INFINITY;

    for (int pixel = 0; pixel < NUM_PIXELS; pixel++) {
        double temp_gini = gini_impurity(data, M, indices, pixel); //compute gini impurity of current pixel
        if (!isnan(temp_gini)) { //check for NAN
            if (temp_gini < min_gini) { // if newly calculated impurity is less than the current minimum,
                min_gini = temp_gini; // replace
                curr_pixel = pixel;
            }
        }
    }
    return curr_pixel;
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
    int *label = (int*)malloc(sizeof(int));
    if (label == NULL) {
            //perror("malloc");
            fprintf(stderr, "subtree label malloc wrong\n");
            exit(1);
        } 
    int *freq = (int*)malloc(sizeof(int));
    if (freq == NULL) {
            //perror("malloc");
            fprintf(stderr, "subtree freq malloc wrong\n");
            exit(1);
        } 
    get_most_frequent(data, M, indices, label, freq);
    double ratio = (double)*freq/M;
    
    if (ratio > THRESHOLD_RATIO) {
        // don't split, make it a leaf that outputs the same class
        DTNode *leaf = malloc(sizeof(DTNode));
        if (leaf == NULL) {
            // perror("malloc");
            fprintf(stderr, "leaf malloc wrong\n");
            exit(1);
        } 
        leaf->classification = *label;
        leaf->pixel = -1;
        leaf->left = NULL;
        leaf->right = NULL;

        free(freq);
        free(label);
        return leaf;
    }
    else { // ratio is less than threshold, so split 
        int pixel = find_best_split(data, M, indices);
        int left_len = 0;
        int right_len = 0;
        for (int i = 0; i < M; i++) { // iterate through indices
            if (data->images[indices[i]].data[pixel] < 128) { // goes in left subtree
                left_len++;
            }
            else { right_len++; } // if geq 128, goes in right subtree
        }
        int *left_indices = malloc(left_len * sizeof(int));
        if (left_indices == NULL) {
            //perror("malloc");
            fprintf(stderr, "left indices malloc wrong\n");
            exit(1);
        }
        int *right_indices = malloc(right_len * sizeof(int));
        if (right_indices == NULL) {
            //perror("malloc");
            fprintf(stderr, "right indices malloc wrong\n");
            exit(1);
        }
        int left = 0;
        int right = 0;
        for (int j = 0; j < M; j++) {
            if (data->images[indices[j]].data[pixel] < 128) {
                left_indices[left] = indices[j];
                left++;
            }
            else { 
                right_indices[right] = indices[j];
                right++;
            }
        }

        DTNode *new_node = malloc(sizeof(DTNode));
        if (new_node == NULL) {
            fprintf(stderr, "new node malloc wrong\n");
            //perror("malloc");
            exit(1);
        }

        new_node->pixel = pixel;
        new_node->classification = -1;

        free(freq);
        free(label);

        new_node->left = build_subtree(data, left_len, left_indices);
        new_node->right = build_subtree(data, right_len, right_indices);

        free(left_indices);
        free(right_indices);

        return new_node;
    }
}

/**
 * This is the function exposed to the user. All you should do here is set
 * up the `indices` array correctly for the entire dataset and call 
 * `build_subtree()` with the correct parameters.
 */
DTNode *build_dec_tree(Dataset *data) {
    int *indices = malloc(data->num_items * sizeof(int));
    if (indices == NULL) {
        fprintf(stderr, "build_dec indices malloc wrong\n");
        //perror("malloc");
        exit(1);
    }

    for (int i = 0; i < data->num_items; i++) {
        indices[i] = i;
    }

    DTNode *tree = build_subtree(data, data->num_items, indices);
    free(indices);
    return tree;
}

/**
 * Given a decision tree and an image to classify, return the predicted label.
 */
int dec_tree_classify(DTNode *root, Image *img) {
    if (root->left == NULL && root->right == NULL) { // if at a leaf
        return root->classification;
    }
    else {
        if (img->data[root->pixel] < 128) {
            return dec_tree_classify(root->left, img);
        }
        else {
            return dec_tree_classify(root->right, img);
        }
    }
    return -1;
}

/**
 * This function frees the Decision tree.
 */
void free_dec_tree(DTNode *node) {
    if (node->left != NULL) {
        free_dec_tree(node->left);
    }
    if (node->right != NULL) {
        free_dec_tree(node->right);
    }
    free(node);
    return;
}

/**
 * Free all the allocated memory for the dataset
 */
void free_dataset(Dataset *data) {
    free(data->labels);
    for (int i = 0; i < data->num_items; i++) {
        free(data->images[i].data);
    }
    free(data->images);
    free(data);
    return;
}
