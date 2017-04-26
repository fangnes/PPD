#include<stdio.h>
//#include<mpi.h>
#include<stdlib.h>

int main(int argc, char **argv)
{
	int n;

	if(argc < 2)
	{
		printf("Missing arguments!");
		return 0;
	}

	n = atoi(argv[1]);
	
/* --------------- SEQUENCIAL PROGRAM ---------------  */
	int vetorA[n], vetorB[n], i, j, x, v;
	FILE *pFile;

	pFile = fopen("numTest.txt", "r");

	for(i = 0; i < n; i++)
	{
		fscanf(pFile, "%d\n", &v);
		vetorA[i] = v;
	}

	for(i = 0; i < n; i++)
	{
		x = 0;
		for(j = 0; j < n; j++)
			if(vetorA[i] > vetorA[j])
				x++;
		vetorB[x] = vetorA[i];
	}

	for(i = 0; i < n; i++)
		printf("%d\n", vetorB[i]);
	

	return 0;
}
