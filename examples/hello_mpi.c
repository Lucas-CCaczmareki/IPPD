#include <mpi.h>
#include <stdio.h>

// argc = número de argumentos
// argv = lista de argumentos passados
int main(int argc, char** argv) {

    // cria o ambiente MPI
    // argc e argv definem quantos processos o MPI vai criar, por isso precisa deles
    MPI_Init(&argc, &argv);
        int rank;
        // MPI_COMM_WORLD é o comunicador comum de processos
        // ele contém todos processos lançados

        // MPI_Comm_rank é uma função que pergunta pro comunicador
        // o rank do processo q ta rodando e guarda em &rank
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        // Printa o rank do processo
        printf("Hello do processo %d\n", rank);

    // finaliza o ambiente MPI e libera recursos
    MPI_Finalize();
    return 0;
}
