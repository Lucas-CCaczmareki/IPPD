#include <cstdio>
#include <cstdlib>
#include <omp.h>
#include <vector>

int main(int argc, char* argv[]) {
    // Esse exercício é sobre proteger uma variável compartilhada, existem 3 jeitos, qual é o melhor em cada contexto?

    double t0, t1;

    if (argc < 2) {
        printf("error. missing operand\n");
        return 1;
    }

    int contador = 0; // variável shared entre as threads
    long long n = atoll(argv[1]);

    // Versão A: sem proteção nenhuma
    contador = 0;

    t0 = omp_get_wtime();

    // Aqui só vai dar erro se eu tirar a flag -O2 de otimização na hora de compilar
    // caso contrário ele faz a mágica dele e ai não dá erro.
    # pragma omp parallel for
    for (long long i = 0; i < n; ++i) {
        contador++; 
    }
    t1 = omp_get_wtime();
    printf("A (sem sync): %lld\nTempo: %.4f\n%s", contador, t1-t0, n == contador ? "ok\n" : "ERRO\n");

    // Versão B: versão com critical
    contador = 0;
    t0 = omp_get_wtime();
    # pragma omp parallel for
    for (long long i = 0; i < n; ++i) {

        // Essa área do bloco fica trancada e apenas uma thread executa o bloco por vez
        // as outras threads ficam esperando até a que tá fazendo termine
        # pragma omp critical
        contador++;
    } 
    t1 = omp_get_wtime();
    printf("B (seção critica): %lld\nTempo: %.4f\n%s", contador, t1-t0, n == contador ? "ok\n" : "ERRO");

    // Versão C: versão com atomic (descobrir o que atomic faz)
    t0 = omp_get_wtime();
    contador = 0;
    # pragma omp parallel for
    for (long long i = 0; i < n; ++i) {

        // troca as 3 operações (leitura, soma, escrita) por 1 única operação de hardware, 
        // que bloqueia os outros núcleos até finalizar. 
        // É barata e rápida, mas só funciona pra instruções atômicas que o hardware consegue fazer
        # pragma omp atomic
        contador++;
    }
    t1 = omp_get_wtime();
    printf("C (instrucao atomica): %lld\nTempo: %.4f\n%s", contador, t1-t0, n == contador ? "ok\n" : "ERRO");

    // a) A não vai dar o resultado correto por causa da condição de corrida
    // b) B e C vão dar o resultado correto
    // c) B impõe exclusão mútua sobre as threads
    // d) C protege a área de memória que vai ser afetada por operações simples (atômicas)
    // e) Atomic serve melhor nesse caso por que a operação de incremento pode ser realizada rapidamente por hardware, blqoueando os outros núcleos
    

    // extensão 
    std::vector<double> v(n, 1.0);
    long long sum = 0;

    // mesma lógica de antes, cada thread recebe um bloco e tem uma versão privada parcial de sum que inicializa com o número nulo da operação
    # pragma omp parallel for reduction (+ : sum)
    for(long long i = 0; i < n; ++i) {
        sum += v[i];
    }

    printf("soma: %lld");



    // contador = 0;
    // long long soma = 0;
    // # pragma omp parallel for
    // for (long long i = 0; i < n; ++i) {
    //     // o atomic só garante um único comando atualizando uma variável
    //     // no caso de um bloco de instruções, pode ser separado em 2 atomics
    //     // porém 2 atomics não garantem que os valores vão ser atualizados ao mesmo tempo/em sequência
    //     // pra isso seria necessário exclusão mútua com critial


    //     # pragma omp critical // área de exclusão mútua, permite bloco de código
    //     {
    //         soma += i;
    //         contador++;
    //         // printf("print do bloco crit\n");
    //     }

    //     // atomic consegue lidar com apenas uma instrução
    //     # pragma omp atomic
    //     soma += i;

    //     # pragma omp atomic
    //     contador++;

    //     // nem toda instrução é atomica, isso aqui chama I/O e faz trocentas coisas, então nao consegue ser realizada por hardware com atomic
    //     // printf("print do bloco\n");
    // }


    return 0;
}