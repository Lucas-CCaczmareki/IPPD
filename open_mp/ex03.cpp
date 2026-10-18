#include <cstdio>
#include <cstdlib>
#include <omp.h>

int main(int argc, char* argv[]) {
    int x = 10;

    // com private, cada thread recebe uma cópia não inicializada da variável (o valor original se perde)
    #pragma omp parallel private(x) 
    {
        //printam 0
        printf("private: thread %d -> x = %d\n", omp_get_thread_num(), x);
        x += omp_get_thread_num();
        printf("sum: \t thread %d -> x = %d\n", omp_get_thread_num(), x);
    }

    // private não copia o valor da variável após o fork pra variável original no join.
    printf("original x: x = %d", x);
    printf("\n\n");

    // com firstprivate, cada thread recebe uma cópia da variável contendo o valor dela antes da área paralela
    #pragma omp parallel firstprivate(x) 
    {
        //printam 10
        printf("private: thread %d -> x = %d\n", omp_get_thread_num(), x);
        x += omp_get_thread_num();
        printf("sum: \t thread %d -> x = %d\n", omp_get_thread_num(), x);
    }

    // firstprivate também não copia o valor da variável após o fork pra variável original no join.
    printf("original x: x = %d", x);
    printf("\n\n");


    // caso o comportamento de cópia de valor pra variável original fosse desejado, seria necessário utilizar lastprivate ou reduction
}