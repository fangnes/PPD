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

int main(int argc, char **argv)
{
	int myRank, nSlaves, tag;
	int posA, posB, nElementos, numLinhas, i, j, x, source, jobs;
	MPI_Status status;
    FILE *mFile, *sFile;

	// Iniciando MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &nSlaves);

	// Calculo de quantos elementos serão enviados
	// para cada escravo
	numLinhas = atoi(argv[1]);
	nElementos = numLinhas / (4 * (nSlaves - 1));
	jobs = numLinhas / nElementos;

    /*
	printf("JOBS: %d\n", jobs);
	printf("nElementos: %d\n", nElementos);
    */	

	if(myRank == 0) // Mestre
	{
		
		int vetorA[atoi(argv[1])], vetorB[atoi(argv[1])];
		int slave = 1, k;
		FILE *pFile;
	
		// Calculo de quando jobs serão ao todo	
		pFile = fopen(argv[2], "r");
		mFile = fopen("master_log", "w");

		// Lê 'numLinhas' números do arquivo passado por parâmetro
		for(i = 0; i <= numLinhas - 1; i++)
		{
			fscanf(pFile, "%d\n", &vetorA[i]);
			//printf("%d, ", vetorA[i]);
		}
		printf("\n");

		posA = 0;
		// Envia os dois primeiros jobs para os slaves		
		for(slave = 1; slave < nSlaves; slave++)
		{
			// Print DEBUG
			//printf("Master sending: %d, %d\n", vetorA[posA], vetorA[posA + 1]);
			
			MPI_Send(vetorA + posA, nElementos, MPI_INT, slave, tag, MPI_COMM_WORLD);
			posA += nElementos;
		}

		posB = 0;
		// Envia mais um job para o slave que terminar seu job
		for(i = 1; i <= jobs; i++)
		{
            //printf("presi\n");
			MPI_Recv(vetorB + posB, nElementos, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
			printf("MASTER received: %d, %d, %d, %d\n", vetorB[posB], vetorB[posB + 1], vetorB[posB + 2], vetorB[posB + 3]);
			posB += nElementos;

			
			// Print DEBUG
			//for(k = 0; k <= nElementos; k++)
			//	printf("Master received vetorB[%d]: %d\n", k, vetorB[k]);

			merge(vetorB, 0, posB - nElementos, posB);

			slave = status.MPI_SOURCE;

			// Print DEBUG
			//for(k = 0; k < nElementos; k++)
			//	printf("Master sending vetorA[%d]: %d\n", k + posA, vetorA[k + posA]);

            if(i <= jobs - 2)
            {
                //printf("Master sending: %d, %d | i = %d\n", vetorA[posA], vetorA[posA + 1], i);
    			MPI_Send(vetorA + posA, nElementos, MPI_INT, slave, tag, MPI_COMM_WORLD);
	    		posA += nElementos;
            }
		}



		for(i = 0; i < numLinhas; i++)
			printf("%d, ", vetorB[i]);
        printf("\n");

		vetorA[0] = -1;
		for(slave = 1; slave <= nSlaves - 1; slave++)
			MPI_Send(vetorA, 1, MPI_INT, slave, tag, MPI_COMM_WORLD);
	}
	else // Escravo
	{
		int slaveVetorA[nElementos], slaveVetorB[nElementos], k;

		MPI_Recv(slaveVetorA, nElementos, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);

		//printf("Slave received elements: %d, %d\n", slaveVetorA[0], slaveVetorA[1]);

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

        	printf("SLAVE sending elements: %d, %d, %d, %d\n", slaveVetorB[0], slaveVetorB[1], slaveVetorB[2], slaveVetorB[3]);
			MPI_Send(slaveVetorB, nElementos, MPI_INT, 0, tag, MPI_COMM_WORLD);

			MPI_Recv(slaveVetorA, nElementos, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
			//printf("Slave RECEIVED elements: %d, %d\n", slaveVetorA[0], slaveVetorA[1]);
		}
	}


	printf("This is the end of %d\n", myRank);

	MPI_Finalize();
	return 0;
}
