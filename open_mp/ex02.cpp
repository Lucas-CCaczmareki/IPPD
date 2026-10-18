#include <cstdio>
#include <cstdlib>
#include <omp.h>
#include <vector>
#include <algorithm>
#include <cmath>

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

//calcular C[i] = a*A[i] + B[i] sequencialmente e paralelamente
// recebe vetores A e B por referência const (n pode modificar)
// recebe C como referência normal (pode modificar)
void calc_seq(double a, const std::vector<double>& A, const std::vector<double>& B, std::vector<double>& C) {
    const long n = A.size();

    for (long i = 0; i < n; ++i) {
        C[i] = a*A[i] + B[i];
    }
}

void calc_par(double a, const std::vector<double>& A, const std::vector<double>& B, std::vector<double>& C) {
    const long n = A.size(); //shared por padrão

    #pragma omp parallel for
    for (long i = 0; i < n; ++i) {
        C[i] = a*A[i] + B[i];
    }
}

int main(int argc, char* argv[]) {
    const double a = 2.5;
    printf("threads = %d\n\n", omp_get_max_threads()); //threads é definida na hora q eu chamo o executável com OMP_NUM_THREADS=4 ./prog
    printf("%10s %12s %12s %9s\n", "N", "T_seq(s)", "T_par(s)", "speedup"); //só formata string pra fazer uma tabela

    for(long n = 1000; n <= 10000000; n *=10) { // esse for existe pra testar com 1000, 10000, 100000, etc elementos pra ver quando q começa a compensar
        std::vector<double> A(n), B(n), Cseq(n), Cpar(n);

        // for q preenche todos vetores
        for(long i = 0; i < n; ++i) {
            A[i] = i * 0.5;
            B[i] = 1.0 / (i + 1);
        }

        double t_seq = medir([&] { calc_seq(a, A, B, Cseq); });
        double t_par = medir([&] { calc_par(a, A, B, Cpar); });

        bool ok = (Cseq == Cpar); //compara pra ver se foram realizadas as mesmas operações por elemento. Permite identificar se foi justo
        printf("%10ld %12.6f %12.6f %8.2fx %s\n", n, t_seq, t_par, t_seq / t_par, ok ? "" : "ERRO!");
    }
    return 0;
}

// speedup é quantas vezes o paralelo foi mais rápido que sequencial
// pra números pequenos o custo de criar e sincronizar as threads é maior, então paralelizar deixa mais lento
// pra número MUITO grande os dados, eles param de caber todos na cache, e o gargalo vira a velocidade de transferência de dados da RAM (tem q ficar buscando pra fazer os cálculos)
// pra números médios (cabem na cache), o gargalo é a CPU, então o paralelo apresenta melhora