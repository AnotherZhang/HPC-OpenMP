#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <mpi.h>
#include <omp.h>
#include "beehive.h"
#include "grower.h"
#include "glider.h"

#define board(x, y, c) (board[((x) * (c)) + (y)])
#define live(i) ((board[(i)]) % 2)

#define matrix_size 3000
#define generations 5000

#define DEAD 0
#define ALIVE 1

void initial(int **matrix);
void print_board(int *board, int rows, int columns);
void move_next_step(int rows, int c, int *board);
void output_life_board(int *board, int rows, int columns);

int main(int argc, char *argv[])
{
	/*initialize the row and column of the matrix*/
	int rows, columns;
	rows = matrix_size;
	columns = matrix_size;

	/*start the clock*/
	fprintf(stderr, "Game starts\n");
	double start, end;
	start = clock();

	/*initialize MPI*/
	int n_threads, world_size, rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	/* Check command line arguments */
	if (argc < 2)
	{
		if (rank == 0)
			printf("Usage: %s [num threads]\n", argv[0]);
		exit(1);
	}

	/*set the num of threads for OpenMP*/
	sscanf(argv[1], "%d", &n_threads);
	omp_set_num_threads(n_threads);

	/*allocate memory for the matrices*/
	int **init_board = (int **)malloc(sizeof(int *) * (matrix_size + 2));
	for (int i = 0; i < matrix_size + 2; i++)
	{
		init_board[i] = (int *)malloc(sizeof(int) * (matrix_size + 2));
	}
	fprintf(stderr, "Memory for matrix set aside \n");
	initial(init_board);

	/* Set chunk size for all processes*/
	/* Find row numbers for current process */
	int row_start, row_end;
	int chunk_size = (rows / world_size);
	int rmdr = rows - chunk_size * world_size;
	row_start = chunk_size * rank;
	if (rank < rmdr)
	{
		chunk_size++;
		row_start += rank;
	}
	else
		row_start += rmdr;
	row_end = row_start + chunk_size;

	/* Board is a row wise flattened 1D structure */
	int *board = (int *)calloc((chunk_size + 2) * columns, sizeof(int));
	int row_size = columns * sizeof(int);
	int src = row_start - (rank != 0);
	int dest = (rank == 0) * columns;
	int size = chunk_size + (rank != 0) + (rank != (world_size - 1));
#pragma omp parallel for
	for (int i = 0; i < size; ++i)
	{
		memcpy(board + dest + i * columns, init_board[src + i], row_size);
	}

	free(init_board);

	MPI_Request request[2], send_req[2];
	MPI_Status recv_status[2];
	int flag0, flag1;

	for (int iter = 0; iter < generations; iter++)
	{
		flag0 = flag1 = 1;
		/* Do non-blocking send and recv of previous and next rows except for the 1st iteration*/
		if (iter != 0)
		{
			if (rank != 0)
			{
				flag0 = 0;
				/* Receive previous row information - non blocking */
				MPI_Irecv(board, columns, MPI_INT, rank - 1, 10, MPI_COMM_WORLD, &request[0]);
			}
			if (rank != world_size - 1)
			{
				flag1 = 0;
				/* Receive next row information - non blocking */
				MPI_Irecv(board + (chunk_size + 1) * columns, columns, MPI_INT, rank + 1, 11, MPI_COMM_WORLD, &request[1]);
			}
			if (rank != 0)
			{
				/* Send previous row information - non blocking*/
				MPI_Isend(board + columns, columns, MPI_INT, rank - 1, 11, MPI_COMM_WORLD, &send_req[1]);
			}
			if (rank != world_size - 1)
			{
				/* Send next row information - non blocking*/
				MPI_Isend(board + chunk_size * columns, columns, MPI_INT, rank + 1, 10, MPI_COMM_WORLD, &send_req[0]);
			}
		}

		/* Perform Game of Life for rows 2 to last but one */
		int start_location = (1 + (!flag0)) * columns;
		int size = chunk_size - (!flag0) - (!flag1);

		move_next_step(size, columns, board + start_location);

		/* Check if the previous and next rows have been received */
		while (!flag0 || !flag1)
		{
			if (!flag0)
			{
				MPI_Test(&request[0], &flag0, &recv_status[0]);
				if (flag0)
				{
					move_next_step(1, columns, board + columns);
				}
			}
			if (!flag1)
			{
				MPI_Test(&request[1], &flag1, &recv_status[1]);
				if (flag1)
				{
					move_next_step(1, columns, board + chunk_size * columns);
				}
			}
		}
		/* Change all progress elements */
#pragma omp parallel for
		for (int i = columns; i < (chunk_size + 1) * columns; i++)
			if (board[i] > 1)
				board[i] = 3 - board[i];
	}

	MPI_Barrier(MPI_COMM_WORLD);

	/* Receive the final board using Gatherv*/
	if (rank == 0)
	{
		int chunk_sizes[world_size];
		/* Initially set all chunks as same size */
		for (int i = 0; i < world_size; i++)
			chunk_sizes[i] = (rows / world_size) * columns;
		/* Increment chunk_size by 1 for remaining processes */
		if (rmdr != 0)
			for (int i = 0; i < rmdr; i++)
				chunk_sizes[i] += columns;

		/* Use gatherv to receive chunks from all processes */
		int displacements[world_size];
		displacements[0] = 0;
		for (int i = 1; i < world_size; i++)
			displacements[i] = displacements[i - 1] + chunk_sizes[i - 1];

		int *final_board = (int *)calloc(rows * columns, sizeof(int));
		MPI_Gatherv(board + columns, chunk_size * columns, MPI_INT, final_board, chunk_sizes, displacements, MPI_INT, 0, MPI_COMM_WORLD);
		output_life_matrix(final_board, rows, columns);
		free(final_board);
	}
	else
	{
		MPI_Gatherv(board + columns, chunk_size * columns, MPI_INT, NULL, NULL, NULL, MPI_INT, 0, MPI_COMM_WORLD);
	}
	free(board);
	MPI_Finalize();

	end = clock();
	printf("Time taken: %f\n", (end - start) / CLOCKS_PER_SEC);
	return 0;
}

