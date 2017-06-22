/*
    O objetivo do trabalho é implementar, usando a biblioteca MPI, uma versão paralela usando o modelo divisão e conquista do algoritmo de ordenação MergeSort.
    O programa deverá receber como parâmetros de entrada um número N, representando o número de valores a serem testados, e o nome de um arquivo, que conterá a lista de valores inteiros a serem ordenados.
    Utilizar 3 casos de teste para realização das medições no cluster.
    A saída que deve ser gerada é a lista ordenada (crescente) dos valores de entrada (1 valor por linha) e também o tempo de execução da aplicação.
*/

//COMPILE: mpicc -o dc dc.c -Wall -Wextra
//EXECUTE: mpirun -np (2 - 4 - 8 - 16) dc (500k - 1kk - 2kk) FILE_WITH_INTEGERS_TO_SORT

//Autor: Felipe Angnes
//Data: 08/05/2017

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "mpi.h"

/* @brief: função escreve conteúdo do vetor em um arquivo
 * @param *vector: ponteiro para o inicio do vetor a ser escrito
 * @param begin: deslocamento do ponteiro *vector
 * @param size: quantidade de dados a serem escritos
 * @param *file: ponteiro para o file descriptor do arquivo a ser escrito
 */
void printVector(int *vetor, int begin, int size, FILE *file)
{
	int i;

	for(i = begin; i < begin + size; i++)
		fprintf(file, "%d\n", vetor[i]);
}

/* @brief: lê conteúdo de um arquivo passado por parâmetro e coloca em um vetor
 * @param fileName: file descriptor do arquivo a ser lido
 * @param numLines: número de elementos a ser lido
 */
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
/* @brief: rotina que realiza o merge de dois vetores ordenando seus elementos
 * @param array: vetor contendo os dois vetores a serem ordenados
 * @param begin: inicio do primeiro vetor
 * @param mid: divisa entre primeiro e segundo vetor
 * @param end: final do segundo vetor
 */
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

