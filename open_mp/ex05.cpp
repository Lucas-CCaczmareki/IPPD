#include <cstdio>
#include <cstdlib>
#include <omp.h>
#include <algorithm>
#include <cmath>
#include <vector>

template <class F>
double medir(F&& f, int reps = 5) { //roda 5 vezes pq a primeira execução é imprecisa (as coisas n tão em cache ainda)
    double melhor = 1e30;

    for (int r = 0; r < reps; ++r) {
        double t0 = omp_get_wtime();
        f();
        melhor = std::min(melhor, omp_get_wtime() - t0); // escolhe o menor entre o melhor tempo até agr e o novo tempo
    }
    return melhor;
}

void sumSeq_vector(double& sum_seq, const std::vector<double>& v) {
    long long n = v.size();
    double sum = 0;

    for (long long i = 0; i < n; i++) {
        sum += v[i];
    }
    sum_seq = sum;
}

void sumPar_vector(double& sum_par, const std::vector<double>& v) {
    long long n = v.size();
    double sum = 0;

    int t_id, nthreads;                        // declaradas ANTES. Precisam estar em private(...)
    long inicio, fim, i;

    // pra evitar condição de corrida
    // # pragma omp parallel for reduction(+ : sum)
    // for (long long i = 0; i < n; i++) {
    //     sum += v[i];
    // }

    # pragma omp parallel for reduction(+ : sum) private(t_id, nthreads, inicio, fim, i)
    // cada uma das threads executa esse bloco
    {
        t_id = omp_get_thread_num();
        nthreads = omp_get_num_threads(); //devolve o número de threads dessa região

        // faz a divisão manual dos blocos do vetor de maneira contígua.
        inicio = n * t_id / nthreads;       // inicio é baseado no número da thread
        fim = n * (t_id + 1) / nthreads;    // fim é a posição de início da próxima thread

        for (i = inicio; i < fim; ++i) {
            sum += v[i];
        }
    }

    sum_par = sum;
}

int main(int argc, char* argv[]) {

    if(argc < 2) { 
        printf("error, missing operand."); 
        return 1; 
    }

    long long n = atoll(argv[1]); // r
    std::vector<double> v(n); //cria um vetor de tamanho n

    for(long long i = 0; i < n; ++i) {
        v[i] = i; //só pra somar mas daria pra colocar um num randomico aq
    }

    double sum_seq = 0, sum_par = 0;

    double t_seq = medir([&] { sumSeq_vector(sum_seq, v); });
    double t_par = medir([&] { sumPar_vector(sum_par, v); });
    double speedup = t_seq / t_par;

    printf("t_seq: %12.6f\nt_par: %12.6f\nspeedup: %8.2fx\n%s\n", t_seq, t_par, speedup, sum_seq == sum_par ? "ok" : "ERRO");
}