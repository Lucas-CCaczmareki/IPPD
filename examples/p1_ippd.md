# Prova 1
## Revisão AOC
- Arquiteturas paralelas e distribuídas
- Arquitetura de von Neumann
- Classificação (nao faço ideia do que) quanto aos fluxos de execução, quanto a estrutura de organização dos componentes
e quanto à granularidade
- SISD SIMD, MISD MIMD, UMA, NUMA e uma gigante sopa de letrinha relacionada (traz tudo q for pertinente e explica, q eu n sei nada)
- granularidade quanto à instrução(pipeline, vetorial, superescalar), fluxo de execução (smt, multicore multiprocessador), 
processo(clusters, bewolf, aglomerados, constelações, MPP, grid computing) e aplicação (rede e claud computing)

## Revisão de SO
- processo, imagem do programa em execução
- escalonamento
- processo multithread
- processo (leve e pesado) e thread
- implementação de threads (1 to 1, n to m)
- comunicação entre processos IPC

## Modelos de programação
- concorrente vs paralelo vs distribuido
- onde reside concorrencia, paralelismo de tarefas ou dados
- multiprocessador vs multicomputador
- ferramentas de programação
- (atividade) tutorial de pthreads e mpi (seja lá oq é mpi). Compilar e executar algo

## Programação multithread
- arquiteturas multiprocessadas
- thread vs multiprocessador
- modelos de arquitetura (n tenho ideia doq ele ta falando pra descrever além disso, tem a ver com multiprocessador e threads)
- implementação de threads (1:1 e N:M)
- questões de programnação, unidade de trabalho, comunicação, sincronização e essas 3 coisas relacionando à task vs thread
- manipulação de UTs (q porra é uma ut velho), parbegin/parend, for/join
- explica comunicaçção com esses parbegin e forkjoin
- explica sincronização com mutex e monitor, alguma coisa de controle de fluxo e ai ele traz proberen e verhogen, algo a ver com o dijkstra
- ai tem um exercício pra usar um segundo semáforo e impedir q duas uts insiram elementos acima do buffer, e justificar pq o mutex é ou não necessário.
- algo sobre contenção dos mecanismos de sincronização
- coordenação e sincroniuzação
- spin lock, thread aware, instruções atômicas, aglorimtos não bloqueantes

## OpenMP
- conceitos fundamentais do open mp pra desenvolver programas
- oq é o openmp, como surgiu, como evoluiu, relação dele com pthreads, modelo de threads do open mp
- unidade de trabalho tarefa, unidade de escalonamento, modelo de execução do open mp, conceitos de região paralela
- semanticas pra ciração de tarefas, mecanismos de escalonamento worksharing e recursivo
- modelo básico do open mp
- como funciona a api do openmp, variaveis de ambiente, serviços de biblioteca, diretivas
- região paralela no open mp
- definição do corpo de tarefas em open mp (normal, private,  master/single, sections, for, schedule) algo sobre chunks
- algoritmos recursivos, task/taskwait
- compartilhamento de dados (modelo de memória, parametros as tarefas)
- clausulas, shared, private, firstprivate, last private, reduction
- escalonamento, worksharing, condição de barreira, nowait, laço paralelo, static, dynamic, guided, task/taskwait de novo, 
- exercício1: Crie um programa que receba como entrada o número de threads a ser criado no pool de execução
e o tamanho de um vetor de inteiros. Inicialmente o vetor deve ser criado e, então, inicializado com
valores quaisquer. Este programa deve imprimir, como saída, o somatório de todos os valores do
vetor. Avalie o desempenho do programa desenvolvido variando o número de threads no pool de execução e o tamanho do vetor
- exercício2: Implemente duas versões de um algoritmo recursivo para
cálculo de Fibonacci: Utilizando a criação de tarefas recursivas utilizando task/taskwait. Utilizando sections/section.
Compare o desempenho das implementações realizadas, variando a posição n calculado na série e o número de threads no pool de execução.

# ainda vai ficar manycore e sistemas distruibuidos RPC/RMI, mas acho qele n deu aula disso ainda, ent fodase por enquanto. Dps eu te peço pra fazer do resto