/* @brief: rotina que divide o vetor o máximo possível, e logo após realiza o merge de todos os pedaços
 * @param array: vetor a ser dividido e ordenado
 * @param begin: inicio do vetor
 * @param end: final do vetor
 */
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
	int vectorA[atoi(argv[1])];
	int myRank, size, numLines, tag = 50;
	int i;
	struct timeval ti, tf;
	char *fileName;
	FILE *finalFile;
	MPI_Status status;

	// Convertendo argumentos
	numLines = atoi(argv[1]);
	fileName = argv[2];
	
	// Limpando vetor inicial
	memset(vectorA, 0, numLines * sizeof(int));

	// Lendo 'numLines' de 'fileName'
	readFile(fileName, vectorA, numLines);

	// Iniciando MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	// Verificando consistência dos parâmetros
	if(numLines % size != 0 && numLines % 2 != 0)
	{
		printf("ERROR: number of lines to read must be multiple of 'np'");
		return -1;
	}

	if(size == 1){ // Versão sequencial
		FILE *pFile;

		pFile = fopen("finalFile.txt", "w+"); 		// Abrindo arquivo final
		gettimeofday(&ti, NULL);	      		// Salvando tempo de inicio do algoritmo
		Sort(vectorA, 0, numLines);	      		// Executando merge sort
		gettimeofday(&tf, NULL);	      		// Salvando tempo de término
		printVector(vectorA, 0, numLines, pFile);	// Escrevendo vetor ordenado no arquivo
		
		// Calculando tempo de execução e escreve no arquivo
		fprintf(pFile, "%ds%dms%dus\n", (int)(tf.tv_sec - ti.tv_sec),
										(int)(tf.tv_usec - ti.tv_usec) / 1000,
										(int)(tf.tv_usec - ti.tv_usec) % 1000);
		fclose(pFile);					// Fechando arquivo final

	}else{  // Versão paralela
		int newVectorSize, nextVectorSize, previousVectorSize, children, *nodeVector;
		int treeSize, treeLevel = 0;
		
		treeSize = log2(size);	// Calcula o tamanho total da árvore

		if(myRank == 0)
		{
			FILE *pFile;

			pFile = fopen("finalFile.txt", "w+");		// Abrindo arquivo final
			gettimeofday(&ti, NULL);			// Salvando tempo de inicio do algoritmo
			while(treeLevel < treeSize){			// Enquanto o nível atual da árvore for menor que o tamanho total
				if(treeLevel != treeSize)
					children = myRank + pow(2, treeLevel);	// Calcula o rank do filho
				
				newVectorSize = numLines / pow(2, treeLevel);	// Calcula o novo tamanho do vetor
				nextVectorSize = newVectorSize / 2;		// Calcula o tamanho do vetor no próximo nível

				if(treeLevel == 0){	// Se for nível raiz
					nodeVector = (int*)malloc(sizeof(int) * newVectorSize);					// Aloca tamanho necessário para o vetor
					memcpy(nodeVector, vectorA, sizeof(int) * newVectorSize);				// Copia o conteúdo do vetor lido inicialmente para o vetor local do processo
				}else{
					nodeVector = (int*)realloc(nodeVector, sizeof(int) * newVectorSize);			// Realloca o tamanho do vetor para o novo tamanho
				}

				MPI_Send(nodeVector + nextVectorSize, nextVectorSize, MPI_INT, children, tag, MPI_COMM_WORLD);	// Envia a segunda metade do vetor para o processo filho
				treeLevel++;	// Incrementa o nível da árvore
			}

			nodeVector = (int*)realloc(nodeVector, sizeof(int) * nextVectorSize);	// Realoca tamanho do vetor para o tamanho que o vetor interno da folha deve ter
			Sort(nodeVector, 0, nextVectorSize);					// Conquista
			previousVectorSize = nextVectorSize;					// "Renomeação" de variável para tornar o código mais legível
			treeLevel--;								// Decrementa o nível da árvore

			while(treeLevel >= 0){	// Enquanto o nível atual da árvore for maior ou igual a zero
				children = myRank + pow(2, treeLevel);					// Calcula o rank do filho
				nodeVector = (int*)realloc(nodeVector, sizeof(int) * newVectorSize);	// Realoca tamanho do vetor para o novo tamanho
				MPI_Recv(nodeVector + previousVectorSize, previousVectorSize, MPI_INT, children, tag, MPI_COMM_WORLD, &status);	// Recebe o vetor ordenado enviado pelo filho
				Merge(nodeVector, 0, previousVectorSize, newVectorSize);		// Faz o merge do vetor interno com o vetor recebido do filho
				treeLevel--;								// Decrementa nível da árvore
				previousVectorSize = newVectorSize;					// Salva tamanho anterior da vetor
				newVectorSize = newVectorSize * 2;					// Calcula novo tamanho do vetor
			}
			gettimeofday(&tf, NULL);							// Salvando tempo de término
			printVector(nodeVector, 0, numLines, pFile);					// Escreve resultado no arquivo
			
			// Calculando tempo de execução e escreve no arquivo
			fprintf(pFile, "%ds%dms%dus\n", (int)(tf.tv_sec - ti.tv_sec),
										(int)(tf.tv_usec - ti.tv_usec) / 1000,
										(int)(tf.tv_usec - ti.tv_usec) % 1000);
			fclose(pFile);		// Fechando arquivo final
		}
		else
		{
			int father, maxValuesToReceive, myLevel;

			maxValuesToReceive = numLines / 2;				// Calcula valor máximo a ser recebido
			nodeVector = (int*)malloc(sizeof(int) * maxValuesToReceive);	// Aloca vetor interno com o resultado do cálculo da linha anterior
			MPI_Recv(nodeVector, maxValuesToReceive, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);	// Recebe vetor enviado pelo processo pai
			MPI_Get_count(&status, MPI_INT, &newVectorSize);		// Salva em 'newVectorSize' a quantidade de elementos recebida
			father = status.MPI_SOURCE;					// Salva em 'father' o rank do pai
			treeLevel = log2(numLines / newVectorSize);			// Cálcula nível da árvore
			myLevel = treeLevel;						// Salva nível de início do processo

			if(treeLevel != treeSize)	// Se o processo não for um processo folha da árvore
				nextVectorSize = newVectorSize / 2;					// Calcula tamanho do vetor no próximo nível
			else
				nodeVector = (int*)realloc(nodeVector, sizeof(int) * newVectorSize);	// Realoca tamanho do vetor para o número de elementos recebido

			while(treeLevel < treeSize){			// Enquanto o nivel da árvore for maior que o tamanho da árvore
				children = myRank + pow(2, treeLevel);	// Calcula rank do filho
				newVectorSize = newVectorSize / 2;	// Calcula novo tamanho do vetor
				MPI_Send(nodeVector + newVectorSize, newVectorSize, MPI_INT, children, tag, MPI_COMM_WORLD);	// Envia a segunda metade do vetor para o filho
				nodeVector = (int*)realloc(nodeVector, sizeof(int) * newVectorSize);	// Realoca o vetor para o novo tamanaho
				treeLevel++;								// Incrementa nível da árvore
				nextVectorSize = newVectorSize;
			}

			Sort(nodeVector, 0, newVectorSize);	// Conquista
			previousVectorSize = newVectorSize;	// "Renomeação" de variável para tornar o código mais legível
			treeLevel--;				// Decrementa o nível da árvore

			while(treeLevel >= myLevel)			// Enquanto o nível da árvore for maior que o nível do processo
			{
				children = myRank + pow(2, treeLevel);	// Calcula o rank do filho
				newVectorSize = newVectorSize * 2;	// Calcula o novo tamanho do vetor
				nodeVector = (int*)realloc(nodeVector, sizeof(int) * newVectorSize);	// Realoca o vetor interno com o novo tamanho
				MPI_Recv(nodeVector + previousVectorSize, previousVectorSize, MPI_INT, children, tag, MPI_COMM_WORLD, &status);	// Recebe o vetor ordenado do filho
				Merge(nodeVector, 0, previousVectorSize, newVectorSize);	// Faz o merge do vetor interno com o vetor recebido do filho
				treeLevel--;							// Decrementa o nível da árvore
				previousVectorSize = newVectorSize;				// Salva o tamanho do vetor do nível anterior
			}
			MPI_Send(nodeVector, newVectorSize, MPI_INT, father, tag, MPI_COMM_WORLD);	// Envia o vetor ordenado interno no processo para o pai
		}
	}

	MPI_Finalize();

	return 0;
}
