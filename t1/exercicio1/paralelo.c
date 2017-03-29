#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main(int argc, char **argv)
{
	int vetorB[atoi(argv[1])];
	int myRank, nSlaves, tag;
	int posA, posB, nElementos, numLinhas, i, j, x, source;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &nSlaves);

	numLinhas = atoi(argv[1]);
	nElementos = numLinhas / (4 * (nSlaves - 1));

	if(myRank == 0)
	{
		// Mestre
		int vetorA[atoi(argv[1])], jobs, slave = 1, k;
		FILE *pFile;
		
		pFile = fopen(argv[2], "r");
		jobs = numLinhas / nElementos;

		// Lê numLinhas números do arquivo passado por parâmetro
		for(i = 0; i <= numLinhas; i++)
		{
			fscanf(pFile, "%d\n", &vetorA[i]);
			printf("%d, ", vetorA[i]);
		}

		posA = 0;
		MPI_Send(&vetorA[posA], nElementos, MPI_INT, 1, tag, MPI_COMM_WORLD);
		posA += nElementos;
		MPI_Send(&vetorA[posA], nElementos, MPI_INT, 2, tag, MPI_COMM_WORLD);

		MPI_Recv(vetorB, nElementos, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);

		printf("Master received elements: %d, %d\n", vetorB[0], vetorB[1]);


		
		// Envia os dois primeiros jobs para os slaves		
		/*
		for(slave = 1; slave < nSlaves; slave++)
		{
			// Print DEBUG
			for(k = 0; k < nElementos; k++)
				printf("Master sending vetorA[%d]: %d\n", k + posA, vetorA[k + posA]);
			
			MPI_Send(&vetorA[posA], nElementos, MPI_INT, slave, tag, MPI_COMM_WORLD);
			posA += nElementos;
		}

		posB = 0;
		// Envia mais um job para o slave que terminar seu job
		for(i = 0; i < jobs; i++)
		{
			MPI_Recv(&vetorB[posB], nElementos, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
			posB += nElementos;
			// Print DEBUG
			for(k = 0; k <= nElementos; k++)
				printf("Master received vetorB[%d]: %d\n", k, vetorB[k]);

			slave = status.MPI_SOURCE;

			// Print DEBUG
			for(k = 0; k < nElementos; k++)
				printf("Master sending vetorA[%d]: %d\n", k + posA, vetorA[k + posA]);
			
			MPI_Send(&vetorA[posA], nElementos, MPI_INT, slave, tag, MPI_COMM_WORLD);
			posA += nElementos;
		}*/
	}
	else
	{
		int slaveVetorA[nElementos], slaveVetorB[nElementos], k;

		MPI_Recv(slaveVetorA, nElementos, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
		printf("Slave received elements: %d, %d\n", slaveVetorA[0], slaveVetorA[1]);

		/*
		for(i = 0; i < nElementos; i++)
			printf("Slave received slaveVetorA[%d]: %d\n", i, slaveVetorA[i]);
		*/

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

		for(k = 0; k < nElementos; k++)
			printf("Slave sending slaveVetorB[%d]: %d\n", k, slaveVetorB[k]);
		

		printf("Slave sending elements: %d, %d\n", slaveVetorB[0], slaveVetorB[1]);
		MPI_Send(&slaveVetorB[0], nElementos, MPI_INT, 0, tag, MPI_COMM_WORLD);
	}

	/*
	printf("Printing vector B elements:\n");
	for(i = 0; i < nElementos; i++)
	{
		printf("%d\n", vetorB[i]);
	}
	*/
	printf("This is the end\n");

	MPI_Finalize();
	return 0;
}
