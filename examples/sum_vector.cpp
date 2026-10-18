#include <iostream>
#include <vector>
#include <cstdlib>
#include <random>
#include <omp.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " <threads> <tamanho>\n";
        return 1;
    }

    //argc é o número de argumentos q a chamada do programa recebe
    //argv é o vetor com os argumentos, incluindo o nome do arquivo
    //todos argumentos (até números) vem como strings

    int num_threads = std::atoi(argv[1]);   // atoi transforma o número de threads num número inteiro
    int n = std::atoi(argv[2]);             // mesma coisa pro tamanho do vetor

    omp_set_num_threads(num_threads); //função openmp que define o número de threads do pool


    // preenchendo o vetor de forma paralela também (adição minha)
    std::vector<int> vetor(n); // instancia o vetor n com 0

    #pragma omp parallel
    {
        // cada thread cria o seu próprio gerador de números aleatórios
        // caso eles compartilhassem, o estado interno do rand() aplicaria um lock
        // e o uso de threads pra isso deixaria o processo mais lento
        std::mt19937 gen(12345 + omp_get_thread_num()); // seed
        std::uniform_int_distribution<int> dist(0, 99); // gera numeros aleatórios de 0 a 99

        #pragma omp for schedule(static)
        for (int i = 0; i < n; i++) {
            vetor[i] = dist(gen);
        }
    }

    long soma = 0;
    double inicio = omp_get_wtime();

    #pragma omp parallel for reduction(+:soma) schedule(static)
    for (int i = 0; i < n; i++) {
        soma += vetor[i];
    }

    double fim = omp_get_wtime();
    std::cout << "Soma = " << soma << " | Tempo = " << fim - inicio << " s\n";

    return 0;
}
