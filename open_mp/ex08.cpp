#include <cstdio>
#include <cstdlib>
#include <omp.h>
#include <vector>
#include <random>


int main(int argc, char* argv[]) {

    if (argc < 2) {
        return 1; //erro
    }

    long long n = atoll(argv[1]);

    std::vector<int> v(n);
    // std::vector<long> hist1(256, 0), hist2(256, 0);
    long seq[256] = {0};
    long ingenua[256] = {0};
    long hist1[256] = {0};
    long hist2[256] = {0};

    // v é o vetor de dados. Ex: {3, 3, 3, 0, 7}
    // isso significa que hist[3] recebe 3 incrementos, hist[0] recebe 1, etc

    // preenchendo o vetor
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> distrib(0, 255);

    double t0 = omp_get_wtime();

    // cria o vetor de dados
    for(int i = 0; i < n; ++i) {
        v[i] = distrib(gen);
    }

    // sequencial (referência)
    t0 = omp_get_wtime();
    for (long long i = 0; i < n; ++i) seq[v[i]]++;
    printf("sequencial: %.4f.\n", omp_get_wtime() - t0);   

    // ingênua: tem corrida
    t0 = omp_get_wtime();
    # pragma omp parallel for
    for (long long i = 0; i < n; ++i) ingenua[v[i]]++;
    printf("ingenua: %.4f. %s\n", omp_get_wtime()-t0, std::equal(seq, seq + 256, ingenua) ? "(acertou por sorte)" : "<-- ERRADO (corrida)");

    // preenche o histograma paralelamente usando atomic
    t0 = omp_get_wtime();
    # pragma omp parallel for
    for(int i = 0; i < n; ++i) {
        # pragma omp atomic
        hist1[v[i]]++;
    }
    printf("atomic    : %.4f.  %s\n", omp_get_wtime() - t0, std::equal(seq, seq + 256, hist1) ? "ok" : "ERRO");


    // preenche o histograma paralelamente usando histogramas privados por thread
    t0 = omp_get_wtime();
    # pragma omp parallel for reduction(+ : hist2[0 : 256])
    for(int i = 0; i < n; ++i) {
        hist2[v[i]]++;
    }
    printf("reduction: %.4f. %s\n", omp_get_wtime()-t0, std::equal(seq, seq+256, hist2) ? "ok"  : "ERRO");


    // versão manual sem reduction
    long hist_global[256] = {0};
    t0 = omp_get_wtime();
    # pragma omp parallel
    {
        long hist_local[256] = {0}; // cada thread declara a sua versão do histograma privada e local

        // divide as iterações entre o pool de threads
        // a cláusula nowait aqui faz as threads do pool não precisarem esperar as outras terminarem antes de ir pra seção crítica
        # pragma omp for nowait 
        for(long i = 0; i < n; ++i) {
            hist_local[v[i]]++;
        }

        // a thread que finaliza o local fica esperando liberar aqui e depois soma no global
        # pragma omp critical 
        for(long i = 0; i < 256; ++i) {
            hist_global[i] += hist_local[i];
        }
    }
    printf("privados  : %.4f. %s\n", omp_get_wtime() - t0, std::equal(seq, seq + 256, hist_global) ? "ok" : "ERRO");
    return 0;
}