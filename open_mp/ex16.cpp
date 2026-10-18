#include <cstdio>
#include <cstdlib>
#include <omp.h>
#include <iostream>

// O exercício é paralelizar isso
long fib_seq(int n) {
    if (n < 2) return n;
    return fib_seq(n-1) + fib_seq(n-2);
}

// é um fibonacci "burro" mesmo. Ele calcula coisas repetidas
// mas que são independentes, portanto paralelizáveis
long fib_par(int n, int limite) {
    // depois é só trocar if (n < limit) return fib_seq
    // pra não criar tarefas pequenas demais. Que piorariam o desempenho
    // O desempenho fica pior por que o overhead de criar as tasks supera o ganho de paralelizar cálculos simples
    // como o fibonacci de 1~25 por exemplo.
    if (n < 2) return n; 
    if (n < limite) return fib_seq(n);
    long x = 0, y = 0;

    // cria uma sub-árvore esquerda inteira de tasks
    // x e y são shared por que eles precisam existir fora da recursão das tasks pra depois ser somado e dar o resultado
    // n é privado pra garantir que cada task tem eu próprio n, mesmo que a task pai 
    # pragma omp task shared(x) firstprivate(n)
    x = fib_par(n-1, limite);

    // cria uma sub-árvore direita inteira de tasks
    # pragma omp task shared (y) firstprivate(n)
    y = fib_par(n-2, limite);

    // essa diretiva faz com que o programa abaixo dela só execute APÓS
    // todas as tasks criadas acima E as suas filhas, terminarem a sua execução.
    # pragma omp taskwait
    return x + y;
}

int main(int argc, char* argv[]) {
    
    if (argc < 3) { std::cerr << "missing operand\n"; return 1; }

    long res = 0;
    int n = atoi(argv[1]);
    int limite = atoi(argv[2]);
    
    double t0 = 0;

    t0 = omp_get_wtime();
    # pragma omp parallel
    {   
        // lembrando, esse bloco é executado só pela thread criadora (a 1a q chegar)
        # pragma omp single
        {
            res = fib_par(n, limite);
        }
    }
    std::cout << "res: " << res << "\ntempo par: " << omp_get_wtime()-t0 << "\n\n";

    //sequencial
    t0 = omp_get_wtime();
    res = fib_seq(n);
    std::cout << "res: " << res << "\ntempo seq: " << omp_get_wtime()-t0 << "\n";


}