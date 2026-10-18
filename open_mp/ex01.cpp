// g++ -fopenmp -O2 ex01.cpp -o ex01
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char *argv[]) {
    
    //inicializa uma região de memória paralela
    // todas as threads executam esse mesmo código
    #pragma omp parallel 
    {
        // variável privada de cada thread pois foi declarada dentro da omp parallel
        int t_id = omp_get_thread_num(); // retorna o id da thread atual. 

        // laço simples, vai ser executado 8 vezes por cada thread
        for (int i = 0; i < 8; i++) {
            // executa nthread x 8 vezes
            // printf("A: thread %d -> i = %d\n", t_id, i);
        }

        // declarar um omp for dentro de uma região paralela é construir um worksharing
        // worksharing: dividir o trabalho de uma região paralela entre as threads.
        // ou seja, vai dividir as iterações desse for entre as threads (cada iteração roda 1 única vez)
        // as n threads trabalham pra dividir esse laço
        #pragma omp for
        for (int i = 0; i < 8; i++) {
            //executa 8 vezes
            // printf("B: thread %d -> i = %d\n", t_id, i);
        }
        // barreira implícita do omp for (todas threads vão esperar até que todas terminem antes de seguir com a execução)
        // ! uma cláusula 'nowait()' poderia mudar isso!

        // a cláusula 'schedule()' controla como as iterações do laço vão ser divididas em chunks (blocos de iterações p/ cada thread)
        // a static divide as iterações em chunks antes da execução iniciar.
        #pragma omp for schedule(static)
        for (int i = 0; i < 8; i++) {
            // cada thread recebe a sua chunk com n iterações e executa.
            // printf("B: thread %d -> i = %d\n", t_id, i);
        }

        // a cláusica dynamic faz com q a thread q se libera pega a próxima iteração
        // o 1 indica em 
        #pragma omp for schedule(dynamic, 1) // esse 1 define o número de ITERAÇÕES por chunk. Ent aq 8 chunks
        for (int i = 0; i < 8; i++) {
            // cada thread pega a próxima chunk "na fila" assim que terminar a sua chunk atual.
            printf("B: thread %d -> i = %d\n", t_id, i);
        }
    }

}
