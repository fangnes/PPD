#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

void printVector(int *vetor, int begin, int size, FILE *file)
{
	int i;

	for(i = begin; i < begin + size; i++)
		fprintf(file, "%d\n", vetor[i]);
}

// Lê 'numLines' números do arquivo passado por parâmetro
void readFile(char *fileName, int *vetor, int numLines)
{
	FILE *pFile;
	int i;

	pFile = fopen(fileName, "r");

	for(i = 0; i < numLines; i++)
		if(fscanf(pFile, "%d\n", &vetor[i]) < 0)
			printf("ERROR while read file\n");

	fclose(pFile);	
}

void insertionSort(int value, int *vector, int index, int flag, int nextStage)
{
	int i, j, val, aux, tag = 50;

	val = value;

	for(j = 0; j <= index; j++)
	{
		if(val < vector[j]) 
		{
			aux = vector[j];
			vector[j] = val;
			val = aux;
		}
	}
	if(flag == 0)
		vector[index] = val;
	else
		MPI_Send(&val, 1, MPI_INT, nextStage, tag, MPI_COMM_WORLD);
}

int main(int argc, char **argv)
{
	int vectorA[atoi(argv[1])], vectorB[atoi(argv[1])];
	int myRank, size, numLines, tag = 50;
	int nPositions;
	int i;
	char *fileName;
	FILE *finalFile;
	MPI_Status status;

	// Getting arguments
	numLines = atoi(argv[1]);
	fileName = argv[2];

	// Oppening result file
	finalFile = fopen("finalFile.txt", "w+");

	// Cleaning vectors
	memset(vectorA, 0, numLines * sizeof(int));
	memset(vectorB, 0, numLines * sizeof(int));

	// Reading 'numLines' from 'fileName'
	readFile(fileName, vectorA, numLines);

	// Starting MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	// Verifying consistency of attributes
	if(numLines % size != 0)
	{
		printf("ERROR: number of lines to read must be multiple of 'np'");
		return -1;
	}
	
	// Number of positions for intern vectors
	nPositions = numLines / size;
	
	if(size == 1)
	{ // Sequencial case
		for(i = 0; i < numLines; i++)
			insertionSort(vectorA[i], vectorB, i, 0, 0);
	}
	else
	{ // Parallel case
		int nextStage, j;
		int signal = -1;

		memset(internVector, 0, nPositions * sizeof(int));

		// Defining process nextStage
		if(myRank + 1 != size)
			nextStage = myRank + 1;
		else
			nextStage = 0;

		if(myRank == 0)
		{ // Rank0

			for(i = 0; i < numLines; i++)
				// If internVector is full, 'flag' parameter is changed to '1'
				(i > nPositions - 1) ? 	insertionSort(vectorA[i], vectorB, nPositions - 1, 1, nextStage) :
										insertionSort(vectorA[i], vectorB, i, 0, nextStage);

			// Send signal to finish nextStage rank1
			MPI_Send(&signal, 1, MPI_INT, nextStage, tag, MPI_COMM_WORLD);

			// Receive from all nextStages their ordered vectors
			for(i = 1; i < size; i++)
				MPI_Recv(vectorB + (nPositions * i), nPositions, MPI_INT, i, tag, MPI_COMM_WORLD, &status);

			printVector(vectorB, 0, numLines, finalFile);
		}
		else
		{ // Rank'i'
			int internVector[nPositions];
			int receivedValue, index, previousStage;

			previousStage = myRank - 1;
			receivedValue = 0;
			index = 0;

			// Receive first value from 'myRank - 1'
			MPI_Recv(&receivedValue, 1, MPI_INT, previousStage, tag, MPI_COMM_WORLD, &status);
			
			// This nextStage will be executed until 'signal' be sent by previous nextStage
			while(receivedValue != -1)
			{
				// If internVector is full, 'flag' parameter is changed to '1'
				(index == nPositions) ? insertionSort(receivedValue, internVector, nPositions - 1, 1, nextStage) :
										insertionSort(receivedValue, internVector, index, 0, nextStage);
			
				// Increases 'index' of 'internVector' of this stage
				if(index <= nPositions - 1)
					index++;

				// Receive another value from 'previousStage'
				MPI_Recv(&receivedValue, 1, MPI_INT, previousStage, tag, MPI_COMM_WORLD, &status);
			}

			// Send 'signal' to finish the next nextStage
			if(nextStage != 0)
				MPI_Send(&signal, 1, MPI_INT, nextStage, tag, MPI_COMM_WORLD);

			// Send 'internVector' to nextStage 0
			MPI_Send(internVector, nPositions, MPI_INT, 0, tag, MPI_COMM_WORLD);
		}
	}

    MPI_Finalize();

	return 0;
}