#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

void merge(int array[], int begin, int mid, int end) {
    int ib = begin;
    int im = mid;
    int j;
    int size = end-begin;
    int b[size];
    
    /* Enquanto existirem elementos na lista da esquerda ou direita */
    for (j = 0; j < (size); j++)  {
        if (ib < mid && (im >= end || array[ib] <= array[im]))  {
            b[j] = array[ib];
            ib = ib + 1;
        }
        else  {
            b[j] = array[im];
            im = im + 1;
        }
    }
    
    for (j=0, ib=begin; ib<end; j++, ib++) array[ib] = b[j];
}

void printVetor(int *vetor, int begin, int size, FILE *file)
{
	int i;


	for(i = begin; i < begin + size; i++)
		fprintf(file, "%d, ", vetor[i]);

	printf("begin: %d, size: %d, \n", begin, size);
	printf("printing: ");
	for(i = begin; i < size; i++)
		printf("%d, ", vetor[i]);
	printf("\n");
	fprintf(file, "\n");
}

// Lê 'numLines' números do arquivo passado por parâmetro
void readFile(char *fileName, int *vetor, int numLines)
{
	FILE *pFile;
	int i;

	pFile = fopen(fileName, "r");

	for(i = 0; i < numLines; i++)
		fscanf(pFile, "%d\n", &vetor[i]);

	fclose(pFile);	
}

int main(int argc, char **argv)
{
	int myRank, nSlaves, tag;
	int posA, posB; 
	int nElementos, numLinhas, jobs;
	int i;
	MPI_Status status;
    
	//FILE *mFile, *sFile;

	// Iniciando MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &nSlaves);

	numLinhas = atoi(argv[1]);					// Número de elementos a serem lidos
	if((numLinhas % (4 * (nSlaves - 1))) != 0)
		nElementos = numLinhas / (4 * (nSlaves - 1));		// Número de elementos a serem enviados para os escravos
	else
	{
		printf("ERROR: division of jobs not integer\n");
		return -1;
	}
	jobs = numLinhas / nElementos;					// Número de tarefas total a serem distribuidas entre os escravos

	/*
	printf("###############################\n");
	printf("nElementos: %d\n", nElementos);
	printf("slaves: %d\n", nSlaves);
	printf("numLinhas: %d\n", numLinhas);
	printf("###############################\n");
	*/
	posA = 0;
	posB = 0;

	if(myRank == 0) // Mestre
	{ 
		int vetorA[numLinhas], vetorB[numLinhas];
		int slave;
		FILE *pFile;

		//mFile = fopen("master_log", "w");
		pFile = fopen("finalFile.txt", "w+");

		readFile(argv[2], vetorA, numLinhas);
		//printVetor(vetorA, 0, numLinhas, mFile);
		//fprintf(mFile, "\n");
		
		// Envia os dois primeiros jobs para os slaves		
		for(slave = 1; slave < nSlaves; slave++)
		{
			//fprintf(mFile, "Master sending: ");
			//printVetor(vetorA, posA, nElementos, mFile);
			MPI_Send(vetorA + posA, nElementos, MPI_INT, slave, tag, MPI_COMM_WORLD);
			posA += nElementos;
		}


		// Envia um job para o slave que terminar seu job
		for(i = 1; i <= jobs; i++)
		{
			MPI_Recv(vetorB + posB, nElementos, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);

			/*
			fprintf(mFile, "MASTER received:");
			printVetor(vetorB, posB, nElementos, mFile);
			fprintf(mFile, "\n");
			*/			
			posB += nElementos;

			merge(vetorB, 0, posB - nElementos, posB);
			slave = status.MPI_SOURCE;

        		if(i <= jobs - 2)
            		{
 				/*
    				fprintf(mFile, "Master sending: ");
    				printVetor(vetorA, posA, nElementos, mFile);
    				fprintf(mFile, "\n");
    				*/
    				MPI_Send(vetorA + posA, nElementos, MPI_INT, slave, tag, MPI_COMM_WORLD);
	    			posA += nElementos;
           		 }
		}

		// Manda sinal de término da tarefa para os slaves
		vetorA[0] = -1;
		for(slave = 1; slave < nSlaves; slave++)
			MPI_Send(vetorA, 1, MPI_INT, slave, tag, MPI_COMM_WORLD);
		
		for(i = 0; i < numLinhas; i++)
			fprintf(pFile, "%d\n", vetorB[i]);

		fclose(pFile);
	}
	else // Escravo
	{
		int slaveVetorA[nElementos], slaveVetorB[nElementos];
		int x, j;

		//sFile = fopen("slave_log", "a+");
		MPI_Recv(slaveVetorA, nElementos, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);

		//fprintf(sFile, "Slave %d received: ", myRank);
		//printVetor(slaveVetorA, 0, nElementos, sFile);
		
		while(slaveVetorA[0] != -1)
		{
			for(i = 0; i < nElementos; i++)
			{
				x=0;
				for(j = 0; j < nElementos; j++)
				{
					if(slaveVetorA[i] > slaveVetorA[j])
						x++;
				}
				slaveVetorB[x] = slaveVetorA[i];
			}		

			//fprintf(sFile, "Slave sending: ");
			//printVetor(slaveVetorB, 0, nElementos, sFile);
        		MPI_Send(slaveVetorB, nElementos, MPI_INT, 0, tag, MPI_COMM_WORLD);
			MPI_Recv(slaveVetorA, nElementos, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
			//fprintf(sFile, "Slave received: ");
			//printVetor(slaveVetorA, 0, nElementos, sFile);
		}
	}

	//printf("This is the end of %d\n", myRank);

	MPI_Finalize();
	return 0;
}
