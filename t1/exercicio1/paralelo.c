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
	nElementos = numLinhas / (4 * nSlaves);

	if(myRank == 0)
	{
		int vetorA[atoi(argv[1])], jobs, slave = 1;
		FILE *pFile;
		
		pFile = fopen(argv[2], "r");
		jobs = numLinhas / nElementos;

		// Lê numLinhas números do arquivo passado por parâmetro
		for(i = 0; i < numLinhas; i++)
		{
			fscanf(pFile, "%d\n", &vetorA[i]);
		}

		posA = 0;
		// Envia os dois primeiros jobs para os slaves		
		for(slave = 1; slave < nSlaves; slave++)
		{
			printf("sending 2 firsts jobs\n");
			MPI_Send(&vetorA[posA], nElementos, MPI_INT, slave, tag, MPI_COMM_WORLD);
			posA += nElementos;
		}

		// Envia mais um job para o slave que terminar seu job
		for(i = 0; i < jobs; i++)
		{
			printf("sending another jobs\n");
			MPI_Recv(&vetorB, nElementos, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);

			slave = status.MPI_SOURCE;

			MPI_Send(&vetorA[posA], nElementos, MPI_INT, slave, tag, MPI_COMM_WORLD);
			posA += nElementos;
		}

		for(i = 0; i < nElementos; i++)
		{
			printf("%d\n", vetorB[i]);
		}
	}
	else
	{
		int slaveVetorA[nElementos], slaveVetorB[nElementos];

		printf("testando 2\n");

		MPI_Recv(&slaveVetorA, nElementos, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);

		printf("valores\n");
		for(i = 0; i < nElementos; i++)
			printf("%d\n", slaveVetorA[i]);
		printf("--------------\n");
		for(i = 0; i < nElementos; i++)
		{

			x=0;
			for(j = 0; j < nElementos; j++)
			{
				printf("olar vc \n");
				if(slaveVetorA[i] > slaveVetorA[j])
					x++;
			}
			slaveVetorB[x] = slaveVetorA[i];
		}

		MPI_Send(&slaveVetorB, nElementos, MPI_INT, 0, tag, MPI_COMM_WORLD);
		printf("ja era\n");
	}

	printf("This is the end\n");

	MPI_Finalize();
	return 0;
}
