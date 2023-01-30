// #include <omp.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "beehive.h"
#include "grower.h"
#include "glider.h"

#define DEAD 0
#define ALIVE 1

#define generations 100

#define matrix_size 3000


/* For each element of the matrix apply the */
/* life game rules                          */
/* store under temp                         */
void new_generation(  int ** matrix, int ** temp )
{
    int i, j, sum;

    for (i = 1; i <= matrix_size; i++) 
        {
        for (j = 1; j <= matrix_size; j++) 
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


void swap_matrices(  int ** matrix, int ** temp, int *changes )
{
    int i, j, aux;

    /* Swap the matrices */
    *changes = 0;
    for (i = 1; i <= matrix_size; i++) 
        {
        for (j = 1; j <= matrix_size; j++) 
        {
        aux = matrix[i][j];
            matrix[i][j] = temp[i][j];
            temp[i][j] = aux;
            if ( matrix[i][j] != temp[i][j] )
            (*changes)++;
        }
        }
}


/* output life matrix, one number per line */
void output_life_matrix( int ** matrix )
{
    int i, j, out;
    out=0;
    /* output new configuration */
    for (i = 1; i <= matrix_size; i++) 
        for (j = 1; j <= matrix_size; j++) 
        //   printf("%d\n", matrix[i][j] );
        if ( matrix[i][j] == ALIVE )
        out++;

    fprintf( stderr, "No of alive cells: %d\n", out );

}


/* sets initial life matrix and boundary */
void initial( int ** matrix, int ** temp )
{
    int i, j, pattern;

    /* Initialize the boundaries of the life matrix */
    for (j = 0; j < matrix_size+2; j++)
        {
        matrix[0][j] = DEAD;
        matrix[matrix_size+1][j] = DEAD;
        temp[0][j] = DEAD;
        temp[matrix_size+1][j] = DEAD ;
        }
    for (i = 0; i < matrix_size+2; i++)
        {
        matrix[i][0] = DEAD;
        matrix[i][matrix_size+1] = DEAD;
        temp[i][0] = DEAD;
        temp[i][matrix_size+1] = DEAD ;
        }
    fprintf( stderr, "Boundary set\n");

    /* Initialize the life matrix */
    // fprintf(stderr, "Input number to set the pattern:  (1) Beehive, (2) Glider, (3) Grower" );
    // scanf("%d", &pattern);
    pattern = 3;
    switch (pattern)
    {
        case 1:
            for (i = 1; i <= BEEHIVE_HEIGHT; i++)
                for (j = 1; j <= BEEHIVE_WIDTH; j++)
                matrix[1500+i][1500+j] = beehive[i-1][j-1];
            break;
        case 2:
            for (i = 1; i <= GLIDER_HEIGHT; i++)
                for (j = 1; j <= GLIDER_WIDTH; j++)
                matrix[1500+i][1500+j] = glider[i-1][j-1];
            break;
        case 3:
            for (i = 1; i <= GROWER_HEIGHT; i++)
                for (j = 1; j <= GROWER_WIDTH; j++)
                matrix[1500+i][1500+j] = grower[i-1][j-1];
            break;
        default:
            break;
    }

    fprintf( stderr, "Pattern initialized\n");

}


int main()
{
    int i, iter, changes;
    int **matrix, **temp;

    fprintf( stderr, "Game starts\n");
    double start, end;
    start = clock();

    /* allocate memory for the matrices */
    matrix = (int **)malloc( sizeof(int *) * (matrix_size+2) );
    temp = (int **)malloc( sizeof(int *) * (matrix_size+2) );
    for (i = 0; i < matrix_size+2; i++) 
        {
        matrix[i] = (int *)malloc( sizeof(int) * (matrix_size+2) );
        temp[i] = (int *)malloc( sizeof(int) * (matrix_size+2) );
        }
    fprintf( stderr, "Memory for matrix set aside \n");

    /* Initialize the boundaries of the life matrix */
    /* Initialize the life matrix itself            */
    initial( matrix, temp ); 
    fprintf( stderr, "Population initialized\n");
    output_life_matrix( matrix);

    /* iterate over generations */
    for( iter=1 ; iter<=generations ; iter++ )
        {
            if(iter==10 ||iter==100 ||iter%10==0)
                fprintf( stderr, "Generation %d starts: ", iter);
            new_generation( matrix, temp );
            swap_matrices(  matrix, temp, &changes );
            if(iter==10 ||iter==100 ||iter%10==0)
                output_life_matrix( matrix );
            
        }

    for (i = 0; i < matrix_size+2; i++) 
        {
        free( matrix[i] );
        free( temp[i] );
        }
    
    end = clock();
    printf("Time taken: %f\n", (end-start)/CLOCKS_PER_SEC);

    free( matrix );
    free( temp );
    exit( 0 );
    
}
