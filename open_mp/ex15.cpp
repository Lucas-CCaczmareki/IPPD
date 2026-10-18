#include <cstdio>
#include <cstdlib>
#include <omp.h>
#include <iostream>

// eu entendi a moral/teoria e como funciona
// porém faltaria finalizar o código direitinho, mas vou pular essa parte devido ao tempo
// se der, eu volto aqui depois

int main(int argc, char* argv[]) {
    int x = 100;
    // instancia o pool de threads (processos rodando no SO)
    # pragma omp parallel
    {
        // a diretiva single faz com que uma thread fique responsável pelo bloco. Que nesse caso cria as tasks e coloca elas na fila.
        // executa o bloco a 1a thread q chegar nele, e só essa thread executa o bloco, as outras vão até o final do single e esperam
        // a thread que tá executando o bloco do single terminar
        # pragma omp single
        {
            std::cout << "THREAD CRIADORA: " << omp_get_thread_num() << "\n";
            for (int i = 0; i < 12; i++) {
                    // cada task é só um "push" com um bloco de código numa fila. Quase não precisa alocar memória separada, organizar pool, etc.
                    // ou seja, o overhead de criação é bem menor do que de uma thread
                    
                    // # pragma omp task firstprivate(i) 
                    // nesse caso (pragma omp task firstprivate(i)), como i é privado da task, ele já seria firstprivate por padrão na task
                    // caso contrário, ele seria tratado como shared

                    # pragma omp task firstprivate(x)
                    {
                        // esse bloco de código vai ficar na fila, liberado para que qualquer thread da área paralela pegue e execute ele.
                        // as tasks são importantes quando o trabalho paralelizável não tem número de iterações conhecido (em recursões por exemplo)
                        // nesses casos de recursão, não dá pra saber pra qual thread enviar o trabalho, então cria-se tasks, que a thread livre pega pra executar.

                        
                        # pragma omp critical // pras threads n executarem 1 pedaço do print de cada vez e ficar tudo misturado na saída
                        {
                            // como cada task é um bloco de código independente, se x for first private ele vai sempre printar "101". 
                            // Pois inicia com 100 e só é incrementado 1 vez nesse bloco
                            // Porém, se fosse shared, ele seria incrementado 1x pra cada vez que a task executou
                            x++; 

                            std::cout << "x task = " << x << "\n";
                            std::cout << "tarefa " << i << " executada pela thread " << omp_get_thread_num() << "\n";
                        }
                    }
            }
        } // single tem uma barreira implícita, as threads que não pegaram a execução ficam esperando a que pegou
        // pode ser desativando usando "nowait". Elas também ficam livres pra executarem tasks
    }

    std::cout << "x After parallel = " << x << "\n";
    std::cout << "\n\n\n";

    // # pragma omp parallel
    // {
    //     // retirando o single, todas threads vão executar esse bloco de código
    //     // o que significa um for de 12 vezes rodando 4x. 48 tasks criadas, 12 por cada task
    //     // e em seguida executadas em ordem de chegada pelas threads que ficam livres.
    //     // # pragma omp single
    //     // {
    //         for (int i = 0; i < 12; i++) {
    //             std::cout << "THREAD CRIADORA: " << omp_get_thread_num() << "\n";
    //             # pragma omp task
    //             {
    //                 std::cout << "tarefa " << i << " executada pela thread " << omp_get_thread_num() << "\n";
    //             }
    //         }
    //     // }
    // }

    return 0;
}
