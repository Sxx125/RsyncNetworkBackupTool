#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "ftree.h"
/*
 * Compute an xor-based hash of the data provided on STDIN. The result
 * should be placed in the array of length block_size pointed to by
 * hash_val.
 */
char* hash(FILE *f) {
    
    char *hash_val = malloc(sizeof(char)*BLOCKSIZE);
    
    int i = 0;
    for(int i=0;i<BLOCKSIZE;i++){
 	    hash_val[i] = '\0';
    }
    int input;
    while((input = fgetc(f)) != EOF) { 
	        hash_val[i] = hash_val[i]^input;
	        i++;
	    if(i==BLOCKSIZE){
	        i=0;
	    }
    }

    return hash_val;

}

