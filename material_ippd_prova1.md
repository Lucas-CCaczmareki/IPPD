# Material de Estudo — Prova 1
### Arquiteturas Paralelas e Distribuídas / Programação Concorrente

> Organização didática: primeiro os fundamentos de arquitetura (de onde vem a necessidade de paralelismo), depois SO (processos e threads), depois os modelos de programação, depois a mecânica de programação multithread (sincronização) e por fim OpenMP, que é a aplicação prática de tudo isso.

---

## Sumário

1. [Fundamentos de Arquitetura de Computadores](#1-fundamentos-de-arquitetura-de-computadores)
2. [Revisão de Sistemas Operacionais](#2-revisão-de-sistemas-operacionais)
3. [Modelos de Programação Paralela e Distribuída](#3-modelos-de-programação-paralela-e-distribuída)
4. [Programação Multithread](#4-programação-multithread)
5. [OpenMP](#5-openmp)
6. [Programação em Ambiente com Memória Distribuída](#6-programação-em-ambiente-com-memória-distribuída)
7. [RPC e RMI](#7-rpc-e-rmi)
8. [Sistemas Distribuídos: Conceitos e Arquitetura](#8-sistemas-distribuídos-conceitos-e-arquitetura)
9. [Quadros-resumo e questões de revisão](#9-quadros-resumo-e-questões-de-revisão)


---

## 1. Fundamentos de Arquitetura de Computadores

### 1.1 Arquitetura de von Neumann

É o modelo clássico (1945, John von Neumann) que descreve como praticamente todo computador digital "convencional" é organizado. Ideia central: **programa e dados ficam na mesma memória**, representados da mesma forma (bits), e são processados sequencialmente.

Componentes principais:

- **CPU (Unidade Central de Processamento)**
  - **UC — Unidade de Controle**: busca e decodifica instruções, gera sinais de controle.
  - **ULA — Unidade Lógica e Aritmética**: executa operações aritméticas/lógicas.
  - **Registradores**: memória rápida interna à CPU (PC, IR, acumuladores etc.).
- **Memória**: armazena instruções e dados no mesmo espaço de endereçamento.
- **Unidade de E/S (Entrada/Saída)**.
- **Barramento (bus)**: liga CPU, memória e E/S, transportando dados, endereços e controle.

Ciclo básico de execução: **busca (fetch) → decodificação (decode) → execução (execute)**, repetido para cada instrução.

**Gargalo de von Neumann**: como CPU e memória são interligadas por um único barramento, a velocidade de transferência de dados entre elas limita o desempenho — a CPU frequentemente fica esperando dados/instruções chegarem da memória. Esse gargalo é uma das motivações históricas para arquiteturas paralelas (pipeline, cache, múltiplos núcleos, memória distribuída etc.), já que apenas aumentar a frequência do processador esbarra em limites físicos (dissipação de calor, consumo de energia).

### 1.2 Por que arquiteturas paralelas e distribuídas?

- **Arquitetura paralela**: múltiplas unidades de processamento cooperam, geralmente próximas fisicamente (mesmo chip, mesma máquina), muitas vezes compartilhando memória.
- **Arquitetura distribuída**: múltiplos nós de processamento autônomos (com sua própria memória), conectados por rede, que cooperam para resolver um problema comum, trocando mensagens.

Motivações: limite físico de clock, necessidade de mais desempenho (HPC, big data, IA), tolerância a falhas, escalabilidade e custo (é mais barato juntar várias máquinas comuns do que construir uma supermáquina).

### 1.3 Classificações de Arquiteturas Paralelas

Existem três eixos de classificação cobrados: **(a)** fluxo de execução, **(b)** organização/estrutura dos componentes (memória) e **(c)** granularidade do paralelismo.

#### (a) Quanto ao fluxo de execução — Taxonomia de Flynn

Classificação clássica (Michael Flynn, 1966), baseada em dois fluxos: **fluxo de instruções** e **fluxo de dados**.

| Sigla | Significado | Descrição | Exemplo |
|---|---|---|---|
| **SISD** | Single Instruction, Single Data | Um processador executa uma instrução por vez sobre um dado por vez. É o computador sequencial clássico (von Neumann puro). | PC antigo monoprocessado |
| **SIMD** | Single Instruction, Multiple Data | Uma única instrução é aplicada simultaneamente a vários dados diferentes. Ótimo para processamento vetorial/matricial. | GPUs, extensões vetoriais (SSE/AVX), processadores vetoriais |
| **MISD** | Multiple Instruction, Single Data | Vários processadores executam instruções diferentes sobre o **mesmo** dado. Categoria rara/pouco usada comercialmente. | Sistemas tolerantes a falha (múltiplos algoritmos validando o mesmo dado, ex. controle de voo redundante) |
| **MIMD** | Multiple Instruction, Multiple Data | Vários processadores executam instruções diferentes sobre dados diferentes, de forma independente. Categoria mais comum hoje. | Multiprocessadores, multicomputadores, clusters |

**MIMD** é subdividido, quanto ao modo de programação, em:
- **SPMD (Single Program, Multiple Data)**: todos os processadores rodam o *mesmo programa*, mas cada um opera sobre uma parte diferente dos dados (ex.: MPI e OpenMP geralmente programados nesse estilo).
- **MPMD (Multiple Program, Multiple Data)**: processadores diferentes rodam *programas diferentes* cooperando entre si.

#### (b) Quanto à organização/acesso à memória

Essa classificação descreve como os processadores enxergam a memória:

- **UMA (Uniform Memory Access)**: todos os processadores compartilham uma única memória física, e o tempo de acesso a qualquer posição de memória é o **mesmo** para todos os processadores. Também chamado de **SMP (Symmetric Multiprocessing)**. Simples de programar, mas escala mal (barramento/memória vira gargalo com muitos núcleos).
- **NUMA (Non-Uniform Memory Access)**: cada processador (ou grupo de processadores) tem memória "local", de acesso rápido, mas também pode acessar a memória "remota" de outros processadores, com latência maior. Escala melhor que UMA, mas o desempenho depende de onde o dado está fisicamente.
- **NORMA (No Remote Memory Access)**: nenhum processador acessa diretamente a memória de outro — cada nó só enxerga sua própria memória local, e a comunicação só ocorre via troca explícita de mensagens (rede). É o modelo típico de **memória distribuída** (clusters, multicomputadores).
- **COMA (Cache-Only Memory Access)**: variação onde toda a memória principal funciona como cache, dados migram fisicamente para perto de quem os usa (menos comum, citado por completude).

Resumo mental:
- UMA/NUMA → **memória compartilhada** (multiprocessadores).
- NORMA → **memória distribuída** (multicomputadores/clusters).

#### (c) Quanto à granularidade

Granularidade = o "nível" em que o paralelismo é explorado, do mais fino (dentro de uma instrução) ao mais grosso (entre aplicações inteiras).

**1. Nível de instrução** (paralelismo dentro da execução de instruções de um único fluxo):
- **Pipeline**: a execução de uma instrução é dividida em estágios (busca, decodificação, execução, escrita etc.), e vários estágios de instruções diferentes ocorrem simultaneamente, como uma linha de montagem.
- **Processamento vetorial**: uma instrução opera sobre um vetor inteiro de dados de uma vez (relaciona-se ao SIMD).
- **Superescalar**: o processador possui múltiplas unidades funcionais e consegue despachar/executar mais de uma instrução por ciclo de clock, mesmo em um único núcleo.

**2. Nível de fluxo de execução** (paralelismo entre threads/processos dentro de uma mesma máquina):
- **SMT (Simultaneous Multithreading)**: um único núcleo físico executa instruções de múltiplas threads simultaneamente, aproveitando unidades funcionais ociosas (ex.: Hyper-Threading da Intel).
- **Multicore**: múltiplos núcleos completos de processamento dentro do mesmo chip.
- **Multiprocessador**: múltiplos processadores físicos (chips) numa mesma máquina, compartilhando memória.

**3. Nível de processo** (paralelismo entre máquinas cooperando):
- **Clusters**: conjunto de computadores independentes, interligados em rede, que trabalham juntos como se fossem um único sistema.
- **Beowulf**: tipo específico de cluster montado com computadores comuns (commodity hardware) e software livre, criado como alternativa barata a supercomputadores.
- **Aglomerados / constelações**: termos usados para agrupamentos de computadores (ou de clusters) trabalhando de forma coordenada; "constelação" às vezes designa um cluster de multiprocessadores (nós SMP interligados).
- **MPP (Massively Parallel Processing)**: sistema com um número muito grande de processadores, cada um com sua própria memória e SO, interligados por uma rede de alta velocidade dedicada — mais integrado/proprietário que um cluster comum.
- **Grid Computing**: agrega recursos computacionais **geograficamente dispersos e heterogêneos** (podem pertencer a organizações diferentes), coordenados para resolver problemas grandes, geralmente de forma mais fracamente acoplada que um cluster.

**4. Nível de aplicação**:
- **Network computing**: aplicações que exploram recursos distribuídos na rede (não necessariamente dedicados) para processar tarefas.
- **Cloud computing**: fornecimento de recursos computacionais (processamento, armazenamento, plataformas, software) sob demanda, via internet, tipicamente virtualizados e elásticos (escalam conforme a necessidade).

---

## 2. Revisão de Sistemas Operacionais

### 2.1 Processo

**Processo** = um programa em execução. Não confundir com o programa (que é apenas código armazenado em disco, passivo); o processo é uma **entidade ativa**.

O SO representa um processo através de uma estrutura de dados chamada **PCB (Process Control Block)**, contendo: estado do processo, PC (program counter), registradores, informações de escalonamento, informações de memória, lista de arquivos abertos etc.

A **imagem do programa em execução** contém tipicamente:
- **Código (text)**: as instruções.
- **Dados (data/BSS)**: variáveis globais e estáticas.
- **Heap**: memória alocada dinamicamente.
- **Stack (pilha)**: variáveis locais, contexto de chamadas de função.

### 2.2 Escalonamento (Scheduling)

É a atividade do SO de decidir **qual processo/thread executa e por quanto tempo**, já que normalmente há mais processos do que processadores disponíveis.

Conceitos-chave:
- **Preemptivo**: o SO pode interromper um processo em execução para dar a vez a outro (ex.: fim de fatia de tempo/*time slice*).
- **Não-preemptivo (cooperativo)**: o processo só libera o processador voluntariamente (termina ou bloqueia).
- Critérios de qualidade: utilização da CPU, vazão (*throughput*), tempo de espera, tempo de resposta, justiça (*fairness*).
- Alguns algoritmos clássicos (contexto geral): FCFS (primeiro a chegar, primeiro a ser servido), Round-Robin (fatias de tempo), prioridades, filas multinível.

Em sistemas paralelos, o escalonamento também decide **em qual núcleo/processador** cada thread roda (afinidade de processador), o que impacta diretamente o desempenho (ex.: em NUMA, importa colocar a thread perto da memória que ela usa).

### 2.3 Processos, Threads e Multithreading

- **Thread** (linha de execução): a menor unidade de execução que pode ser escalonada pelo SO. Possui seu próprio contexto mínimo (PC, registradores, pilha), mas **compartilha** com as outras threads do mesmo processo o espaço de endereçamento (código, dados, heap, arquivos abertos).
- **Processo multithread**: um processo que contém múltiplas threads executando concorrentemente dentro dele, compartilhando os mesmos recursos (memória, arquivos).

**Processo pesado vs. processo leve vs. thread**:
- **Processo pesado (heavyweight process)**: unidade tradicional com seu próprio espaço de endereçamento isolado. Criar/trocar de contexto entre processos pesados é caro (precisa trocar tabelas de páginas, invalidar cache/TLB etc.).
- **Processo leve (lightweight process, LWP)**: termo usado para uma thread, justamente por ser "mais barata" de criar e escalonar que um processo pesado — não precisa duplicar o espaço de endereçamento inteiro.
- Ou seja: **thread ≈ processo leve**, em oposição ao processo tradicional ("pesado").

Vantagens de usar threads em vez de múltiplos processos: criação mais rápida, troca de contexto mais barata, comunicação mais simples (memória compartilhada em vez de IPC), melhor uso de múltiplos núcleos dentro de uma mesma aplicação.

### 2.4 Implementação de Threads

Como as threads de nível de usuário se relacionam com as threads reconhecidas pelo *kernel* do SO (que são as que efetivamente rodam num processador):

- **Modelo 1:1 (kernel-level threading)**: cada thread de usuário corresponde a uma thread de kernel. O SO conhece e escalona cada thread individualmente. Vantagem: aproveita bem múltiplos núcleos e paralelismo real; se uma thread bloqueia (ex.: I/O), as demais continuam. Desvantagem: criação/troca de contexto tem overhead do kernel (é o modelo usado por Pthreads/Linux atualmente).
- **Modelo N:1 (user-level threading)**: várias threads de usuário são mapeadas para **uma única** thread de kernel; o escalonamento entre elas é feito por uma biblioteca em espaço de usuário, sem envolver o SO. Vantagem: troca de contexto muito rápida (não entra em modo kernel). Desvantagem: se uma thread bloqueia numa chamada de sistema, **todas** as threads do processo bloqueiam; não aproveita múltiplos núcleos.
- **Modelo N:M (many-to-many)**: N threads de usuário são multiplexadas sobre M threads de kernel (M ≤ N), combinando as vantagens dos dois modelos anteriores — o SO enxerga várias threads de kernel (paralelismo real em múltiplos núcleos) mas ainda há flexibilidade e leveza no espaço de usuário. É o modelo mais sofisticado, porém mais complexo de implementar corretamente.

### 2.5 Comunicação entre Processos (IPC — Inter-Process Communication)

Mecanismos para processos (que não compartilham memória, ao contrário de threads) trocarem dados e se sincronizarem:

- **Pipes**: canal de comunicação unidirecional entre processos relacionados (ex.: `|` no shell).
- **Named pipes (FIFOs)**: como pipes, mas com um nome no sistema de arquivos, permitindo comunicação entre processos não relacionados.
- **Filas de mensagens (message queues)**: mensagens estruturadas são colocadas numa fila gerenciada pelo SO.
- **Memória compartilhada (shared memory)**: uma região de memória é mapeada no espaço de endereçamento de mais de um processo — a forma mais rápida de IPC, mas exige sincronização explícita (semáforos, mutexes) para evitar condições de corrida.
- **Sockets**: comunicação via rede (ou localmente), usada tanto entre processos na mesma máquina quanto em máquinas diferentes — base da comunicação em sistemas distribuídos.
- **Sinais (signals)**: notificações assíncronas simples enviadas a um processo (ex.: `SIGKILL`, `SIGTERM`).
- **Semáforos**: usados majoritariamente para *sincronização* entre processos que compartilham algum recurso (ver seção 4.7).

---

## 3. Modelos de Programação Paralela e Distribuída

### 3.1 Concorrente vs. Paralelo vs. Distribuído

Esses três termos são frequentemente confundidos, mas descrevem coisas diferentes:

- **Concorrência**: propriedade de um sistema em que **múltiplas tarefas progridem em um intervalo de tempo sobreposto**, não necessariamente ao mesmo instante físico. Pode ocorrer até em uma única CPU, via *time-sharing* (o SO alterna rapidamente entre as tarefas, dando a ilusão de simultaneidade). Concorrência é sobre **estrutura**: como organizar um programa em partes que podem ser executadas de forma intercalada.
- **Paralelismo**: propriedade de um sistema em que múltiplas tarefas **executam literalmente ao mesmo tempo**, em hardware distinto (múltiplos núcleos/processadores). Paralelismo é sobre **execução simultânea real**, e exige hardware com múltiplas unidades de processamento.
- **Sistema distribuído**: múltiplas máquinas autônomas (cada uma com seu processador e memória própria), que não compartilham memória física, cooperando através de troca de mensagens pela rede para atingir um objetivo comum, aparecendo para o usuário, idealmente, como um único sistema coerente.

Relação entre os três: todo programa paralelo é concorrente (tem partes que progridem "juntas"), mas nem todo programa concorrente é paralelo (pode rodar concorrente em uma única CPU sem paralelismo real). Sistemas distribuídos são inerentemente concorrentes e frequentemente também paralelos (várias máquinas processando ao mesmo tempo), mas se diferenciam pela ausência de memória compartilhada e pela comunicação via rede.

### 3.2 Onde reside a concorrência/paralelismo

Ao paralelizar um problema, o paralelismo pode ser explorado de duas formas principais:

- **Paralelismo de tarefas (task parallelism)**: diferentes unidades de execução realizam **tarefas/funções diferentes**, possivelmente sobre os mesmos ou diferentes dados (ex.: um pipeline de processamento de imagem onde uma thread faz o filtro A e outra o filtro B).
- **Paralelismo de dados (data parallelism)**: a **mesma operação** é aplicada simultaneamente a **diferentes pedaços dos dados** (ex.: somar partes diferentes de um vetor em paralelo). É o modelo mais comum e mais fácil de escalar (relaciona-se com SIMD/SPMD).

### 3.3 Multiprocessador vs. Multicomputador

- **Multiprocessador**: sistema com múltiplos processadores que **compartilham a mesma memória física** (UMA ou NUMA). A comunicação entre processadores é feita, na prática, por leitura/escrita direta em posições de memória compartilhada. É o modelo natural para programação com **threads**.
- **Multicomputador**: conjunto de computadores completos e independentes (cada um com sua própria memória — modelo NORMA), interligados por rede, sem memória compartilhada. A comunicação só ocorre por **troca explícita de mensagens**. É o modelo natural para programação com **passagem de mensagens** (MPI) e é a base de clusters e sistemas distribuídos.

### 3.4 Ferramentas de Programação Paralela

- **Pthreads (POSIX Threads)**: biblioteca padrão (padrão POSIX) para criação e gerenciamento de threads em C/C++ — modelo de **memória compartilhada**, indicado para multiprocessadores/multicore.
- **OpenMP**: API de mais alto nível que Pthreads, baseada em diretivas de compilador, para paralelismo em memória compartilhada (ver seção 5).
- **MPI (Message Passing Interface)**: é um **padrão/biblioteca** (não uma linguagem) para programação com **passagem de mensagens** entre processos que podem estar em máquinas diferentes (memória distribuída) — usado para programar clusters e multicomputadores. Cada processo MPI tem seu próprio espaço de memória e se comunica via funções como `MPI_Send` e `MPI_Recv`.
- Outras (contexto): **CUDA/OpenCL** (paralelismo em GPU), **RPC/RMI** e sockets (para sistemas distribuídos — ficam para depois, conforme você mencionou).

### 3.5 Atividade prática — Compilando e executando Pthreads e MPI

**Pthreads** (compartilha memória, roda em uma única máquina):

```c
#include <pthread.h>
#include <stdio.h>

void *ola(void *arg) {
    printf("Ola da thread!\n");
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, ola, NULL);
    pthread_join(t, NULL);
    return 0;
}
```
Compilar (é necessário linkar a biblioteca pthread com `-lpthread` ou `-pthread`):
```bash
gcc programa.c -o programa -lpthread
./programa
```

**MPI** (passagem de mensagens, pode rodar em vários processos/máquinas):

```c
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("Ola do processo %d\n", rank);
    MPI_Finalize();
    return 0;
}
```
Compilar e executar (com `N` processos):
```bash
mpicc programa.c -o programa
mpirun -np 4 ./programa
```

---

## 4. Programação Multithread

### 4.1 e 4.2 Arquiteturas multiprocessadas, thread vs. multiprocessador

Uma arquitetura multiprocessada fornece o **hardware** (vários núcleos/processadores, memória compartilhada — UMA/NUMA) sobre o qual o **software** explora paralelismo através de threads. Ou seja: a thread é a unidade de **software** que o programador cria e coordena; o multiprocessador é o **hardware** subjacente que permite que essas threads realmente executem simultaneamente (em vez de apenas concorrentemente/intercaladas). Sem um multiprocessador real (múltiplos núcleos), múltiplas threads ainda trazem benefícios de concorrência (ex.: uma thread não bloqueia as outras em I/O), mas não trazem paralelismo verdadeiro.

### 4.3 Modelos de arquitetura relacionados a threads

Ligando de volta à seção 1.3(b): o modelo de memória do multiprocessador (UMA ou NUMA) impacta diretamente como threads devem ser programadas/otimizadas — em NUMA, por exemplo, é importante que uma thread trabalhe preferencialmente com dados alocados "perto" (no nó de memória local ao núcleo em que ela roda), evitando acessos remotos mais lentos.

### 4.4 Implementação de threads (1:1 e N:M)

Já detalhado na seção 2.4 — revisão do SO aplicada diretamente ao contexto de programação multithread: a forma como as threads da sua aplicação (ex.: criadas via Pthreads) são mapeadas para threads de kernel (1:1) ou multiplexadas (N:M) afeta o quanto de paralelismo real sua aplicação consegue explorar.

### 4.5 Questões de programação concorrente: Unidade de Trabalho, Comunicação e Sincronização

Ao programar de forma concorrente/paralela, três questões fundamentais precisam ser resolvidas:

**a) Unidade de Trabalho (UT)**: é o termo genérico para "aquilo que executa de forma independente" — pode ser implementada como um **processo** ou como uma **thread** (ou, em OpenMP, uma **task**). Usa-se o termo "UT" justamente para falar de forma abstrata sobre concorrência sem se prender à implementação específica (processo pesado vs. leve).
- **Task vs. Thread**: uma *task* é uma unidade de trabalho **lógica**, um "pedaço de código a ser executado" que ainda não está necessariamente amarrado a uma thread específica — o sistema de execução (runtime) é quem decide em qual thread (de um pool, por exemplo) a task efetivamente vai rodar. Uma *thread*, por outro lado, é a unidade de execução do SO. Em OpenMP, por exemplo, você cria *tasks*, e o runtime as distribui entre um conjunto (pool) de threads.

**b) Comunicação**: como as UTs trocam informação entre si (ver 4.6 — parbegin/parend, fork/join; em memória compartilhada, normalmente via variáveis compartilhadas).

**c) Sincronização**: como garantir que as UTs coordenem o acesso a recursos compartilhados e a ordem de certos eventos, evitando condições de corrida (*race conditions*) (ver 4.7).

### 4.6 Manipulação de Unidades de Trabalho

Formas clássicas de expressar criação/término de UTs em paralelo:

- **parbegin / parend** (também chamado *cobegin/coend*): construção que delimita um bloco de instruções que devem ser executadas **em paralelo**. Tudo entre `parbegin` e `parend` roda concorrentemente, e a execução só prossegue depois do `parend` quando **todas** as UTs daquele bloco terminarem. É uma forma estruturada e de alto nível de expressar paralelismo (conceito teórico, presente em pseudocódigos de livros-texto).

  ```
  parbegin
      tarefaA();
      tarefaB();
      tarefaC();
  parend
  // só chega aqui quando A, B e C tiverem terminado
  ```

- **fork/join**: modelo mais próximo do que bibliotecas reais implementam (Pthreads, OpenMP).
  - **fork**: um fluxo de execução "principal" cria (dispara) uma ou mais UTs novas, que passam a rodar concorrentemente com quem as criou.
  - **join**: o criador espera até que a(s) UT(s) criada(s) terminem, sincronizando a continuação do programa.
  - Exemplo direto: `pthread_create` (fork) e `pthread_join` (join, visto no código de exemplo da seção 3.5).

  Ambos os modelos (`parbegin/parend` e `fork/join`) resolvem o mesmo problema — expressar comunicação implícita de **quando algo começa e quando termina** em paralelo — mas `parbegin/parend` é uma notação estrutural/declarativa, enquanto `fork/join` é o padrão usado operacionalmente nas APIs reais.

### 4.7 Sincronização

Sincronização é necessária sempre que UTs concorrentes acessam **recursos compartilhados** (ex.: uma variável, um buffer), para evitar **condições de corrida** (resultado incorreto dependendo da ordem/intercalação da execução).

**Mutex (Mutual Exclusion)**: mecanismo mais simples — um "cadeado" binário que garante que **apenas uma** UT por vez execute uma seção crítica (trecho de código que acessa o recurso compartilhado). Uma UT faz `lock()` antes de entrar na seção crítica e `unlock()` ao sair; se outra UT tentar dar `lock()` enquanto já está travado, ela bloqueia até ser liberado.

**Monitor**: uma construção de mais alto nível (geralmente de linguagens orientadas a objetos, como em Java) que encapsula **dados compartilhados + os procedimentos que os acessam + a exclusão mútua**, de forma que o próprio compilador/runtime garante que só uma thread execute um método do monitor por vez. Monitores frequentemente incluem **variáveis de condição**, que permitem uma thread esperar (`wait`) até que uma condição se torne verdadeira, sendo acordada (`signal`/`notify`) por outra.

**Semáforos (P/V — Dijkstra)**: mecanismo proposto por **Edsger Dijkstra**, mais geral que um mutex — um semáforo é um contador inteiro com duas operações **atômicas**:
- **P (proberen, "testar/provar" em holandês)** — também chamada `wait()` ou `down()`: decrementa o contador; se o resultado for negativo, a UT bloqueia até que o valor volte a permitir a operação.
- **V (verhogen, "incrementar" em holandês)** — também chamada `signal()` ou `up()`: incrementa o contador, potencialmente liberando uma UT que estava bloqueada esperando em `P`.

Um **semáforo binário** (valores 0 ou 1) funciona como um mutex. Um **semáforo contador** (valores maiores) é usado para controlar acesso a um **conjunto de N recursos idênticos** (ex.: N vagas em um buffer), o que o mutex sozinho não resolve bem.

**Exercício clássico — Produtor/Consumidor com buffer limitado**: usar **dois semáforos contadores** além de um mutex:
- `sem_vazio` (inicializado com **N**, o tamanho do buffer): conta quantos espaços livres existem. Produtor faz `P(vazio)` antes de inserir.
- `sem_cheio` (inicializado com **0**): conta quantos itens estão prontos para consumo. Consumidor faz `P(cheio)` antes de retirar.
- `mutex` (binário, inicializado com **1**): protege o acesso à estrutura do buffer em si (índices, ponteiros), garantindo que inserções/remoções não corrompam a estrutura se ocorrerem "ao mesmo tempo".

Pseudocódigo do produtor:
```
produtor():
    loop:
        item = produzir()
        P(sem_vazio)     // espera haver espaço livre
        P(mutex)         // exclusão mútua na estrutura
        inserir(buffer, item)
        V(mutex)
        V(sem_cheio)     // sinaliza que há um item novo
```

**Por que o segundo semáforo (`sem_vazio`) impede que duas UTs insiram acima da capacidade do buffer?** Porque `sem_vazio` é inicializado com o **número exato de vagas livres (N)**. Cada `P(sem_vazio)` bem-sucedido consome uma "permissão de inserir", e se o buffer já estiver cheio (contador chegou a 0), qualquer nova UT que tente inserir ficará **bloqueada em `P(sem_vazio)`** até que um consumidor libere espaço com `V(sem_vazio)`. Ou seja, o controle de "cheio/vazio" é feito pelos semáforos contadores, e **não** pelo mutex.

**O mutex sozinho, sem os semáforos contadores, resolveria o problema?** Não. O mutex garante apenas que **duas UTs não mexam ao mesmo tempo na estrutura de dados do buffer** (exclusão mútua), evitando corrupção de índices/ponteiros. Mas ele **não sabe nada sobre "cheio" ou "vazio"** — ele não impede que uma UT execute a lógica de inserção quando o buffer já está cheio (ela pegaria o mutex, escreveria fora dos limites ou sobrescreveria dados, e liberaria o mutex normalmente, sem erro nenhum do ponto de vista da exclusão mútua). Por isso o mutex é **necessário mas não suficiente**: ele resolve o problema da integridade da estrutura compartilhada, mas não resolve o problema da **capacidade/condição lógica** (cheio/vazio), que é papel dos semáforos contadores.

### 4.8 Contenção dos mecanismos de sincronização

**Contenção** ocorre quando várias UTs tentam adquirir o **mesmo** mecanismo de sincronização (ex.: o mesmo mutex) ao mesmo tempo, e a maioria delas precisa esperar. Alta contenção é ruim para desempenho: as UTs ficam bloqueadas/ociosas em vez de fazer trabalho útil, e o overhead de gerenciar filas de espera e trocas de contexto aumenta. Reduzir contenção é um objetivo de projeto importante em programação paralela (ex.: granularidade mais fina de locks, estruturas *lock-free*, particionamento de dados para reduzir compartilhamento).

### 4.9 Coordenação e sincronização

**Sincronização** (no sentido estrito) geralmente se refere a **exclusão mútua** e ordenação de acesso a recursos compartilhados (o que vimos em 4.7). **Coordenação** é um conceito mais amplo: refere-se a fazer com que múltiplas UTs trabalhem de forma organizada para atingir um objetivo comum, o que inclui não só sincronização, mas também comunicação, divisão de trabalho e sequenciamento de etapas (ex.: uma barreira, que faz todas as UTs esperarem até que todas cheguem em um certo ponto antes de qualquer uma prosseguir, é um mecanismo de coordenação).

### 4.10 Spin lock, thread-aware, instruções atômicas, algoritmos não bloqueantes

- **Spin lock ("trava girando")**: forma de esperar por um lock em que a UT fica em um **laço ativo** (*busy-waiting*), testando repetidamente se o lock foi liberado, em vez de "dormir" e ser acordada depois. Vantagem: se a espera for muito curta, evita o custo de uma troca de contexto completa (dormir/acordar). Desvantagem: desperdiça ciclos de CPU se a espera for longa — por isso spin locks só compensam quando a seção crítica é muito curta e/ou o sistema tem núcleos sobrando.
- **Thread-aware**: diz respeito a mecanismos de sincronização/algoritmos que são projetados sabendo explicitamente que rodam em ambiente multithread (em oposição a código que assume execução sequencial) — ou seja, "cientes" da possibilidade de concorrência real e projetados para lidar com ela corretamente (evitando condições de corrida, deadlocks etc.).
- **Instruções atômicas**: instruções de hardware que executam como uma **operação indivisível** (não podem ser interrompidas no meio, nem sofrer interferência de outro núcleo), como `compare-and-swap (CAS)`, `test-and-set`, `fetch-and-add`. São a base sobre a qual mutexes, spin locks e outros mecanismos de sincronização de mais alto nível são construídos.
- **Algoritmos não bloqueantes (*lock-free / non-blocking*)**: algoritmos que garantem progresso do sistema como um todo **sem usar locks tradicionais** (que podem causar bloqueio, deadlock, ou perda de desempenho por contenção). Em vez disso, usam instruções atômicas (como CAS) diretamente para atualizar estruturas de dados compartilhadas, com re-tentativa em caso de conflito. Vantagem: evita deadlocks e certos problemas de contenção/prioridade invertida; desvantagem: são significativamente mais difíceis de projetar e provar corretos.

---

## 5. OpenMP

### 5.1 O que é, origem e evolução

**OpenMP (Open Multi-Processing)** é uma **API** (não uma linguagem nova) para programação paralela em **memória compartilhada**, para C, C++ e Fortran. Surgiu em 1997, criada por um consórcio de fabricantes de hardware/software (OpenMP Architecture Review Board), com o objetivo de **padronizar** e simplificar a programação paralela em multiprocessadores, que antes era feita de formas específicas de cada fabricante.

**Relação com Pthreads**: OpenMP é implementado, por baixo dos panos, geralmente **sobre threads de baixo nível (como Pthreads)** — ou seja, o compilador/runtime do OpenMP traduz as diretivas em chamadas de criação/gerenciamento de threads. A diferença é o **nível de abstração**: Pthreads exige que o programador gerencie manualmente cada `create`/`join`/mutex; OpenMP permite paralelizar código com **diretivas de compilador** (pragmas), com o runtime cuidando dos detalhes de baixo nível — muito mais produtivo, ainda que com menos controle fino que Pthreads.

Evolução: OpenMP começou apenas com paralelismo estruturado por laços (worksharing), e evoluiu (a partir da versão 3.0, 2008) para incluir o modelo de **tasks**, permitindo paralelizar algoritmos irregulares/recursivos que não se encaixam bem em laços `for` simples.

### 5.2 Modelo de execução: Fork-Join

OpenMP segue o modelo **fork-join** (ver seção 4.6):
1. O programa começa com **uma única thread**, chamada **thread mestre (master)**.
2. Ao encontrar uma **região paralela**, a thread mestre faz um **fork**: cria um **time de threads (thread team)**, formando um **pool de threads**.
3. Todas as threads do time executam o código dentro da região paralela.
4. Ao final da região paralela, ocorre um **join** implícito: as threads esperam umas pelas outras (barreira implícita) e o time é desfeito, voltando a haver apenas a thread mestre.

Isso pode se repetir várias vezes ao longo do programa (múltiplas regiões paralelas).

### 5.3 Unidade de trabalho, unidade de escalonamento e Região Paralela

- **Unidade de trabalho** em OpenMP é a **task** — um bloco de código (com seus dados associados) que pode ser executado por qualquer thread do time.
- **Unidade de escalonamento**: a **thread**, do ponto de vista de quem efetivamente executa; o runtime do OpenMP decide como distribuir as tasks/iterações entre as threads disponíveis do pool.
- **Região paralela**: o bloco de código delimitado pela diretiva `#pragma omp parallel { ... }`, que é executado por **todas as threads do time** (cada uma executa uma cópia do bloco inteiro, a menos que haja diretivas de *worksharing* dentro dele dividindo o trabalho).

  ```c
  #pragma omp parallel
  {
      // este bloco é executado por TODAS as threads do time
      printf("Ola da thread %d\n", omp_get_thread_num());
  }
  ```

### 5.4 A API do OpenMP: diretivas, cláusulas, variáveis de ambiente e biblioteca

OpenMP é composto de três partes:

1. **Diretivas de compilador (pragmas)**: instruções como `#pragma omp parallel`, `#pragma omp for`, `#pragma omp task` etc., que dizem ao compilador *onde* e *como* paralelizar.
2. **Rotinas de biblioteca (runtime)**: funções chamáveis em tempo de execução, como `omp_get_thread_num()` (retorna o ID da thread atual), `omp_get_num_threads()`, `omp_set_num_threads()`.
3. **Variáveis de ambiente**: configuram o comportamento do runtime sem alterar o código-fonte, como `OMP_NUM_THREADS` (define quantas threads usar) e `OMP_SCHEDULE` (define a política de escalonamento padrão do `for`).

### 5.5 Compartilhamento de dados: modelo de memória e cláusulas

No modelo de memória compartilhada do OpenMP, por padrão, **variáveis declaradas fora da região paralela são compartilhadas** entre todas as threads, a menos que se diga o contrário. Isso é controlado por **cláusulas** anexadas às diretivas:

- **`shared(lista)`**: a(s) variável(is) é(são) a **mesma posição de memória** para todas as threads (mudanças de uma são visíveis para as outras — exige cuidado com condições de corrida).
- **`private(lista)`**: cada thread recebe sua **própria cópia** da variável, **não inicializada** (o valor original de fora da região paralela é perdido, e o valor final também não "sai" da região).
- **`firstprivate(lista)`**: como `private`, mas cada cópia é **inicializada com o valor que a variável tinha antes** de entrar na região paralela.
- **`lastprivate(lista)`**: como `private`, mas o valor da **última iteração** (na ordem sequencial lógica do laço) é copiado de volta para a variável original após o laço terminar.
- **`reduction(operador:variável)`**: cada thread mantém sua própria cópia parcial da variável (inicializada de forma neutra para o operador — ex.: 0 para soma), realiza a operação localmente, e ao final o runtime **combina** (reduz) todas as cópias parciais usando o operador indicado (`+`, `*`, `max`, `min` etc.), atualizando a variável original. É a forma correta e eficiente de, por exemplo, somar um vetor em paralelo sem precisar de locks manuais.

  ```c
  int soma = 0;
  #pragma omp parallel for reduction(+:soma)
  for (int i = 0; i < n; i++) {
      soma += vetor[i];
  }
  ```

### 5.6 Mecanismos de escalonamento: Worksharing e Recursivo (Tasks)

OpenMP oferece dois grandes modelos para distribuir trabalho entre as threads do time:

**A) Worksharing** (distribuição de trabalho **estruturado**, tipicamente laços regulares):

- **`for`**: divide as iterações de um laço `for` entre as threads do time.
  ```c
  #pragma omp parallel for
  for (int i = 0; i < n; i++) { ... }
  ```
  A cláusula **`schedule`** controla *como* as iterações são divididas em **chunks** (blocos de iterações):
  - **`static [,chunk]`**: as iterações são divididas em chunks de tamanho fixo e distribuídas entre as threads **antes** da execução começar (de forma round-robin). Baixo overhead, mas pode gerar desbalanceamento se as iterações tiverem custo desigual.
  - **`dynamic [,chunk]`**: cada thread pega um novo chunk **da fila** assim que termina o anterior, dinamicamente durante a execução. Melhor balanceamento em cargas desiguais, mas com mais overhead (acesso à fila compartilhada).
  - **`guided [,chunk]`**: parecido com `dynamic`, mas o tamanho dos chunks começa grande e **diminui progressivamente** ao longo da execução — tenta equilibrar baixo overhead no início com bom balanceamento no fim.
- **`sections` / `section`**: permite atribuir **blocos de código diferentes** (não iterações de um laço) a threads diferentes — é a forma de expressar paralelismo de **tarefas** (não de dados) de forma estruturada.
  ```c
  #pragma omp parallel sections
  {
      #pragma omp section
      { tarefaA(); }
      #pragma omp section
      { tarefaB(); }
  }
  ```
- **`single`**: apenas **uma** das threads do time executa o bloco (a primeira que chegar); as demais esperam (há uma barreira implícita no final, a menos que se use `nowait`).
- **`master`**: apenas a thread **mestre** (thread 0) executa o bloco; diferente de `single`, **não há** barreira implícita — as outras threads simplesmente seguem em frente sem esperar.
- **Barreira e `nowait`**: a maioria das construções de worksharing (`for`, `sections`, `single`) tem uma **barreira implícita** ao final — todas as threads esperam até que todas terminem aquele bloco antes de prosseguir. A cláusula **`nowait`** remove essa barreira implícita, permitindo que threads que terminaram sigam para o próximo trecho de código sem esperar as demais (útil quando não há dependência entre os blocos).

**B) Modelo Recursivo — Tasks** (para paralelismo **irregular**, como algoritmos recursivos que dividem o problema em tamanhos desiguais e imprevisíveis, ex.: Fibonacci, quicksort, percurso de árvores):

- **`task`**: cria uma nova unidade de trabalho (task) que pode ser executada por **qualquer** thread do pool, imediatamente ou mais tarde, à escolha do runtime.
- **`taskwait`**: faz a thread atual esperar que **todas as tasks filhas criadas por ela** (diretamente) terminem, antes de continuar — análogo a um `join` local, essencial em algoritmos recursivos para sincronizar antes de combinar resultados (ex.: `fib(n-1) + fib(n-2)`).

  ```c
  int fib(int n) {
      if (n < 2) return n;
      int x, y;
      #pragma omp task shared(x)
      x = fib(n - 1);
      #pragma omp task shared(y)
      y = fib(n - 2);
      #pragma omp taskwait   // espera as duas tasks acima terminarem
      return x + y;
  }
  ```
  Observação importante: `task` só cria trabalho paralelo de fato dentro de uma região paralela já aberta (geralmente combinada com `single`, para que apenas uma thread dispare a árvore de tasks recursivas, e as demais do pool fiquem disponíveis para executá-las):
  ```c
  #pragma omp parallel
  {
      #pragma omp single
      resultado = fib(n);
  }
  ```

### 5.7 Passagem de parâmetros para tasks

Como as *tasks* podem ser executadas em qualquer momento por qualquer thread (até depois de a função que a criou já ter avançado), o OpenMP precisa decidir **quando** capturar o valor das variáveis usadas dentro da task — isso é controlado pelas mesmas cláusulas de compartilhamento de dados vistas na seção 5.5 (`shared`, `private`, `firstprivate`), aplicadas à diretiva `task`. Em geral, variáveis locais da função que cria a task tendem a ser **firstprivate por padrão** (cada task recebe uma cópia do valor no momento da criação), justamente para evitar que o valor mude antes da task realmente rodar.

### 5.8 Exercícios de fixação

**Exercício 1 — Soma de vetor com pool de threads**

Objetivo: ler o número de threads e o tamanho do vetor, inicializar o vetor com valores quaisquer, e calcular a soma dos elementos em paralelo.

```c
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char *argv[]) {
    int num_threads = atoi(argv[1]);
    int n = atoi(argv[2]);

    int *vetor = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) vetor[i] = rand() % 100;

    omp_set_num_threads(num_threads);

    long soma = 0;
    double inicio = omp_get_wtime();

    #pragma omp parallel for reduction(+:soma) schedule(static)
    for (int i = 0; i < n; i++) {
        soma += vetor[i];
    }

    double fim = omp_get_wtime();
    printf("Soma = %ld | Tempo = %f s\n", soma, fim - inicio);

    free(vetor);
    return 0;
}
```

```cpp
#include <iostream>
#include <vector>
#include <cstdlib>
#include <omp.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " <threads> <tamanho>\n";
        return 1;
    }

    int num_threads = std::atoi(argv[1]);
    int n = std::atoi(argv[2]);

    std::vector<int> vetor(n);
    for (int i = 0; i < n; i++) vetor[i] = std::rand() % 100;

    omp_set_num_threads(num_threads);

    long soma = 0;
    double inicio = omp_get_wtime();

    #pragma omp parallel for reduction(+:soma) schedule(static)
    for (int i = 0; i < n; i++) {
        soma += vetor[i];
    }

    double fim = omp_get_wtime();
    std::cout << "Soma = " << soma << " | Tempo = " << fim - inicio << " s\n";

    return 0;
}
```

Compilar: `gcc -fopenmp soma.c -o soma`
Executar: `./soma <num_threads> <tamanho_vetor>`

**Roteiro de avaliação de desempenho**: rodar variando o número de threads (1, 2, 4, 8...) para um mesmo tamanho de vetor, e variando o tamanho do vetor para um mesmo número de threads. Observar: (a) *speedup* = tempo sequencial / tempo paralelo; (b) a partir de que ponto adicionar mais threads deixa de compensar (overhead de criação/sincronização passa a dominar, especialmente em vetores pequenos).

**Exercício 2 — Fibonacci recursivo: task/taskwait vs. sections**

*Versão 1 — task/taskwait* (mostrada na seção 5.6, funciona bem porque a recursão pode ter profundidade arbitrária — cada chamada cria novas tasks dinamicamente).

*Versão 2 — sections/section*:

```c
int fib_sections(int n) {
    if (n < 2) return n;
    int x, y;
    #pragma omp parallel sections
    {
        #pragma omp section
        { x = fib_sections(n - 1); }
        #pragma omp section
        { y = fib_sections(n - 2); }
    }
    return x + y;
}
```
Atenção: usar `sections` de forma recursiva é **muito menos eficiente**, porque cada chamada recursiva abre uma **nova região paralela** (com o overhead de criar/destruir um time de threads a cada nível de recursão), enquanto `task` reaproveita o **mesmo** pool de threads já criado uma única vez lá fora.

**Roteiro de comparação**: medir o tempo de execução das duas versões variando (a) o valor de `n` calculado na série de Fibonacci (cresce exponencialmente o número de chamadas) e (b) o número de threads no pool. Resultado esperado: `task/taskwait` deve escalar melhor com o aumento de `n` e do número de threads, enquanto `sections` tende a ter overhead crescente devido à criação repetida de regiões paralelas aninhadas.


### Memória Distribuída, RPC/RMI e Sistemas Distribuídos

## 6. Programação em Ambiente com Memória Distribuída

Liga diretamente com as seções 1.3(b) (NORMA) e 3.3 (multicomputador) do material original: aqui não existe memória compartilhada, então **toda interação passa pela rede**.

### 6.1 Modelo de arquitetura: o multicomputador

- Conjunto de **nós** (cada um com seu processador e sua **memória própria**) interligados por uma **rede de comunicação**.
- A rede é o substrato para a comunicação entre as partes do programa que rodam em nós diferentes.
- **Cluster** (aglomerado de computadores): a arquitetura mais popular para processamento distribuído. Vários nós explorados *como se fossem* um computador único. Na verdade, cada nó é um computador autônomo, com **sua própria instância do SO**. A "visão de computador único" vem de uma **camada de abstração** (biblioteca ou framework).
- Como não há memória compartilhada, o **investimento em rede** (patch panels, switches, cabeamento) retorna diretamente em desempenho.

### 6.2 Características do modelo (o que muda em relação a threads)

| Característica | Significado |
|---|---|
| **Endereçamento** | A comunicação é um processo **ativo**: as partes precisam **se conhecer mutuamente** para endereçar mensagens com dados. |
| **Responsividade** | O nó **receptor** precisa **disparar a execução de um serviço** para completar a comunicação iniciada pelo outro nó. Não é passivo. |
| **Sincronização** | Comunicar em ambiente distribuído promove, **em algum nível**, a sincronização entre as partes. |
| **Gerência distribuída** | Os nós têm **autonomia** nas decisões locais. Pode haver coordenação global, dependendo da ferramenta. |
| **Latência** | Cada comunicação introduz tempos (normalmente **altos**) que são **somados** ao tempo total de execução. |
| **Granularidade** | Convém que o **volume de dados comunicados** e a **quantidade de cálculo entre duas comunicações** sejam **grandes** (para diluir o custo da latência). |

> **Ideia-chave:** em memória distribuída vale a pena fazer **poucas comunicações com muito trabalho entre elas** (granularidade grossa). Muitas mensagens pequenas matam o desempenho por causa da latência.

### 6.3 Os cinco modelos de programação

1. Troca de mensagens
2. Chamada remota de procedimentos (RPC)
3. Objetos distribuídos
4. Publish/Subscribe
5. Espaço de tuplas

Do mais "baixo nível" (troca de mensagens) ao mais "abstrato". Os mais abstratos são construídos **sobre** a troca de mensagens.

---

### 6.4 Modelo 1 — Troca de Mensagens

- Um processo participa de uma comunicação como **origem** (*sender*) ou **destino** (*receiver*).
- É preciso **estabelecer alguma conexão** entre os participantes para que se reconheçam → necessidade de **endereçamento**.
- Pelo menos um sender e um receiver; **comunicações coletivas** também são possíveis.
- Um **protocolo na camada de aplicação** dá semântica às mensagens.
- É a **tecnologia mais básica**, em geral apoiada em mecanismos de camadas mais baixas (como **sockets**).
- Oferece **poucos recursos de programação**, mas dá **flexibilidade** e (potencialmente) **desempenho**.
- Em algum nível, **sempre ocorre**: os modelos mais abstratos são assentados sobre ele.
- A troca de mensagens **não é passiva no receiver**: ele precisa processar a mensagem conforme o protocolo da aplicação.

**Síncrono vs. assíncrono**

| | Síncrono | Assíncrono |
|---|---|---|
| O `send` termina quando... | o **receiver completa a recepção** | o **mecanismo de transmissão assumiu o compromisso** do envio, mesmo que a mensagem **ainda não tenha saído do nó** do sender |
| Consequência | sender fica bloqueado até a recepção | sender segue em frente rapidamente |

Entre os dois extremos existem **meio-termos**, com diferentes níveis de confiabilidade e desempenho.

**Esquema básico:** `send(destino, dado)` no nó origem e `recv(origem, &dado)` no nó destino, com a mensagem atravessando a rede.

**Tecnologias:**
- **Sockets**: API construída sobre TCP, comunicação pela Internet.
- **MPI** (*Message Passing Interface*): permite criar uma **arquitetura virtual** sobre uma rede local (já visto na seção 3.4 do material original).

---

### 6.5 Modelo 2 — RPC (visão geral)

- **Primeira abstração de alto nível** para programar em ambientes distribuídos.
- Oferece uma interface **próxima do que os programadores já conheciam**: **chamada de procedimento**, com passagem de parâmetros e retorno de resultados.
- Voltado ao modelo **cliente-servidor**: um servidor RPC oferece um serviço associado a um **endereço de rede**.
- **Não é totalmente transparente:** o cliente precisa conhecer o servidor e a interface do serviço (endereço, nome, parâmetros, retorno).
- **Não é um padrão de fato**, o que dificulta a interoperabilidade.
- **Compatibilidade de dados:** **XDR**.
- Parâmetros e retorno **por valor** (cópia).
- Pode ser **síncrono ou assíncrono**.
- Abstração feita por **stub** (lado cliente) e **skeleton** (lado servidor), que encapsulam a troca de mensagens.
- **RPCL** (*remote procedure call language*) + **rpcgen**: interface de alto nível para **gerar** stub e skeleton.

> Detalhado na **seção 7**.

---

### 6.6 Modelo 3 — Objetos Distribuídos

- A rede é **escondida** do programador: as comunicações são representadas por **chamadas de métodos de objetos remotos**.
- É uma **variante de RPC**.
- O sender precisa **conhecer o objeto destino**. Pode existir um **diretório** onde ficam os endereços dos objetos provedores de serviço.
- **Grande vantagem:** reflete integralmente o **modelo Orientado a Objetos**, promovendo desenvolvimento de software "em massa".
- **Desvantagem:** o desempenho pode não ser dos melhores.
- Também usa o par **stub/skeleton**.

**Tecnologias:**
- **CORBA** — *Common Object Request Broker Architecture*
- **Java RMI** — *Remote Method Invocation*

---

### 6.7 Modelo 4 — Publish/Subscribe

Alternativa ao modelo cliente/servidor.

- **Publicador** (*publisher*): fornece/produz a informação.
- **Assinante** (*subscriber*): requisita/consome a informação.
- **Evento ou publicação**: a informação fornecida pelo publicador.
- Comunicação **assíncrona e unidirecional**.
- **Escalável**: o publisher se dedica a produzir conteúdo, e o número de assinantes pode ser adequado à aplicação.
- **Fracamente acoplado:** o publicador **não conhece os assinantes**, e o assinante **não sabe quem fornece** o conteúdo.
- A inscrição **não é a um publicador, e sim a uma categoria de conteúdo**.

**Serviço de Notificação (canal Pub/Sub):** intermedeia inscrições e entrega de eventos. Seu papel é:
1. receber pedidos de assinatura;
2. manter as informações sobre as assinaturas efetivadas;
3. receber as notificações dos publicadores;
4. encaminhar aos assinantes as informações que aguardam.

```
Publisher ──► [ Serviço de Notificação ] ──► Assinante
                     (na rede)           ──► Assinante
                                         ──► Assinante
```

**Pontos de atenção:**
- **Tolerância a falhas** e **garantia de entrega** devem ser suportadas pelo **middleware**.
- O assinante precisa de um **mecanismo ativo** para receber as postagens.

**Implementações possíveis do middleware:**

| Implementação | Como funciona |
|---|---|
| **Multicast** | Todos os assinantes de um assunto formam um **grupo**. Voltado a **rede local**. |
| **Broker** | Usa protocolo de mais baixo nível (TCP, HTTP) para encaminhar mensagens. |
| **P2P** | Permite **inundar** os assinantes com a mensagem do evento. |

**Tecnologias:** **WebSub** (protocolo sobre Internet), **RSS** (web feed / agregador de notícias), **JMS** (*Java Message Service*, hoje Jakarta Message API).

---

### 6.8 Modelo 5 — Espaço de Tuplas

- Modelo de **memória distribuída** (uma "memória compartilhada virtual" construída sobre a rede).
- **Tupla = `<chave, dado>`**
  - **chave**: identificação (endereço) da informação;
  - **dado**: a informação armazenada.
- A memória é um grande "quadro" onde as tuplas são compartilhadas entre os processos.
- Comunicação **essencialmente assíncrona** (fraco acoplamento entre as partes).

**Operações:**

| Operação | O que faz |
|---|---|
| `RD(chave, ?)` | **Lê** o dado associado à chave (a tupla **permanece** no espaço). |
| `IN(chave, ?)` | Lê o dado associado à chave **e retira** a tupla do espaço. |
| `WR(chave, dado)` | **Escreve** uma nova tupla. |
| `EVAL(chaveA, func(<chaveB, ?>))` | Escreve uma nova tupla cujo dado é o **retorno da função `func`** executada no espaço de tuplas. |

Exemplo (da figura do slide): `wr(1,1)`, `wr(10,55)`, `rd(13,? = 233)`, `in(11,? = 89)`. Os dois últimos usam `?` como marcador do dado a ser recuperado. A diferença: o `rd` deixa a tupla `<13,233>` lá, e o `in` a remove.

**Tecnologias:**
- **Linda**: linguagem de **coordenação** que implementa o modelo.
- **JavaSpace**: API Java com **operações atômicas** sobre o espaço de tuplas.

> **Pegadinha possível:** `RD` vs `IN`. Ambos retornam o dado; só o `IN` **retira** a tupla.

---

## 7. RPC e RMI

### 7.1 Princípio

Oferecer a **semântica de uso de procedimentos/métodos** (encapsulamento de código) em ambiente de memória distribuída. Funciona com **relativo sucesso**, mas **a abstração completa do ambiente distribuído não é obtida**.

- **RPC** — *Remote Procedure Call*
- **RMI** — *Remote Method Invocation*

### 7.2 Por que a abstração é incompleta

| Problema | Explicação |
|---|---|
| **Endereçamento** | O cliente precisa conhecer o **endereço do serviço**: nome do serviço, hospedeiro (host) e até a **porta** para estabelecer a conexão. |
| **Parâmetros por cópia** | Não existe espaço de endereçamento compartilhado. Parâmetros de entrada e resultados são passados **por cópia**, **nunca por referência**. |
| **Ambiente de rede** | Alta latência, falhas e outras situações típicas de rede **devem ser toleradas pela aplicação**. |

> **Por que não passa por referência?** Um ponteiro/referência só faz sentido dentro de um espaço de endereçamento. O servidor está em outra máquina, com outra memória, então o endereço não significa nada lá.

### 7.3 Componentes (comuns a RPC e RMI)

| Componente | Papel | Origem |
|---|---|---|
| **Interface** | Especifica o serviço: nome do procedimento (ou classe/método), parâmetros de entrada e retorno | **Programado** |
| **Serviço** | **Implementação** do serviço | **Programado** |
| **Cliente** | **Requisita** a execução do serviço | **Programado** |
| **Servidor** | Processo que **recebe invocações** e provê o atendimento | **Programado** |
| **Stub** | **Proxy no cliente**: cumpre as funções de chamada/invocação | **Gerado** |
| **Skeleton** | **No servidor**: recebe a requisição e **requisita a execução do serviço** | **Gerado** |

> ⚠️ **Erro nos slides:** no slide 11 (componentes RPC) o skeleton aparece descrito como "módulo ligado ao **cliente**". O correto, e o que os demais slides dizem, é que o **skeleton fica no servidor** e o **stub fica no cliente**.

**Regra de ouro:** o programador escreve **interface, serviço, cliente e servidor**. As ferramentas **geram stub e skeleton**.

### 7.4 Fluxo de uma chamada (11 passos)

```
        CLIENTE                                         SERVIDOR
 ┌──────────┐   ┌──────┐                      ┌──────────┐   ┌─────────┐
 │Aplicação │──►│ Stub │ ═══ mensagem/rede ═► │ Skeleton │──►│ Serviço │
 └──────────┘   └──────┘ ◄═══ resposta ═════  └──────────┘   └─────────┘
```

| # | O que acontece |
|---|---|
| 1 | Aplicação **invoca o serviço local** (chama o stub, como se fosse local) |
| 2 | Stub **monta a mensagem** (empacotamento / *marshalling*) |
| 3 | Mensagem é **enviada** pela rede |
| 4 | Skeleton **desempacota** a mensagem |
| 5 | Skeleton **chama o procedimento** |
| 6 | O **serviço executa** |
| 7 | Serviço **retorna o resultado** |
| 8 | Skeleton **empacota** o retorno |
| 9 | Mensagem **enviada** de volta |
| 10 | Stub **desempacota** o retorno |
| 11 | Stub **retorna à aplicação** |

Do ponto de vista do cliente, tudo parece uma chamada local comum. O stub esconde os passos 2 a 10.

### 7.5 RPC vs. RMI

Têm características tão próximas que podem ser apresentados juntos, **mas** suas diferenças marcam **vocações de uso diferentes** (os slides comparam a um buggy vs. um carro esportivo: ambos andam, mas servem a propósitos diferentes).

| Aspecto | **RPC** | **RMI** |
|---|---|---|
| Paradigma | **Procedural** (procedimentos) | **Orientado a objetos** (métodos de objetos remotos) |
| Linguagem | Tipicamente **C** | **Java** |
| Interface | Arquivo de **definição de interface** (`.x`, linguagem RPCL) | **Interface Java** (tipo abstrato do serviço) |
| Geração de stub/skeleton | Ferramenta **`rpcgen`** gera arquivos `.c`/`.h` | Classes geradas **implicitamente pelo compilador Java** |
| Representação de dados | **XDR** (empacotamento/desempacotamento) | Serialização Java, via **JVM** |
| Registro/localização do serviço | O servidor **registra o serviço** | **`rmiregistry`**: serviço de registro de objetos remotos |
| Portabilidade | Depende da plataforma/compilação em C | Alta, por rodar na **JVM** (segurança também pela máquina virtual) |

### 7.6 Tecnologia RPC

- **XDR**: abstração para **compartilhamento de dados**, permitindo **empacotamento e desempacotamento** dos dados envolvidos na execução do serviço.
- **rpcgen**: gerador, a partir da **interface**, dos **stubs e skeletons** da aplicação.

**Arquivos envolvidos (exemplo `addsub`):**

| Arquivo | Quem escreve | Conteúdo |
|---|---|---|
| `addsub.x` | programador | **Interface** (RPCL) |
| `cliente.c` | programador | **Cliente** |
| `servico.c` | programador | **Implementação do serviço** |
| `addsub.h`, `addsub_xdr.c`, `addsub_clnt.c`, `addsub_svc.c` | **`rpcgen`** | cabeçalho, rotinas XDR, **stub do cliente** (`_clnt`), **skeleton/servidor** (`_svc`) |

**Fluxo de compilação e execução:**

```bash
$> rpcgen addsub.x
$> gcc cliente.c addsub_xdr.c addsub_clnt.c -o cliente
$> gcc servico.c addsub_xdr.c addsub_svc.c -o servidor
$> ./servidor          # No terminal 1
$> ./cliente 5 4       # No terminal 2
```

Repare no que entra em cada executável:
- **cliente** = `cliente.c` + `_xdr.c` + `_clnt.c` (stub)
- **servidor** = `servico.c` + `_xdr.c` + `_svc.c` (skeleton)

> **Exemplo ilustrativo** (não é o arquivo da página da disciplina; serve para fixar o formato de um `.x`):
> ```c
> struct operandos { int x; int y; };
>
> program ADDSUB_PROG {
>     version ADDSUB_VERS {
>         int ADD(operandos) = 1;
>         int SUB(operandos) = 2;
>     } = 1;
> } = 0x23451111;
> ```
> Passar os dois inteiros dentro de uma `struct` é o jeito clássico de contornar a limitação de **um único parâmetro** nas versões tradicionais do `rpcgen`.

**Atividade de acompanhamento (slide 20):** modificar a implementação RPC para que ela **retorne o Fibonacci de um número**. O que muda: a **interface** (`.x`: nova função, ex. `long FIBO(int)`), a **implementação** no `servico.c` e a **chamada** no `cliente.c`. Stubs e skeletons são **regenerados** pelo `rpcgen`.

### 7.7 Tecnologia RMI

- **JVM**: alta portabilidade, além de segurança pela execução em máquina virtual.
- **`java.rmi.*`**: pacote com os serviços de suporte necessários.
- **`rmiregistry`**: serviço de registro de objetos remotos.

**Arquivos envolvidos (exemplo Fibonacci):**

| Arquivo | Papel |
|---|---|
| `Fibonacci.java` | **Interface** |
| `FiboImplementacao.java` | **Implementação do serviço** |
| `FiboServidor.java` | **Servidor**: registra o serviço, **cria uma instância** do serviço, recebe e trata as requisições |
| `FiboCliente.java` | **Cliente**: (i) conexão com o servidor e (ii) chamada aos serviços |

**Execução:**

```bash
$> javac *.java
$> rmiregistry            # Terminal 1 — no MESMO diretório das classes!
$> java FiboServidor      # Terminal 2
$> java FiboCliente 13    # Terminal 3
```

> **Atenção:** o `rmiregistry` precisa ser iniciado **no mesmo diretório** onde estão as classes compiladas (o slide destaca isso). Ordem: **registry → servidor → cliente**.

> **Exemplo ilustrativo** do formato (não é o código da página da disciplina):
> ```java
> // Interface: estende Remote; todo método lança RemoteException
> public interface Fibonacci extends java.rmi.Remote {
>     long fibo(int n) throws java.rmi.RemoteException;
> }
> ```
> A implementação costuma estender `UnicastRemoteObject` e implementar a interface. O servidor instancia a implementação e a **registra** (ex.: `Naming.rebind(...)`). O cliente faz o **lookup** e chama o método como se fosse local. Stub e skeleton são gerados pelo compilador/runtime Java.

**Pelo que RMI "paga" em troca da portabilidade:** só funciona entre **componentes Java**. Para integrar linguagens diferentes, o slide aponta o **CORBA** como alternativa.

### 7.8 Resumo de diferenças em uma frase

- **RPC** → "chamo um **procedimento** em outra máquina". Interface em **RPCL**, geração com **rpcgen**, dados em **XDR**.
- **RMI** → "chamo um **método de um objeto** em outra máquina". Interface é uma **interface Java**, registro no **rmiregistry**, tudo sobre a **JVM**.

---

## 8. Sistemas Distribuídos: Conceitos e Arquitetura

### 8.1 Definição

> Um **Sistema Distribuído (SD)** é uma **coleção de elementos de computação autônomos (nós)** que se mostra, aos seus usuários, como **um sistema único e coerente**.

- **Nós** podem ser **dispositivos de hardware** *ou* **processos de software**.
- O "sistema único e coerente" (para pessoas **ou** para aplicações) **exige colaboração entre os nós**.

Frase de Leslie Lamport (slide 5), que resume o "lado ruim": *um sistema distribuído é aquele em que a falha de um computador que você nem sabia que existia pode tornar o seu próprio computador inutilizável.*

Retoma a definição de SD da seção 3.1 do material original (autônomos, sem memória física compartilhada, comunicação por mensagens).

### 8.2 Os cinco termos-chave

#### a) Coleção de nós autônomos
- **Poder de decisão distribuído**.
- **Não existe relógio global**.
- Questões de **sincronização e coordenação** devem ser tratadas **pela aplicação**.
- **Gerência dos membros** (quem está no sistema).
- **Segurança**: identificar com segurança a identidade dos interlocutores.

#### b) Rede sobreposta (*overlay network*)
- Na rede que representa o SD, os **nós são processos** e os **links são os possíveis canais de comunicação** entre eles, usando protocolos de camadas inferiores.
- **Estruturada**: a rede tem estrutura definida e os nós conhecem seus vizinhos (**anel, árvore...**).
- **Não estruturada**: não há padrão nas conexões.
- A rede sobreposta é uma camada **lógica por cima da rede física**.

#### c) Sistema coerente
A coleção de nós opera **da mesma forma**, não importa **onde, quando e como** ocorra a interação com o usuário:
- o usuário **não sabe onde** a computação está sendo feita;
- a **localização exata dos dados** pode ser irrelevante;
- **replicação** de dados (existir ou não) **não é visível** para a aplicação.

#### d) Transparência
Requisito desejado, mas **nem sempre é possível esconder falhas**, e a recuperação é difícil. **Na prática, não é possível esconder completamente do usuário final a ocorrência de falhas.**

#### e) Middleware
> **"O Sistema Operacional dos sistemas distribuídos."**

Contém componentes e funcionalidades **necessárias a todo sistema**, evitando implementações individuais. Provê:
1. **Compartilhamento de recursos**
2. **Transparência da distribuição**
3. **Abertura**
4. **Escalabilidade**

### 8.3 Tipos de transparência (tabela clássica, muito cobrada)

| Transparência | Esconde... |
|---|---|
| **Acesso** | diferenças na **representação dos dados** e na forma como os acessos são realizados |
| **Localização** | a **localização física** do objeto |
| **Relocação** | a possibilidade de o objeto **ser movido enquanto está em uso** |
| **Migração** | a possibilidade de o objeto **alterar sua localização** |
| **Replicação** | que o objeto **pode ser replicado** |
| **Concorrência** | o **uso compartilhado** de um mesmo objeto |
| **Falha** | **falhas**, e **recupera** o objeto caso uma falha ocorra |

**Diferença sutil:** *migração* é o objeto **mudar de lugar** (fora de uso); *relocação* é mudar **enquanto alguém está usando**.

**Limites da transparência (o slide insiste: "transparência é legal, mas obtê-la é muito difícil; às vezes a meta nem deve ser atingi-la"):**
- É **impossível esconder as latências** das comunicações.
- **Transparência total a falhas é impossível:** o objeto está lento ou falhou? O servidor executou a operação **antes, durante ou depois** de falhar?
- **Mais transparência = maior custo.** Réplicas têm custo de processamento e de atualização do master.

**Expor a distribuição pode ser bom:**
- a informação de localização ajuda a achar **serviços próximos**;
- permite explorar recursos em função do **fuso horário**;
- permite ao usuário **interpretar atrasos** e identificar situações de falha.

### 8.4 Abertura

Capacidade de **interagir com outros sistemas**:
- conformidade com **interfaces bem definidas**;
- **interoperabilidade**;
- suporte à **portabilidade** de aplicações;
- **extensibilidade**.

### 8.5 Escalabilidade: três eixos

| Eixo | Pergunta |
|---|---|
| **Tamanho** | Quantidade de **recursos** (usuários, nós, dados) |
| **Geográfica** | **Área de abrangência** |
| **Administrativa** | **Gestão dos recursos** (domínios distintos) |

**Tamanho**
- Comparado a um sistema **centralizado**, o ganho vem da restrição natural dos recursos da central e da **distância de rede** entre usuário e central.
- Contraponto: **mais recursos, mais próximos dos usuários**.

**Geográfica**
- Os problemas em **WAN** não são os de **LAN**: **maior latência**, solução de natureza diferente.
- Exemplos: **streaming de vídeo**; **broadcast vs. diretório de serviços**.
- **Técnicas para escalar:**
  1. **Mascarar latências** com comunicação **assíncrona**;
  2. **Mover parte da computação para perto do usuário**;
  3. **Réplicas**;
  4. **Caches e mirrors** (exemplo do slide: a rede de caches do **Netflix Open Connect**).
- Sempre **pesar vantagens e desvantagens** (ex.: replicar/cachear gera o problema de manter **consistência**).

**Administrativa**
- Garantir que, mesmo surgindo **vários domínios administrativos**, o sistema continue **administrável**: políticas de uso, pagamento, gerenciamento e **segurança**.
- Exemplo clássico: **Grid Computing**.

### 8.6 As falácias da computação distribuída

O slide 28 lista o que **não se pode considerar** ao implementar um SD:

| ❌ Falsa suposição |
|---|
| A rede é **confiável** |
| A rede é **segura** |
| A rede é **homogênea** |
| A **topologia** da rede **não varia** |
| A **latência é nula** |
| A **largura de banda é infinita** |
| O **custo do transporte é zero** |
| Há **apenas um administrador** |

> Dica de estudo: cada item vira uma questão de projeto (tratar falhas, criptografar, lidar com formatos diferentes, tolerar mudanças de topologia, minimizar comunicação, comprimir/agrupar dados, etc.).

### 8.7 Três tipos de Sistemas Distribuídos

| Tipo | Foco |
|---|---|
| **HPC** | **High Performance Computing** |
| **Integração de aplicações** | Atender demandas de **interoperabilidade** entre várias aplicações |
| **Pervasivo** | Nós **pequenos** (poucos recursos), que **se deslocam** e podem estar **inseridos no ambiente** |

#### HPC: Cluster, Grid, Cloud

| | Cluster | Grid | Cloud |
|---|---|---|---|
| Rede | **LAN** | **WAN** | **Internet** |
| Homogeneidade | **SO homogêneo**, hardware **quase homogêneo** | **Grande heterogeneidade** (sw e hw) | Recursos acessados **via rede** |
| Abrangência | Local | **Várias organizações**, ampla área geográfica | Global |

**Diferença Grid vs. Cloud:**
- **Grid:** os recursos estão todos disponíveis e são **acessados diretamente** (não só hardware, mas também **dados**). Frequentemente usa **virtualização**.
- **Cloud:** os recursos são acessados **via rede, não diretamente**. A ideia é **outsourcing**.

(Reforça a seção 1.3(c) do material original, onde Cluster, Beowulf, MPP, Grid e Cloud foram definidos.)

#### Integração de aplicações
- **Fato seminal:** aplicações rodam em **servidores** que atendem requisições de clientes. A integração básica **combina respostas de consultas a diferentes servidores**, produzindo uma resposta coerente.
- **Questão tratada:** permitir **comunicação direta entre as aplicações**.
- **Exemplo:** agência de viagens reservando **voo e hotel de forma casada** (cada um em seu BD). Solução: **transações aninhadas** (ou as duas reservas acontecem, ou nenhuma).

#### Pervasivo
Dispositivos pequenos, com alta capacidade de processamento (para o tamanho) e **baixo consumo**; caracterizam-se por **mobilidade** e por **se misturar ao meio**. Subtipos:

- **Computação ubíqua** — cinco propriedades:
  1. **Distribuição**: dispositivos em rede, acessíveis de forma transparente;
  2. **Interação** altamente **discreta**;
  3. **Consciência de contexto**: o sistema identifica o contexto do usuário para decidir;
  4. **Autonomia**: pouca ou nenhuma intervenção humana, **auto-gerenciáveis**;
  5. **Inteligência**: o sistema executa um amplo espectro de ações e interações.
- **Computação móvel** — smartphones, tablets, vestíveis. O dispositivo **se desloca**, então é preciso **suporte à descoberta**; a comunicação é **instável** (perda de conexão), e a aplicação deve **tolerar** isso.
- **Rede de sensores** — dezenas a milhares de sensores interconectados, **poucos recursos**, tarefas específicas, fornecem **dados para decisão**. O **consumo energético é crítico**.
- **Computação na névoa** (*fog computing*) — citada no slide como subtipo; não foi detalhada.

### 8.8 Estilos arquiteturais

O **estilo arquitetural** de um SD é definido em termos de **componentes** e **conectores**:

- **Componente**: unidade **modular**, com **interface bem definida**, que **pode ser substituída**.
- **Conector**: **mecanismo de comunicação, coordenação e/ou cooperação** entre componentes (RPC, troca de mensagens, ...). Viabiliza a troca de dados de qualquer natureza.

A configuração de componentes e conectores deve passar a **visão de um sistema único**, e pode refletir diferentes estilos:

1. **Em camadas**
2. **Baseado em objetos**
3. **Baseado em recursos**
4. **Publish/Subscribe**

#### 8.8.1 Estilo em camadas

Cada camada disponibiliza uma **interface de acesso** aos serviços que oferece. Três variações de interação (figura do slide 8):

| Variação | Descrição |
|---|---|
| **(a) Modelo clássico** | Chamada à camada inferior **resulta em uma resposta** (**síncrono**). Camada N → N-1 → ... → 1. |
| **(b)** | Uma camada pode acessar **duas ou mais camadas inferiores**. Ocorre quando uma camada oferece **recursos auxiliares** a certas classes de aplicações, mas ainda usa serviços das camadas abaixo. |
| **(c) Assíncrono (upcall)** | A demanda é feita às camadas inferiores e a **camada superior é informada quando o serviço foi concluído** (*upcall*). |

**Exemplo 1: pilha de protocolo de comunicação.** Cada camada tem **interface** (acesso aos serviços) e um **protocolo** (interação entre componentes distintos, Party A ↔ Party B). A **demanda da camada N é tratada como o dado a ser transmitido pela camada N-1**.

**Exemplo 2: sistema de busca na web (3 camadas de aplicação):**

| Camada | Papel |
|---|---|
| **Interface com o usuário** (*user-interface level*) | Dá acesso aos serviços (a usuários ou outros sistemas) |
| **Processamento** (*processing level*) | **Operacionaliza as funcionalidades** (ex.: gerar consulta, algoritmo de ranking, gerar HTML) |
| **Dados** (*data level*) | Contém os **dados** manipulados pelos demais (BD de páginas web) |

#### 8.8.2 Estilo baseado em objetos

- Abstração em termos de **objetos que interagem**.
- Mesma essência da programação OO: o objeto **encapsula dados (estado) e serviços (métodos)**, e a chamada de serviço é uma **invocação (remota) de método**.
- Apoiado pelo par **proxy/skeleton**, que abstrai a manipulação das invocações e seus retornos: o cliente invoca o método no **proxy** (mesma interface do objeto), a invocação é **marshalled** e enviada pela rede, e o **skeleton** invoca o mesmo método no objeto real.
- Muito usado em aplicações com **Java/RMI** ou **CORBA**.

(É exatamente o modelo da seção 7: proxy = stub.)

#### 8.8.3 Estilo baseado em recursos (arquiteturas RESTful)

- O SD é uma **coleção de recursos**, **gerenciados individualmente** pelos componentes. Componentes podem **adicionar, retirar, encontrar e modificar dinamicamente** os recursos.
- Exemplo do slide: **Amazon Storage Service (S3)**.
- Comunicação via **HTTP Request/Response**, com payloads em XML, JSON, HTML, texto etc.

**Princípios:**
1. Recursos são **identificados por um nome**.
2. **Todos os serviços oferecem a mesma interface.**
3. As mensagens são **inteiramente autodescritivas** (descrevem **dados e ações** a executar).
4. O componente **não armazena informações sobre quem fez a solicitação** (**stateless**: sem estado de sessão no servidor).

**Operações básicas (como listadas no slide):**

| Operação | Efeito |
|---|---|
| **PUT** | **Cria** um novo recurso |
| **GET** | **Obtém informações** sobre um recurso |
| **DELETE** | **Remove** um recurso |
| **POST** | **Atribui um novo estado** a um recurso |

> **Atenção ao enunciado da prova:** a tabela acima segue o slide. Na especificação HTTP geral, o uso mais comum é **POST para criar** e **PUT para criar/substituir** (idempotente). Se a questão pedir "segundo o material da disciplina", use a tabela do slide.

#### 8.8.4 Estilo Publish/Subscribe

- Valorizam o **caráter dinâmico** dos componentes: **entrar e sair** do sistema.
- A implementação pode ser: **envio direto** da publicação a cada assinante, **ou** um componente (ou vários) que **armazena** as postagens, segundo alguma política, e as **reencaminha** aos assinantes.
- Figuras do slide: **barramento de eventos** (*event bus*: subscribe → publish → notification delivery) e **espaço de dados compartilhado persistente** (publish → armazenado → subscribe → data delivery).
- **Exemplo: Linda tuple space.** Linguagem de **coordenação** em que **quatro operações básicas** (`RD`, `WR`, `IN`, `EVAL`) estendem o potencial de uma segunda linguagem ao acesso a um **espaço de dados compartilhado**.

Operações segundo o slide 17:
- `RD(key, dta)`: lê uma tupla correspondente à key;
- `WR(key, dta)`: escreve uma nova tupla;
- `IN(key, dta)`: **retira** do espaço a tupla correspondente à key;
- `EVAL(F, key_out, F, key_in)`: executa o serviço F **no espaço de tuplas**, usando a tupla `key_in` como entrada e armazenando o resultado em `key_out`.

(Mesmo conteúdo da seção 6.8, aqui visto como **estilo arquitetural**.)

### 8.9 O problema do middleware: wrappers, brokers e interceptors

**Ideia:** o middleware promove a interação entre componentes, então as interfaces **deveriam ser compatíveis** entre todos. **Na prática, não são.** Entram em ação:

| Mecanismo | O que é |
|---|---|
| **Wrapper** | Componente que **oferece uma interface aceita pelo cliente**; **adapta a comunicação entre dois componentes**. |
| **Broker** | Permite a **adaptação das comunicações entre vários componentes** (um ponto central de adaptação, evitando um wrapper para cada par). |
| **Interceptor** | Peça de software que **se interpõe entre dois componentes** e executa operações **sem que o cliente saiba**. |

**Exemplo do interceptor (slide 20):** o cliente faz uma requisição (`B.doit(val)`) a um componente gerido pelo middleware. **Sem o cliente saber**, por interceptação (em nível de requisição ou de mensagem), **réplicas** do componente de serviço são invocadas.

**Por que broker > wrappers?** Com N componentes, adaptar todos entre si via wrappers exigiria algo da ordem de N² adaptações. Com um broker, cada componente se adapta **uma vez** ao broker.

### 8.10 Esquemas arquiteturais (organização do sistema)

```
Esquemas arquiteturais
├── Centralizada
│   ├── Cliente/Servidor
│   └── Multicamada
└── Descentralizada
    └── Redes P2P
        ├── Estruturadas
        └── Não estruturadas
```

#### Centralizada

**Ideia básica:** alguns componentes **oferecem serviços**, outros **consomem**.

**Cliente/Servidor:** o cliente faz uma requisição (*request*) e **espera** (*wait*); o servidor **provê o serviço** e devolve a resposta (*reply*).

**Multicamada:** é possível distribuir as camadas **entre cliente e servidor** em diferentes pontos de corte (figura do slide 23, variantes a–e):

| Variante | Fica no cliente | Fica no servidor |
|---|---|---|
| (a) | (parte da) interface com usuário | restante da interface + aplicação + BD |
| (b) | interface com usuário | aplicação + BD |
| (c) | interface + (parte da) aplicação | restante da aplicação + BD |
| (d) | interface + aplicação | BD |
| (e) | interface + aplicação + (parte do) BD | restante do BD |

**Três camadas:** o cliente pede a operação ao **servidor de aplicação**, que **atua como cliente** do **servidor de dados**. Ou seja, **o mesmo componente pode ser servidor para um e cliente para outro**.

#### Descentralizada: Redes P2P

**Todos os componentes têm o mesmo papel** (cliente **e** servidor ao mesmo tempo). Existe a necessidade de **endereçamento** do serviço/dado (como achar quem tem o que preciso?).

**P2P Estruturada, exemplo: Chord**
- Nós têm um **identificador de m bits** e são organizados em um **anel**.
- Existem **atalhos** (*shortcuts*) no anel, que aceleram a busca.
- Cada item de dado tem um **hash** que o mapeia para uma **chave de m bits**.
- Um item com chave **k** é armazenado no nó de **menor identificador com `id ≥ k`**, chamado **sucessor da chave k**.
- Exemplo da figura: o nó 9 é responsável pelas chaves {5,6,7,8,9}; a busca `lookup(3)@9` segue **28 → 1 → 4** (4 é o sucessor de 3).

**P2P Não estruturada**
- **Não existe estrutura conhecida.** Cada nó mantém uma **lista de vizinhos** construída ao longo da computação (**ad-hoc**).
- O resultado é um **grafo** em que as arestas (u,v) indicam a **probabilidade** de haver conexão entre u e v.
- **Busca** feita por:
  - **Flooding** (inundação): o nó repassa a consulta a **todos** os vizinhos;
  - **Random walker** (caminhante aleatório): a consulta segue por **um** vizinho escolhido aleatoriamente.

| | Estruturada (Chord) | Não estruturada |
|---|---|---|
| Topologia | **Definida** (anel/árvore) | Ad-hoc, sem padrão |
| Onde fica o dado | **Determinado pela chave** (hash → sucessor) | Qualquer nó |
| Busca | **Dirigida** (lookup) | **Flooding** ou **random walker** |

---

## 9. Quadros-resumo e questões de revisão

### 9.1 Visão geral dos cinco modelos de memória distribuída

| Modelo | Acoplamento | Comunicação | Unidade de interação | Exemplos |
|---|---|---|---|---|
| **Troca de mensagens** | forte (precisa endereçar) | send/recv | **mensagem** | Sockets, MPI |
| **RPC** | forte (cliente conhece servidor) | síncrona ou assíncrona | **procedimento** | rpcgen, XDR |
| **Objetos distribuídos** | forte (conhece objeto) | invocação de método | **método de objeto** | Java RMI, CORBA |
| **Publish/Subscribe** | **fraco** | assíncrona, unidirecional | **evento/publicação** | JMS, RSS, WebSub |
| **Espaço de tuplas** | **fraco** | essencialmente assíncrona | **tupla `<chave,dado>`** | Linda, JavaSpace |

### 9.2 Memória compartilhada vs. distribuída (síntese com o material original)

| | Compartilhada (threads/OpenMP) | Distribuída (MPI/RPC/RMI/...) |
|---|---|---|
| Modelo de hardware | Multiprocessador (UMA/NUMA) | Multicomputador (NORMA) |
| Comunicação | Ler/escrever em variáveis compartilhadas | **Mensagens pela rede** |
| Sincronização | Mutex, semáforos, monitores... | **Implícita na comunicação** (+ coordenação pela aplicação) |
| Custo de comunicar | Baixo | **Alto (latência de rede)** |
| Granularidade ideal | Pode ser mais fina | **Grossa** |
| Falhas | O sistema todo cai junto | **Falhas parciais**, devem ser toleradas |

### 9.3 Questões para treinar

**Memória distribuída e modelos**
1. Por que, em memória distribuída, é desejável que a quantidade de cálculo entre duas comunicações seja grande?
2. Qual a diferença entre uma troca de mensagens síncrona e uma assíncrona? Em qual delas o `send` pode terminar antes de a mensagem sair do nó de origem?
3. Em Publish/Subscribe, por que o modelo é dito "fracamente acoplado"? Quais as quatro responsabilidades do serviço de notificação?
4. Cite as três formas de implementar o middleware de Pub/Sub (multicast, broker, P2P) e uma característica de cada.
5. No espaço de tuplas, qual a diferença entre `RD` e `IN`? O que faz o `EVAL`?

**RPC e RMI**
6. Cite três motivos pelos quais RPC/RMI **não** oferecem abstração completa do ambiente distribuído.
7. Por que os parâmetros em RPC/RMI são passados por cópia, e não por referência?
8. Qual a função do **stub** e do **skeleton**? Em qual lado cada um fica? Quem os escreve?
9. Descreva os passos de uma chamada remota, do cliente ao retorno.
10. O que são **XDR**, **RPCL** e **rpcgen**? Como são gerados os executáveis cliente e servidor no exemplo `addsub`?
11. Qual o papel do **rmiregistry**? O que acontece se você esquecer de iniciá-lo (ou iniciá-lo em outro diretório)?
12. Compare RPC e RMI em pelo menos quatro aspectos.

**Sistemas distribuídos**
13. Defina SD. O que significa "sistema único e coerente"?
14. O que é **rede sobreposta**? Diferencie estruturada de não estruturada.
15. Liste e explique os sete tipos de **transparência**. Diferencie **migração** de **relocação**.
16. Por que a transparência total é impossível/indesejável? Dê dois motivos.
17. Quais os **três eixos de escalabilidade**? Cite quatro técnicas para escalar geograficamente.
18. Liste cinco das oito **falácias** da computação distribuída.
19. Diferencie **Cluster, Grid e Cloud** (rede, homogeneidade, forma de acesso aos recursos).
20. Cite as cinco propriedades da **computação ubíqua**.
21. O que são **componentes** e **conectores**? Dê exemplos de conectores.
22. Descreva as três variações de interação do **estilo em camadas** (a, b, c). O que é um *upcall*?
23. Quais os quatro princípios de uma arquitetura **RESTful** e as quatro operações básicas (segundo o slide)?
24. Diferencie **wrapper**, **broker** e **interceptor**.
25. Em Chord, onde é armazenada a chave `k`? Se os nós existentes são {1, 4, 9, 11, 14, 18, 20, 21, 28}, em qual nó fica a chave 3? E a chave 12?
26. Como funcionam **flooding** e **random walker** em uma P2P não estruturada?

### 9.4 Respostas rápidas de alguns itens

- **(2)** Síncrono: o `send` só termina quando o receiver completa a recepção. Assíncrono: o `send` termina quando o mecanismo de transmissão assume o compromisso do envio, **mesmo que a mensagem ainda não tenha saído do nó**.
- **(5)** `RD` lê e **mantém** a tupla; `IN` lê e **retira**. `EVAL` escreve uma tupla cujo dado é o **resultado de uma função executada no próprio espaço de tuplas**.
- **(7)** Não há espaço de endereçamento compartilhado: uma referência/ponteiro do cliente não tem significado na memória do servidor.
- **(8)** **Stub** = proxy no **cliente**; **skeleton** = no **servidor**. Ambos são **gerados** (por `rpcgen` no RPC; pelo compilador/runtime Java no RMI), não escritos pelo programador.
- **(25)** Sucessor = menor id ≥ k. Chave **3** → nó **4**. Chave **12** → nó **14**. (Confira com a figura do slide 26: nós 1, 4, 9, 11, 14, 18, 20, 21, 28 e m = 5 bits, ou seja, IDs de 0 a 31.)

---

## Observação final sobre a cobertura

| Pendência original | Situação |
|---|---|
| Sistemas Distribuídos — RPC | ✅ Coberto (seção 7) |
| Sistemas Distribuídos — RMI | ✅ Coberto (seção 7) |
| Sistemas Distribuídos — conceitos, middleware, estilos arquiteturais | ✅ Coberto (seção 8) |
| Modelos em memória distribuída | ✅ Coberto (seção 6) |
| **Manycore** | ❌ **Não estava nos slides enviados** |

