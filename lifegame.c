/*****************************************************************************
 * @file: lifegame.c 
 * @authors: Zhe Liu, Bowen Liang, Yue Zhang
 * @version: 1.1
 * @date: 2023-1-29
 * Description: game of life implemented parallelization with OpenMP in C
 *****************************************************************************/
#include "stdint.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <omp.h>
#include "beehive.h"
#include "glider.h"
#include "grower.h"

#define MATRIX_SIZE 3000
#define ITERATIONS 5000
#define DEAD 0
#define ALIVE 1
uint8_t **matrix;
uint8_t **temp; 
void init(int h, int w, uint8_t** pattern);
void new_generation();
void copy_matrix();
void output_life_matrix();
int main(int argc, char *argv[]){
    if(argc != 2){
	    printf("The input is not correct, please input the number of a pattern.\n1\tGrower\n2\tGlider\n3\tBeehive\n");
	    exit(1);
    }
    int choice = atoi(argv[1]);
    /* allocate memory for the matrices, matricx and temp are global variables */
    matrix = (uint8_t **)malloc( sizeof(uint8_t *) * (MATRIX_SIZE+2) );
    temp = (uint8_t **)malloc( sizeof(uint8_t *) * (MATRIX_SIZE+2) );
    for (int i = 0; i < MATRIX_SIZE+2; i++) 
        {
        matrix[i] = (uint8_t *)malloc( sizeof(uint8_t) * (MATRIX_SIZE+2) );
        temp[i] = (uint8_t *)malloc( sizeof(uint8_t) * (MATRIX_SIZE+2) );
        }
    fprintf( stderr, "Memory for matrix set aside \n");

    /*initial with pattern model*/
    if(choice == 1){
         printf("Grower Pattern\n");
         init(GROWER_HEIGHT, GROWER_WIDTH, (uint8_t**) grower);
    }
    else if(choice == 2){
         printf("Glider Pattern\n");
         init(GLIDER_HEIGHT, GLIDER_WIDTH, (uint8_t**) glider);
    }
    else if(choice == 3){ 
         printf("Beehive Pattern\n");
         init(BEEHIVE_HEIGHT, BEEHIVE_WIDTH, (uint8_t**) beehive);
    }
    else{
        printf("Wrong Input, Bye!\n");
        exit(0);
    }

    /*start iteration*/
    double start, end;
    start = omp_get_wtime();
    for(int iter = 0; iter < ITERATIONS; iter ++){
        fprintf( stderr, "Generation %d starts -- ", iter+1);
        new_generation();
        /*swap the address of matrix and temp*/
        copy_matrix();
        output_life_matrix();
    }
    end = omp_get_wtime();
    printf("Time taken: %fs, Thread using: %d\n", end-start, omp_get_max_threads());
    for (int i = 0; i < MATRIX_SIZE+2; i++) 
    {
        free( matrix[i] );
        free( temp[i] );
    }
    free( matrix );
    free( temp );
    return 0;
} 
/**
 * Function name:init() 
 * Description:function is to initialize the matrix of matrix and temp
 * with specified pattern
 * Parameter
 *@gap: to make sure the pattern[0][0] starts from matrix[1500][1500]
 *@h: the height of the pattern matrix
 *@w: the width of the pattern matrix
*/
void init(int h, int w, uint8_t** pattern){
    //printf("%d\n", *((uint8_t*) pattern+1)+0); 
    /*initialize the matrix*/
    int gap = (int) (MATRIX_SIZE+2)/2;
    #pragma omp parallel for
    for(int i = 0; i < MATRIX_SIZE+2; i++){
        for(int j = 0; j < MATRIX_SIZE+2; j++){
            matrix[i][j] = DEAD;
            temp[i][j] = DEAD;
            if((i>=gap && i<gap+h) && (j>=gap && j<gap+w)){
                int x = i-gap;
                int y = j-gap;
                if(*((uint8_t *)pattern+ x*w + y) == 1){
                    matrix[i][j] = ALIVE;
                    temp[i][j] = ALIVE;
                }
            }
        }
    }
}
/**
 *Function name:new_generation() 
 *Description:to generate the next state of life matrix and store the next 
 *status in temp[][].
 *Parameter
 *@sum: to count the alive neighbor
*/
void new_generation(){
    int i, j, sum;
    #pragma omp parallel for 
    for (i = 1; i <= MATRIX_SIZE; i++) 
    {
        for (j = 1; j <= MATRIX_SIZE; j++) 
        {
            /* find out the neighbors of the current cell */
            sum = matrix[i-1][j-1] + matrix[i-1][j] + matrix[i-1][j+1] 
                + matrix[i][j-1] + matrix[i][j+1] 
                    + matrix[i+1][j-1] + matrix[i+1][j] + matrix[i+1][j+1] ;
                
            /* check if the cell dies or life is born */ 

            /* dies of isolation */
            if (sum < 2 || sum > 3)
                temp[i][j] = DEAD ;

            /* born through neighbors */
            else if (sum == 3)
                temp[i][j] = ALIVE ;

            /* life continues */
            else
                temp[i][j] = matrix[i][j] ;
        }
    }
}
/**
 * Function name: copy_matrix()
 * Description: copy the new states from temp[][] to matrix[][] for next iteration
*/
void copy_matrix(){
    #pragma omp parallel for
    for(int i = 1; i <= MATRIX_SIZE; i++){
        for(int j = 1; j<= MATRIX_SIZE; j++){
            if(temp[i][j] == matrix[i][j]) continue;
            matrix[i][j] = temp[i][j];
        }
    }
}
/**
 * Function name: output_life_matrix()
 * Description: count the number of alive population for each iteration
 * Parameter
 * @out: the sum of the alive population
*/
void output_life_matrix(){
    int i, j, out;
    out=0;
    /* output new configuration */
    #pragma omp parallel for reduction(+:out)
    for (i = 1; i <= MATRIX_SIZE; i++)
    {
        for (j = 1; j <= MATRIX_SIZE; j++){
            if ( matrix[i][j] == ALIVE ) out++;
        }
    }
    fprintf(stderr, "Number of alive cells: %d\n", out);
}
