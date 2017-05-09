#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
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

void Merge(int array[], int begin, int mid, int end) 
{
    int ib = begin;
    int im = mid;
    int j;
    int size = end-begin;
    int b[size];
    
    /* Enquanto existirem elementos na lista da esquerda ou direita */
    for (j = 0; j < (size); j++)  
    {
        if (ib < mid && (im >= end || array[ib] <= array[im]))  
        {
            b[j] = array[ib];
            ib = ib + 1;
        }
        else  
        {
            b[j] = array[im];
            im = im + 1;
        }
    }
    
    for (j=0, ib=begin; ib<end; j++, ib++) array[ib] = b[j];
}

void Sort(int array[], int begin, int end)
{
	int mid;
	
	if (begin == end)
		return;
	
	if (begin == end - 1)
		return;
	
	mid = (begin + end) / 2;
	
	Sort(array, begin, mid);
	Sort(array, mid, end);
	Merge(array, begin, mid, end);
}

int main(int argc, char **argv)
{
	int vectorA[atoi(argv[1])], vectorB[atoi(argv[1])];
	int myRank, size, numLines, tag = 50;
	int i;
	char *fileName;
	FILE *finalFile;
	MPI_Status status;

	// Getting arguments
	numLines = atoi(argv[1]);
	fileName = argv[2];

	// Oppening result file
	/*finalFile = fopen("finalFile.txt", "w+");*/

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
	if(numLines % size != 0 && numLines % 2 != 0)
	{
		printf("ERROR: number of lines to read must be multiple of 'np'");
		return -1;
	}

	if(size == 1){
		Sort(vectorA, 0, numLines);
	}else{
		int newVectorSize, nextVectorSize, previousVectorSize, children, *nodeVector;
		int treeSize, treeLevel = 0;
		
		treeSize = log2(size);

		if(myRank == 0)
		{
			while(treeLevel < treeSize){
				if(treeLevel != treeSize)
					children = myRank + pow(2, treeLevel);
				
				newVectorSize = numLines / pow(2, treeLevel);
				nextVectorSize = newVectorSize / 2;

				if(treeLevel == 0){
					nodeVector = (int*)malloc(sizeof(int) * newVectorSize);
					memcpy(nodeVector, vectorA, sizeof(int) * newVectorSize);
				}else{
					nodeVector = (int*)realloc(nodeVector, sizeof(int) * newVectorSize);
				}

				MPI_Send(nodeVector + nextVectorSize, nextVectorSize, MPI_INT, children, tag, MPI_COMM_WORLD);
				treeLevel++;
			}

			nodeVector = (int*)realloc(nodeVector, sizeof(int) * nextVectorSize);
			Sort(nodeVector, 0, nextVectorSize);
			previousVectorSize = nextVectorSize;
			treeLevel--;

			while(treeLevel >= 0){
				children = myRank + pow(2, treeLevel);
				nodeVector = (int*)realloc(nodeVector, sizeof(int) * newVectorSize);
				MPI_Recv(nodeVector + previousVectorSize, previousVectorSize, MPI_INT, children, tag, MPI_COMM_WORLD, &status);
				Merge(nodeVector, 0, previousVectorSize, newVectorSize);
				treeLevel--;
				previousVectorSize = newVectorSize;
				newVectorSize = newVectorSize * 2;
			}
			// ESCREVER NO ARQUIVO

		}
		else
		{
			int father, maxValuesToReceive, myLevel;

			maxValuesToReceive = numLines / 2;
			nodeVector = (int*)malloc(sizeof(int) * numLines);
			MPI_Recv(nodeVector, maxValuesToReceive, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_INT, &newVectorSize);
			father = status.MPI_SOURCE;
			treeLevel = log2(numLines / newVectorSize);
			myLevel = treeLevel;
			/*printf("%d - treeLevel = %d\n", myRank, treeLevel);*/

			if(treeLevel != treeSize){
				//newVectorSize = numLines / pow(2, treeLevel);
				nextVectorSize = newVectorSize / 2;
			}else{
				/*nextVectorSize = newVectorSize;
				newVectorSize = nextVectorSize * 2;*/
				nodeVector = (int*)realloc(nodeVector, sizeof(int) * newVectorSize);
			}

			while(treeLevel < treeSize){
				//if(treeLevel != treeSize)
				children = myRank + pow(2, treeLevel);

				newVectorSize = newVectorSize / 2;
				/*printf("%d -> %d\n", myRank, children);*/
				MPI_Send(nodeVector + newVectorSize, newVectorSize, MPI_INT, children, tag, MPI_COMM_WORLD);
				nodeVector = (int*)realloc(nodeVector, sizeof(int) * newVectorSize);
				treeLevel++;
				//newVectorSize = numLines / pow(2, treeLevel);
				nextVectorSize = newVectorSize;
			}
			for(i = 0; i < nextVectorSize; i++)
					printf("%d - nodeVector[i]: %d\n", myRank, nodeVector[i]);
				printf("\n\n\n");

			Sort(nodeVector, 0, newVectorSize);
			previousVectorSize = newVectorSize;
			treeLevel--;

			while(treeLevel >= myLevel)
			{
				children = myRank + pow(2, treeLevel);
				newVectorSize = newVectorSize * 2;
				nodeVector = (int*)realloc(nodeVector, sizeof(int) * newVectorSize);
				MPI_Recv(nodeVector + previousVectorSize, previousVectorSize, MPI_INT, children, tag, MPI_COMM_WORLD, &status);
				Merge(nodeVector, 0, previousVectorSize, newVectorSize);
				treeLevel--;
				previousVectorSize = newVectorSize;
				//newVectorSize = newVectorSize * 2;
			}
			printf("%d sending %d values\n", myRank, newVectorSize);
			MPI_Send(nodeVector, newVectorSize, MPI_INT, father, tag, MPI_COMM_WORLD);

			/*for(i = 0; i < newVectorSize; i++)
				printf("%d -> nodeVector[%d] = %d\n", myRank, i, nodeVector[i]);*/
		}
	}

	MPI_Finalize();

	return 0;
}








