/*
   O programa abaixo com um mestre e N escravos. Cada escravo recebe dois valores inteiros e positivos, soma eles e envia o resultado para o mestre. O mestre gera M (M tem que ser par e maior ou igual a 2*N) valores a serem somados, envia para os escravos. Quando um escravo termina a execução, manda o resultado de volta para o mestre e se o mestre ainda tiver trabalho manda mais trabalho para o escravo.
 
   Author: Avelino Zorzo
   Date:   22.09.2013
 */

#include <mpi.h>
#include <stdio.h>

#include <time.h>
#include <stdlib.h>

// gera valores entre 0 e 100
int gera_valor() {
    return rand()%100;
}

int main(int argc, char **argv) {
    int rank, np, escravo, m, v1,v2, tag=0;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    
    if (argc < 2) {
        if (rank==0) printf("Uso: programa M (onde M é o número de valores a serem somados.)\n");
        MPI_Finalize();
        return 1;        
    }
    m = atoi(argv[1]);
    if ((m%2 != 0) || (m < 2*(np-1))) {
        if (rank == 0) printf("Numero de elementos a somar tem que ser par\n e maior que 2 vezes o número de escravos.\n");
        MPI_Finalize();
        return 1;
    }
    
    if (rank == 0) { // Mestre
        int valores_a_receber, soma_total;
        
        srand(time(NULL));
        
        for (escravo=1; escravo<np; escravo++) {
            // envia dois valores para cada escravo
            v1 = gera_valor();
            MPI_Send(&v1, 1, MPI_INT, escravo, tag, MPI_COMM_WORLD);
            v2 = gera_valor();
            MPI_Send(&v2, 1, MPI_INT, escravo, tag, MPI_COMM_WORLD);
            m = m - 2; // gerou e enviou 2 valores
            printf("Master enviou %d e %d para %d\n",v1,v2,escravo);
        }
        valores_a_receber = np-1;
        soma_total = 0;
        while (valores_a_receber != 0) {
            MPI_Recv(&v1, 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
            escravo = status.MPI_SOURCE; // recebeu deste escravo
            valores_a_receber--;   // decrementa o numero de valores a receber
            soma_total = soma_total + v1;
            if (m > 0) { // ainda tem que gerar elementos
                         // manda para o escravo que respondeu
                v1 = gera_valor();
                MPI_Send(&v1, 1, MPI_INT, escravo, tag, MPI_COMM_WORLD);
                v2 = gera_valor();
                MPI_Send(&v2, 1, MPI_INT, escravo, tag, MPI_COMM_WORLD);
                m = m - 2; // gerou e enviou 2 valores
                printf("Master enviou %d e %d para %d\n",v1,v2,escravo);
                valores_a_receber++; // tem um novo resultado a receber
            }
        }
        v1 = -1;
        for (escravo=1; escravo<np; escravo++) {
            // envia valor para encerrar escravo
            MPI_Send(&v1, 1, MPI_INT, escravo, tag, MPI_COMM_WORLD);
        }
        printf("Master tem soma total = %d\n", soma_total);

    } else { // Código para os escravos
        int terminou = 0, s;
        
        while (!terminou) { // fica trabalhando enquanto o mestre não terminar
            MPI_Recv(&v1, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
            if (v1 < 0) // mestre terminou
                terminou = 1;
            else { // ainda tem trabalho
                MPI_Recv(&v2, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
                s = v1+v2;
                printf("%d: recebeu %d e %d, enviou %d\n",rank,v1,v2,s);
                MPI_Send(&s, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
            }
        }
    }
        
    MPI_Finalize();
  return 0;
}