/*print the current board (One-dimensional array)*/
void print_board(int *board, int rows, int columns)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
			printf("%d\n", board[i * columns + j]);
		printf("\n");
	}
}

/*evolve to the next generation following the survival rules*/
void move_next_step(int rows, int c, int *board)
{
#pragma omp parallel for
	for (int i = 0; i < rows * c; i++)
	{
		int live_nbr_count = 0;
		if (i % c != 0)
		{
			live_nbr_count += live(i - c - 1);
			live_nbr_count += live(i - 1);
			live_nbr_count += live(i + c - 1);
		}
		live_nbr_count += live(i - c);
		live_nbr_count += live(i + c);

		if (live_nbr_count > 3)
		{
			if (live(i))
/* Doing test in the middle to reduce the total no. of operations */
#pragma omp atomic write
				board[i] = 3;
			continue;
		}
		if ((i + 1) % c != 0)
		{
			live_nbr_count += live(i - c + 1);
			live_nbr_count += live(i + 1);
			live_nbr_count += live(i + c + 1);
		}
		if (live(i))
		{
			if (live_nbr_count != 3 && live_nbr_count != 2)
#pragma omp atomic write
				board[i] = 3;
		}
		else if (live_nbr_count == 3)
		{
#pragma omp atomic write
			board[i] = 2;
		}
	}
}

/* output the number of alive cells */
void output_life_board(int *board, int rows, int columns)
{
	int out = 0;
	/* output new configuration */
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			if (board[i * columns + j] == 1)
				out++;
		}
	}
	fprintf(stderr, "No of alive cells: %d\n", out);
}

/* sets initial life matrix and boundary */
void initial(int **board)
{
	int i, j, pattern;

	/* Initialize the boundaries of the life matrix */
	for (j = 0; j < matrix_size + 2; j++)
	{
		board[0][j] = DEAD;
		board[matrix_size + 1][j] = DEAD;
	}
	for (i = 0; i < matrix_size + 2; i++)
	{
		board[i][0] = DEAD;
		board[i][matrix_size + 1] = DEAD;
	}
	fprintf(stderr, "Boundary set\n");

	/* Initialize the life matrix */
	fprintf(stderr, "Input number to set the pattern:  (1) Beehive, (2) Glider, (3) Grower");
	scanf("%d", &pattern);
	switch (pattern)
	{
	case 1:
		for (i = 1; i <= BEEHIVE_HEIGHT; i++)
			for (j = 1; j <= BEEHIVE_WIDTH; j++)
				board[1500 + i][1500 + j] = beehive[i - 1][j - 1];
		break;
	case 2:
		for (i = 1; i <= GLIDER_HEIGHT; i++)
			for (j = 1; j <= GLIDER_WIDTH; j++)
				board[1500 + i][1500 + j] = glider[i - 1][j - 1];
		break;
	case 3:
		for (i = 1; i <= GROWER_HEIGHT; i++)
			for (j = 1; j <= GROWER_WIDTH; j++)
				board[1500 + i][1500 + j] = grower[i - 1][j - 1];
		break;
	default:
		break;
	}

	fprintf(stderr, "Pattern initialized\n");
}