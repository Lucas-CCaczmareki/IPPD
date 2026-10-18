#include <cstdio>
#include <cstdlib>
#include <omp.h>

int main (int argc, char* argv[]) {

    const long long N = (argc > 1) ? atoll(argv[1]) : 10000000;
    const long long res_esperado = N * (N + 1) / 2;

    printf("res esperado: %lld\n\n", res_esperado);

    for (int rep = 0; rep < 5; ++rep) {
        long long soma = 0;

        // versão com condição de corrida
        #pragma omp parallel for //soma é variável shared, vai dar condição de corrida
        for (long long i = 1; i <= N; i++) {
            /*
            // essa instrução faz 3 coisa, le, soma e escreve.
            // aqui ocorre uma condição de corrida, 2 threads leem e somam, e então escrevem (paralelamente)
            // ou seja, duas leem 100, somam 1 e somam 2 (temos 101 e 102) e então escrevem 101, dps escrevem 102
            // o resultado q era pra ser 103 é 102, um valor é perdido
            */
            soma += i; 
        }
        printf("racy      : %lld  %s\n", soma, soma == res_esperado ? "ok" : "ERRADO");

        // versão sem condição de corrida usando reduction
        long long soma_r = 0;
        // operadores possíveis pro reduction: + * - & | ^ && || min max
        #pragma omp parallel for reduction(+ : soma_r)
        /*
            Com reduction, cada thread após o fork recebe sua própria cópia de soma_r, com o valor sendo o elemento neutro do operador recebido na diretiva
            pra sua fatia do laço, e soma. Ao final (na barreira implícita) pega todas soma_r parciais e soma na soma_r original.
            Ou seja, se soma_r original já tivesse algum valor (1 ou 100), o resultado das operações realizadas seria aplicado em cima desse valor original
        */
        for (long long i =1; i <= N; i++) {
            soma_r += i;
        }
        printf("reduction : %lld %s\n", soma_r, soma_r == res_esperado ? "ok" : "ERRADO");
    }
}