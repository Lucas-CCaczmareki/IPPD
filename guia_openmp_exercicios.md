ESSENCIAIS: 3, 4, 6, 7, 10, 12, 15, 16

# Guia Prático de OpenMP em C++ — aprendendo por exercícios

> Material de estudo baseado nas listas do professor (IPPD): *Laboratório de OpenMP*, *Exercícios – OpenMP*, os exercícios de 31/out (dependências e escalonamento) e o exercício de soma de vetor com `parallel` + `private` + `reduction`.
> Tudo está em **C++17** (`g++`).

---

## Como usar este guia

Cada exercício segue o mesmo roteiro:

1. **Enunciado** — o que pedem.
2. **O que você precisa saber** — a teoria mínima para resolver *sozinho*.
3. **Tente sozinho** — pare aqui e tente implementar antes de olhar a solução.
4. **Solução comentada** — código com comentários sobre o *quê* e o *porquê* de cada decisão.
5. **Para fixar** — respostas às perguntas do enunciado.

**Dica de estudo:** digite o código (não copie e cole), compile, rode com `OMP_NUM_THREADS=1,2,4,8` e mude coisas para ver o que quebra. Em OpenMP, errar de propósito ensina muito.

### Índice

| Nível | # | Exercício | Conceito central |
|---|---|---|---|
| **Fundamentos** | 1 | Região paralela e distribuição de iterações | `parallel`, `for`, `schedule` |
| | 2 | Operações sobre vetores (SAXPY) | `parallel for`, speedup |
| | 3 | `private` e `firstprivate` | escopo de dados |
| **Sincronização** | 4 | Condição de corrida e `reduction` | race condition, `reduction` |
| | 5 | Soma de vetor "à mão" | `parallel` sem `for`, divisão manual |
| | 6 | `lastprivate` | valor da última iteração |
| | 7 | `atomic` × `critical` | exclusão mútua |
| | 8 | Histograma concorrente | atomic × histogramas privados |
| | 9 | Estatísticas de um conjunto de dados | `reduction` com `+`, `min`, `max` |
| **Escalonamento** | 10 | `static` × `dynamic` I (carga irregular) | `schedule`, desbalanceamento |
| | 11 | `static` × `dynamic` II (primos) | chunk e custo de escalonamento |
| **Dependências** | 12 | Dependências entre iterações | RAW, WAR, WAW |
| | 13 | Soma de prefixos | reescrever o algoritmo |
| | 14 | Redução definida pelo programador | `declare reduction` |
| **Tarefas** | 15 | Criação de tarefas | `task`, `single`, `firstprivate` |
| | 16 | Tarefas recursivas (Fibonacci) | `taskwait`, granularidade |
| | 17 | Merge Sort com tarefas | divisão e conquista |
| | 18 | Grafo de dependências | `task depend` |
| **Integração** | 19 | Problema integrador | juntar tudo |
| | 20 | Miniaplicação: processamento de imagem | decisões de projeto |
| **Revisão** | — | Questões de pensar e responder | conceitos |
| | — | Cola de diretivas e cláusulas | referência rápida |

---

# Parte 0 — Fundamentos (leia antes de começar)

## 0.1 Modelo mental

- **Memória compartilhada:** todas as threads enxergam o mesmo espaço de endereçamento (mesmas variáveis globais, mesmo heap).
- **Fork/Join:** o programa começa com **uma** thread (a *master*, id 0). Ao entrar em `#pragma omp parallel`, ela faz **fork**: cria/aciona um *pool* de threads. Ao fim do bloco, há **join** (barreira implícita): só a master segue.

```
  master ──────┬─────────────┬────────────── master
               │  thread 1 ──┤
               │  thread 2 ──┤     ← região paralela
               │  thread 3 ──┘
             FORK           JOIN
```

- Em OpenMP **você descreve a concorrência** (diretivas); o runtime decide **como decompor** em unidades de execução e **mapear** nas threads.
- Duas famílias de construções:
  - **Worksharing** (`for`, `sections`, `single`): dividem o trabalho de uma região paralela entre as threads.
  - **Tarefas** (`task`): unidades de trabalho independentes, boas para recursão e grafos de dependência.

## 0.2 Compilar e executar

```bash
g++ -fopenmp -O2 -std=c++17 prog.cpp -o prog
OMP_NUM_THREADS=4 ./prog          # define nº de threads sem recompilar
```

- `-fopenmp` habilita as diretivas (sem ele, os `#pragma omp` são **ignorados** e o programa roda sequencial — sem erro!).
- `-O2` é importante para medir tempo de verdade (sem otimização, os números enganam).
- `-lpthread` (citado nos slides) normalmente já é incluído pelo `-fopenmp` no GCC.

## 0.3 Funções que você vai usar o tempo todo

| Função | O que faz |
|---|---|
| `omp_get_thread_num()` | id da thread atual (0 … n-1) |
| `omp_get_num_threads()` | nº de threads **da região atual** (fora de região paralela vale 1!) |
| `omp_get_max_threads()` | nº de threads que uma próxima região usaria |
| `omp_set_num_threads(n)` | define nº de threads |
| `omp_get_wtime()` | relógio de parede em segundos (`double`) — use para medir tempo |

## 0.4 Variáveis: shared × private

Regra padrão: variáveis declaradas **fora** da região paralela são **`shared`**; as declaradas **dentro** são **privadas** de cada thread. A variável de controle do laço em `omp for` é sempre privada.

| Cláusula | Cada thread tem cópia própria? | Valor inicial da cópia | Valor ao sair |
|---|---|---|---|
| `shared(x)` | não (todas usam a mesma) | — | o que foi escrito |
| `private(x)` | sim | **indefinido** | original **não muda** |
| `firstprivate(x)` | sim | **cópia do valor original** | original **não muda** |
| `lastprivate(x)` | sim | indefinido | recebe o valor da **última iteração (ordem sequencial)** |
| `reduction(op:x)` | sim | elemento neutro de `op` | cópias combinadas na original |

## 0.5 Modelo para medir tempo

```cpp
double t0 = omp_get_wtime();
/* ... código ... */
double t1 = omp_get_wtime();
printf("tempo = %f s\n", t1 - t0);
```

Boas práticas: repetir a medição e usar o **menor** tempo; descartar a primeira execução (cache frio, páginas de memória sendo alocadas); **sempre** verificar que o resultado paralelo bate com o sequencial.

**Métricas:**

- **Speedup** `S = T_seq / T_par`
- **Eficiência** `E = S / nº_threads` (1.0 = ideal)

---

# Nível 1 — Fundamentos

## Exercício 1 — Região paralela e distribuição de iterações

### Enunciado
Dado o programa abaixo, execute com 2, 4 e 8 threads.

```cpp
#pragma omp parallel
{
    int tid = omp_get_thread_num();
    for (int i = 0; i < 8; i++) printf("A: thread %d -> i = %d\n", tid, i);

    #pragma omp for
    for (int i = 0; i < 8; i++) printf("B: thread %d -> i = %d\n", tid, i);
}
```

a) Explique a diferença entre os dois laços. b) Quantas vezes o corpo de cada laço executa? c) Por que o primeiro laço não é distribuído automaticamente?
Depois use `schedule(static)` e `schedule(dynamic,1)` no segundo laço e descreva como as iterações são atribuídas.

### O que você precisa saber
- `#pragma omp parallel` **replica** o bloco: todas as threads executam o **mesmo código completo**.
- Um `for` comum dentro da região é só código: cada thread o executa por inteiro → 8 iterações × nº de threads.
- `#pragma omp for` (dentro de uma região paralela) é uma construção de **worksharing**: ele **divide as iterações** entre as threads do grupo. Cada iteração roda **uma única vez**.
- Ao fim do `omp for` há uma **barreira implícita** (a menos que use `nowait`).
- `schedule(static[,chunk])`: iterações divididas **antes** de executar, em blocos fixos (sem chunk → um bloco contíguo por thread). `schedule(dynamic[,chunk])`: threads **pegam** o próximo bloco quando ficam livres.

### Tente sozinho
Rode com `OMP_NUM_THREADS=4`. Quantas linhas "A:" e "B:" você espera ver? Escreva a previsão antes de rodar.

### Solução comentada

```cpp
// ex01.cpp  —  g++ -fopenmp -O2 ex01.cpp -o ex01
#include <cstdio>
#include <omp.h>

int main() {
    #pragma omp parallel           // FORK: todas as threads executam o bloco inteiro
    {
        int tid = omp_get_thread_num();   // declarada DENTRO => privada de cada thread

        // Laço A: sem "omp for" => é código replicado. Cada thread faz as 8 iterações.
        for (int i = 0; i < 8; i++)
            printf("A: thread %d -> i = %d\n", tid, i);

        // Laço B: "omp for" divide as 8 iterações entre as threads.
        // Mude a linha abaixo para testar as políticas:
        //   #pragma omp for schedule(static)
        //   #pragma omp for schedule(dynamic, 1)
        #pragma omp for
        for (int i = 0; i < 8; i++)
            printf("B: thread %d -> i = %d\n", tid, i);
    }                              // JOIN
    return 0;
}
```

### Para fixar
- **a)** O laço A é um laço ordinário executado por todas as threads; o B é um laço *worksharing*, cujas iterações são particionadas.
- **b)** A: `8 × nthreads` linhas (com 4 threads: 32). B: exatamente **8** linhas (cada `i` uma só vez).
- **c)** Porque `parallel` só cria as threads e replica o código. Quem diz "divida estas iterações" é o `omp for`; o compilador não adivinha se o laço é seguro para dividir.
- **`static`** (8 iterações, 4 threads): thread 0 → i=0,1; thread 1 → i=2,3; etc. Previsível e sempre igual. **`dynamic,1`**: cada thread pega 1 iteração por vez; quem terminar antes pega mais; a distribuição **muda a cada execução**.

---

## Exercício 2 — Operações elementares sobre vetores (SAXPY) com `parallel for`

### Enunciado
Gere vetores `A` e `B` com alguns milhões de elementos e calcule `C[i] = a*A[i] + B[i]`. Faça versão sequencial e paralela (`#pragma omp parallel for`). Imprima nº de threads, tempo sequencial, tempo paralelo e speedup. **Pergunta experimental:** a partir de que tamanho a paralelização compensa?

### O que você precisa saber
- `#pragma omp parallel for` = `parallel` + `for` juntos: cria as threads **e** divide as iterações.
- Só é correto se **as iterações são independentes**: aqui cada `C[i]` depende só de `A[i]` e `B[i]`, e cada thread escreve em posições diferentes de `C` → sem corrida.
- Paralelizar tem **custo fixo** (acordar threads, barreira final). Se o trabalho é pouco, o custo supera o ganho.
- Esse laço é **limitado por memória** (poucas contas por byte lido): o speedup satura quando a largura de banda da RAM acaba, mesmo com muitos núcleos.
- Para um bom benchmark: repetir, pegar o menor tempo, validar o resultado.

### Tente sozinho
Escreva `saxpy_seq` e `saxpy_par` e um laço que teste tamanhos 10³, 10⁴, …, 10⁷.

### Solução comentada

```cpp
// ex02.cpp
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cmath>
#include <omp.h>

// Mede o MENOR tempo de 'reps' execuções de f (reduz ruído do sistema).
template <class F>
double medir(F&& f, int reps = 5) {
    double melhor = 1e30;
    for (int r = 0; r < reps; ++r) {
        double t0 = omp_get_wtime();
        f();
        melhor = std::min(melhor, omp_get_wtime() - t0);
    }
    return melhor;
}

void saxpy_seq(double a, const std::vector<double>& A,
               const std::vector<double>& B, std::vector<double>& C) {
    const long n = (long)A.size();
    for (long i = 0; i < n; ++i) C[i] = a * A[i] + B[i];
}

void saxpy_par(double a, const std::vector<double>& A,
               const std::vector<double>& B, std::vector<double>& C) {
    const long n = (long)A.size();
    // A, B, C são shared (declaradas fora). 'i' é privada (variável do laço).
    // Iterações independentes => não precisa de sincronização.
    #pragma omp parallel for
    for (long i = 0; i < n; ++i) C[i] = a * A[i] + B[i];
}

int main() {
    const double a = 2.5;
    printf("threads = %d\n\n", omp_get_max_threads());
    printf("%10s %12s %12s %9s\n", "N", "T_seq(s)", "T_par(s)", "speedup");

    for (long n = 1000; n <= 10'000'000; n *= 10) {
        std::vector<double> A(n), B(n), Cs(n), Cp(n);
        for (long i = 0; i < n; ++i) { A[i] = i * 0.5; B[i] = 1.0 / (i + 1); }

        double ts = medir([&] { saxpy_seq(a, A, B, Cs); });
        double tp = medir([&] { saxpy_par(a, A, B, Cp); });

        // Validação: SEMPRE compare com a versão sequencial
        bool ok = (Cs == Cp);   // mesmas operações por elemento => resultado idêntico
        printf("%10ld %12.6f %12.6f %8.2fx %s\n", n, ts, tp, ts / tp, ok ? "" : "ERRO!");
    }
    return 0;
}
```

### Para fixar
- Para vetores pequenos (10³–10⁴) o speedup costuma ser **< 1** (paralelo mais lento): o custo de criar/sincronizar threads domina.
- O "ponto de equilíbrio" depende da máquina; costuma aparecer entre 10⁴ e 10⁵ elementos. Anote o seu.
- Em tamanhos grandes, o speedup tende a ficar abaixo do nº de núcleos porque o laço é *memory-bound*.
- Dica: a cláusula `if(n > LIMITE)` em `parallel for` desliga a paralelização para tamanhos pequenos: `#pragma omp parallel for if(n > 50000)`.

---

## Exercício 3 — `private` e `firstprivate`

### Enunciado
```cpp
int x = 10;
#pragma omp parallel private(x)
{ printf("private: thread %d -> x = %d\n", omp_get_thread_num(), x); }
```
a) Execute e analise `x`. b) Troque por `firstprivate(x)`. c) Explique a diferença. Depois faça cada thread executar `x += omp_get_thread_num();`, imprima o valor, e após a região imprima o `x` original. Explique `private` × `firstprivate` quanto a: valor inicial, relação com a original, valor após a região.

### O que você precisa saber
- `private(x)`: cada thread ganha **uma cópia nova** de `x`, **sem inicializar** (lixo). A cópia é descartada ao fim; a original não é tocada.
- `firstprivate(x)`: igual, mas a cópia é **inicializada com o valor que `x` tinha antes da região**.
- Nenhuma das duas "devolve" o valor para fora (isso é `lastprivate` ou `reduction`).
- Ler uma variável `private` não inicializada é comportamento indefinido: o compilador até avisa (`-Wall`).

### Tente sozinho
Preveja: com 4 threads e `firstprivate(x)`, após `x += tid`, quais valores aparecem? E o `x` depois da região?

### Solução comentada

```cpp
// ex03.cpp
#include <cstdio>
#include <omp.h>

int main() {
    int x = 10;

    printf("--- private(x): cópia NÃO inicializada ---\n");
    #pragma omp parallel private(x)
    {
        // x aqui é uma variável nova, com valor lixo. (Só imprimimos para ver!)
        printf("thread %d -> x = %d (lixo)\n", omp_get_thread_num(), x);
    }

    x = 10;
    printf("--- firstprivate(x): cópia inicializada com 10 ---\n");
    #pragma omp parallel firstprivate(x)
    {
        printf("thread %d -> x inicial = %d\n", omp_get_thread_num(), x);
        x += omp_get_thread_num();      // altera SÓ a cópia da thread
        printf("thread %d -> x final   = %d\n", omp_get_thread_num(), x);
    }
    printf("x original depois da região = %d\n", x);   // continua 10

    printf("--- private(x) + atribuição explícita ---\n");
    #pragma omp parallel private(x)
    {
        x = 100;                         // é preciso inicializar antes de usar
        x += omp_get_thread_num();
        printf("thread %d -> x = %d\n", omp_get_thread_num(), x);
    }
    printf("x original depois da região = %d\n", x);   // continua 10
    return 0;
}
```

### Para fixar

| | `private` | `firstprivate` |
|---|---|---|
| Valor inicial da cópia | indefinido | valor da original (10) |
| Relação com a original | independente (só reaproveita o nome) | independente depois da cópia inicial |
| Valor após a região | original **inalterada** | original **inalterada** |

Use `firstprivate` quando a thread precisa **ler** um valor definido antes da região (um parâmetro, um contador inicial) mas deve alterá-lo localmente.

---

# Nível 2 — Sincronização e redução

## Exercício 4 — Condição de corrida e `reduction`

### Enunciado
Calcule `S = Σ i` (i = 1…N), com N = 10.000.000, usando:
```cpp
long long soma = 0;
#pragma omp parallel for
for (long long i = 1; i <= N; i++) soma += i;
```
Execute várias vezes e compare com `N(N+1)/2`. a) Sempre correto? b) Explique. c) Identifique a condição de corrida. Corrija com `reduction(+:soma)` e execute com vários nº de threads. Explique como `reduction` atua.

### O que você precisa saber
- `soma += i` parece uma instrução, mas é **três**: *ler* `soma`, *somar*, *escrever* `soma`.
- **Condição de corrida (race condition):** duas threads acessam o mesmo dado, ao menos uma escreve, sem sincronização. O resultado depende da intercalação.

```
 Thread 0: lê soma (=100) ─────────── soma = 100+1 → escreve 101
 Thread 1:      lê soma (=100) ── soma = 100+2 → escreve 102   ← sobrescreve!
 Esperado: 103.  Obtido: 102 (uma atualização foi PERDIDA)
```
- **`reduction(op:var)`**: o OpenMP cria uma **cópia privada** de `var` por thread, inicializada com o **elemento neutro** de `op` (0 para `+`, 1 para `*`), cada thread acumula na sua cópia (sem disputa) e, no fim, as cópias são **combinadas** na variável original (de forma segura).
- Operadores: `+ * - & | ^ && || min max`.

### Tente sozinho
Rode a versão racy 5 vezes. Os resultados são todos diferentes? Rode com `OMP_NUM_THREADS=1`: o que acontece?

### Solução comentada

```cpp
// ex04.cpp
#include <cstdio>
#include <cstdlib>
#include <omp.h>

int main(int argc, char** argv) {
    const long long N = (argc > 1) ? atoll(argv[1]) : 10'000'000;
    const long long esperado = N * (N + 1) / 2;
    printf("esperado = %lld\n", esperado);

    for (int rep = 0; rep < 5; ++rep) {
        // ---- Versão COM corrida ----
        long long soma = 0;
        #pragma omp parallel for           // 'soma' é shared => corrida!
        for (long long i = 1; i <= N; i++)
            soma += i;                     // ler-somar-escrever NÃO atômico
        printf("racy      : %lld  %s\n", soma, soma == esperado ? "ok" : "ERRADO");

        // ---- Versão com reduction ----
        long long soma_r = 0;
        #pragma omp parallel for reduction(+ : soma_r)
        for (long long i = 1; i <= N; i++)
            soma_r += i;                   // cada thread soma na sua cópia privada
        printf("reduction : %lld  %s\n", soma_r, soma_r == esperado ? "ok" : "ERRADO");
    }
    return 0;
}
```

Por que `long long`? `N(N+1)/2 ≈ 5·10¹³` estoura um `int` de 32 bits. Isso é um erro clássico **independente** de paralelismo.

### Para fixar
- **a)** Não. Com mais de uma thread o resultado racy varia (e quase sempre é menor que o esperado). Com 1 thread sempre dá certo — por isso testar só com 1 thread esconde o bug.
- **b/c)** A corrida está em `soma += i`: leituras e escritas concorrentes de `soma` perdem atualizações.
- **`reduction`:** (1) cada thread recebe `soma_r = 0` privado; (2) acumula sua fatia do laço; (3) no fim da região as parciais são somadas na original. Sem disputa durante o laço → correto **e** rápido.

---

## Exercício 5 — Soma de vetor "à mão": `parallel` + `private` + `reduction`

### Enunciado
Calcule a soma dos elementos de um vetor de N inteiros nas versões sequencial e paralela, usando **apenas** `#pragma omp parallel` (sem `omp for`, `sections` ou `task`), com `private` e `reduction`. A divisão do vetor entre threads deve ser **manual**, via `omp_get_thread_num()` e `omp_get_num_threads()`. Meça os tempos, confira se os resultados são iguais e exiba o speedup.

### O que você precisa saber
- O que o `omp for` faz por você, aqui você faz **manualmente**: cada thread calcula **seu intervalo** `[inicio, fim)` a partir do seu `tid` e do `nthreads`.
- `omp_get_num_threads()` só vale **dentro** da região paralela; fora dela retorna 1.
- Divisão balanceada que também cobre o resto: `inicio = N*tid/nthreads`, `fim = N*(tid+1)/nthreads`. (A dica do enunciado dá o resto à última thread — também funciona, mas desbalanceia um pouco.)
- `reduction(+:soma)` combina as somas parciais; `private(...)` garante que `inicio, fim, i, tid` não sejam compartilhados. Atenção: `private` só se aplica a variáveis **declaradas antes** da região; variáveis declaradas **dentro** dela já são privadas automaticamente.
- Use `long long` para a soma e `long` para os índices.

### Tente sozinho
Desenhe no papel N = 10 e 3 threads: quais são `inicio` e `fim` de cada uma?

### Solução comentada

```cpp
// ex05.cpp
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <omp.h>

int main(int argc, char** argv) {
    const long N = (argc > 1) ? atol(argv[1]) : 10'000'000;

    std::vector<int> vetor(N);
    for (long i = 0; i < N; ++i) vetor[i] = (int)(i + 1);   // soma esperada = N(N+1)/2

    // ---------------- versão sequencial ----------------
    double t0 = omp_get_wtime();
    long long soma_seq = 0;
    for (long i = 0; i < N; ++i) soma_seq += vetor[i];
    double t_seq = omp_get_wtime() - t0;

    // ---------------- versão paralela ------------------
    long long soma = 0;                       // variável da redução (deve ser inicializada!)
    int tid, nthreads;                        // declaradas ANTES => precisam estar em private(...)
    long inicio, fim, i;

    t0 = omp_get_wtime();
    #pragma omp parallel reduction(+ : soma) private(tid, nthreads, inicio, fim, i)
    {
        tid      = omp_get_thread_num();
        nthreads = omp_get_num_threads();      // número REAL de threads desta região

        // Divisão manual em blocos contíguos; o último bloco absorve o resto
        // automaticamente, pois fim da thread t == inicio da thread t+1.
        inicio = N * tid / nthreads;
        fim    = N * (tid + 1) / nthreads;

        // 'soma' aqui é a CÓPIA PRIVADA (começa em 0). Sem corrida.
        for (i = inicio; i < fim; ++i)
            soma += vetor[i];
    }   // ao sair: soma_original = soma_original + soma_t0 + soma_t1 + ...
    double t_par = omp_get_wtime() - t0;

    printf("Tamanho do vetor: %ld  (threads: %d)\n", N, omp_get_max_threads());
    printf("Soma sequencial:  %lld   Tempo: %.4f s\n", soma_seq, t_seq);
    printf("Soma paralela:    %lld   Tempo: %.4f s\n", soma, t_par);
    printf("Resultados iguais? %s\n", soma == soma_seq ? "SIM" : "NAO");
    printf("Speedup: %.2fx\n", t_seq / t_par);
    return 0;
}
```

### Para fixar
- `vetor` é `shared` (só leitura → seguro). `soma` é a única variável escrita por todas → por isso `reduction`.
- Se `inicio/fim/i` fossem `shared` (esqueceu o `private`), as threads sobrescreveriam os limites umas das outras → resultado errado. Teste tirar o `private`!
- **Extensão (false sharing):** troque a redução por um vetor `parciais[tid]` compartilhado, onde cada thread acumula diretamente em `parciais[tid]` dentro do laço. Muito mais lento: posições vizinhas ficam na **mesma linha de cache (64 B)** e as threads ficam "brigando" por ela. Correção: acumular numa variável **local** e só escrever `parciais[tid]` no fim, ou usar *padding* (`alignas(64)`).

---

## Exercício 6 — `lastprivate`

### Enunciado
```cpp
int ultimo = -1;
#pragma omp parallel for
for (int i = 0; i < 20; i++) { ultimo = i; printf("thread %d -> i = %d\n", omp_get_thread_num(), i); }
printf("ultimo = %d\n", ultimo);
```
Execute várias vezes: o valor final é previsível? Por quê? Use `lastprivate(ultimo)` e explique por que passa a valer o da última iteração sequencial. E: qual o valor final de `valor` em
```cpp
int valor;
#pragma omp parallel for lastprivate(valor)
for (int i = 0; i < 20; i++) valor = 10 * i;
```

### O que você precisa saber
- Sem cláusula, `ultimo` é `shared` → todas as threads escrevem nela → "vence" quem escreve por último **no tempo**, que varia a cada execução. (Também é uma corrida de dados.)
- `lastprivate(v)`: cada thread usa cópia privada de `v` e, ao fim do laço, a original recebe o valor da cópia **da thread que executou a última iteração lógica** (`i = 19`), independente de quem terminou primeiro.
- Requisito: a variável deve ser **atribuída** nessa última iteração; se a atribuição for condicional, a original pode receber lixo.

### Solução comentada

```cpp
// ex06.cpp
#include <cstdio>
#include <omp.h>

int main() {
    // (1) shared por padrão: resultado imprevisível (e com corrida)
    int ultimo = -1;
    #pragma omp parallel for
    for (int i = 0; i < 20; i++) ultimo = i;
    printf("sem lastprivate : ultimo = %d (pode variar!)\n", ultimo);

    // (2) lastprivate: sempre o valor da iteração i = 19
    ultimo = -1;
    #pragma omp parallel for lastprivate(ultimo)
    for (int i = 0; i < 20; i++) ultimo = i;
    printf("com lastprivate : ultimo = %d (sempre 19)\n", ultimo);

    // (3) Segundo exemplo do enunciado
    int valor;
    #pragma omp parallel for lastprivate(valor)
    for (int i = 0; i < 20; i++) valor = 10 * i;
    printf("valor = %d\n", valor);       // 190
    return 0;
}
```

### Para fixar
- Valor final de `valor`: **190** (= 10 × 19, a última iteração na ordem sequencial).
- Exemplo de uso: guardar "o último elemento processado", o valor final de uma variável de estado, ou o último `i` quando o laço foi totalmente executado.

---

## Exercício 7 — `atomic` × `critical`

### Enunciado
Com N = 10⁸, implemente `contador++` em três versões: **A** sem sincronização, **B** com `critical`, **C** com `atomic`. Meça os tempos. Responda: a) quais dão resultado correto? b) qual impõe exclusão mútua sobre código arbitrário? c) qual protege uma operação simples sobre uma posição de memória? d) compare os tempos; e) por que `atomic` serve melhor para `++contador`? Analise por que o bloco `critical { soma += x; contador++; printf(...); }` não pode virar um único `atomic`. Extensão: compare `critical`, `atomic` e `reduction` para somar `a[i]`.

### O que você precisa saber
- **`critical`**: só **uma thread por vez** executa o bloco. Serve para **qualquer** conjunto de instruções. Implementado com *lock* (mutex) → caro; threads **esperam** em fila. Blocos `critical` sem nome compartilham o **mesmo** lock global; com nome (`critical(nome)`) só excluem entre si os de mesmo nome.
- **`atomic`**: protege **uma única atualização** de uma posição de memória (`x++`, `x += expr`, `x = x op expr`...), tipicamente via instrução de hardware atômica (`lock add`). Muito mais barato, mas só serve para essa forma simples.
- **`reduction`**: a melhor opção quando o objetivo é um acumulado global, pois evita sincronizar a cada iteração.
- Atenção ao tamanho: com `critical` e N = 10⁸ pode levar **dezenas de segundos**. Comece com N = 10⁷ (argumento do programa).

### Tente sozinho
Preveja a ordem de velocidade entre A, B, C e `reduction` antes de medir.

### Solução comentada

```cpp
// ex07.cpp
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <omp.h>

int main(int argc, char** argv) {
    const long N = (argc > 1) ? atol(argv[1]) : 10'000'000;
    double t0;

    // Versão A: SEM sincronização -> incorreta
    long cA = 0;
    t0 = omp_get_wtime();
    #pragma omp parallel for
    for (long i = 0; i < N; i++) cA++;
    printf("A (sem sync): %ld  tempo=%.4f  %s\n", cA, omp_get_wtime() - t0, cA == N ? "ok" : "ERRADO");

    // Versão B: critical -> correta, porém lenta (lock a cada iteração)
    long cB = 0;
    t0 = omp_get_wtime();
    #pragma omp parallel for
    for (long i = 0; i < N; i++) {
        #pragma omp critical
        { cB++; }
    }
    printf("B (critical): %ld  tempo=%.4f  %s\n", cB, omp_get_wtime() - t0, cB == N ? "ok" : "ERRADO");

    // Versão C: atomic -> correta e bem mais barata (instrução atômica de hardware)
    long cC = 0;
    t0 = omp_get_wtime();
    #pragma omp parallel for
    for (long i = 0; i < N; i++) {
        #pragma omp atomic
        ++cC;
    }
    printf("C (atomic)  : %ld  tempo=%.4f  %s\n", cC, omp_get_wtime() - t0, cC == N ? "ok" : "ERRADO");

    // ---------------- Extensão: soma global de um vetor ----------------
    std::vector<double> a(N, 1.0);
    double s1 = 0, s2 = 0, s3 = 0;

    t0 = omp_get_wtime();
    #pragma omp parallel for
    for (long i = 0; i < N; i++) {
        #pragma omp critical
        s1 += a[i];
    }
    printf("soma critical : %.0f  %.4f s\n", s1, omp_get_wtime() - t0);

    t0 = omp_get_wtime();
    #pragma omp parallel for
    for (long i = 0; i < N; i++) {
        #pragma omp atomic
        s2 += a[i];
    }
    printf("soma atomic   : %.0f  %.4f s\n", s2, omp_get_wtime() - t0);

    t0 = omp_get_wtime();
    #pragma omp parallel for reduction(+ : s3)
    for (long i = 0; i < N; i++) s3 += a[i];
    printf("soma reduction: %.0f  %.4f s\n", s3, omp_get_wtime() - t0);
    return 0;
}
```

### Para fixar
- **a)** B e C. A é incorreta.
- **b)** `critical`. **c)** `atomic`.
- **d)** Tipicamente `atomic` é várias vezes mais rápido que `critical` (e ambos muito mais lentos que o sequencial! sincronizar a cada iteração mata o paralelismo).
- **e)** `++contador` é exatamente uma atualização simples de uma posição de memória → mapeia numa instrução atômica; `critical` custa um lock inteiro para isso.
- **Por que o bloco com 3 comandos não vira um `atomic`:** `atomic` cobre **um único** comando de atualização de **uma** variável. O bloco atualiza **duas** variáveis e faz E/S (`printf`). Dois `atomic` separados garantiriam cada atualização, mas **não** que `soma` e `contador` mudem "juntos" nem que o `printf` mostre um estado consistente. Isso exige exclusão mútua do bloco todo → `critical`. (E mais: `printf` dentro de seção crítica é ruim para desempenho — veja a pergunta 4 da revisão.)
- **Extensão:** `reduction` é a mais adequada: cada thread soma localmente e a sincronização acontece **uma vez por thread**, não uma por elemento. `atomic` vem em segundo; `critical` por último.


---

## Exercício 8 — Histograma concorrente

### Enunciado
Gere um vetor de inteiros em [0, 255] e construa o histograma. A paralelização ingênua de `hist[v[i]]++` gera condição de corrida. Você deve: **demonstrar o erro**, **corrigir com `atomic`** e **implementar histogramas privados por thread + combinação posterior**. Compare `atomic` × privados.

### O que você precisa saber
- Aqui a posição `hist[v[i]]` é **decidida pelos dados**; duas iterações diferentes podem cair no mesmo *bin* → corrida.
- Para **demonstrar o erro**, basta conferir que `Σ hist[b] == N`; na versão com corrida vai faltar contagem.
- **Opção 1 — `atomic`:** simples, correta; mas threads disputam os mesmos 256 bins (contenção) e cada incremento custa uma operação atômica.
- **Opção 2 — histogramas privados:** cada thread tem seu `hist_local[256]`, incrementa **sem sincronização**, e só ao final as 256 parciais são somadas no global (uma seção crítica curta por thread). Troca **memória** (nT × 256) por **velocidade**.
- `nowait` remove a barreira ao fim do `omp for` — aqui seguro, pois o passo seguinte (a combinação) usa só a cópia local da própria thread.
- OpenMP ≥ 4.5 permite **redução sobre arrays**: `reduction(+:h[:256])` faz exatamente o padrão "privado + combinação" automaticamente.

### Tente sozinho
Faça os 3 métodos e meça. Depois teste um vetor onde **todos** os valores são iguais (contenção máxima): o que acontece com `atomic`?

### Solução comentada

```cpp
// ex08.cpp
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <random>
#include <numeric>
#include <algorithm>
#include <omp.h>

constexpr int BINS = 256;

int main(int argc, char** argv) {
    const long N = (argc > 1) ? atol(argv[1]) : 20'000'000;
    const bool uniforme = !(argc > 2 && atoi(argv[2]) == 1);   // 2º arg=1 => todos iguais

    std::vector<unsigned char> v(N);
    std::mt19937 gen(42);
    for (long i = 0; i < N; i++) v[i] = uniforme ? (unsigned char)(gen() & 255) : 7;

    auto total = [](const std::vector<long>& h) { return std::accumulate(h.begin(), h.end(), 0L); };
    double t0;

    // ---- Sequencial (referência) ----
    std::vector<long> hs(BINS, 0);
    t0 = omp_get_wtime();
    for (long i = 0; i < N; i++) hs[v[i]]++;
    printf("sequencial : %.4f s  total=%ld\n", omp_get_wtime() - t0, total(hs));

    // ---- Ingênua: CORRIDA ----
    std::vector<long> hr(BINS, 0);
    t0 = omp_get_wtime();
    #pragma omp parallel for
    for (long i = 0; i < N; i++) hr[v[i]]++;
    printf("ingenua    : %.4f s  total=%ld  (esperado %ld) %s\n",
           omp_get_wtime() - t0, total(hr), N, total(hr) == N ? "" : "<-- CORRIDA!");

    // ---- atomic ----
    std::vector<long> ha(BINS, 0);
    t0 = omp_get_wtime();
    #pragma omp parallel for
    for (long i = 0; i < N; i++) {
        #pragma omp atomic
        ha[v[i]]++;
    }
    printf("atomic     : %.4f s  total=%ld  %s\n", omp_get_wtime() - t0, total(ha), ha == hs ? "ok" : "ERRADO");

    // ---- Histogramas privados + combinação ----
    std::vector<long> hp(BINS, 0);
    t0 = omp_get_wtime();
    #pragma omp parallel
    {
        std::vector<long> local(BINS, 0);        // declarado DENTRO => privado de cada thread

        #pragma omp for nowait                   // cada thread conta apenas na sua cópia
        for (long i = 0; i < N; i++) local[v[i]]++;

        #pragma omp critical(combina)            // seção curta: só 256 somas por thread
        for (int b = 0; b < BINS; b++) hp[b] += local[b];
    }
    printf("privados   : %.4f s  total=%ld  %s\n", omp_get_wtime() - t0, total(hp), hp == hs ? "ok" : "ERRADO");

    // ---- Redução sobre array (OpenMP >= 4.5) ----
    long hred[BINS] = {0};
    t0 = omp_get_wtime();
    #pragma omp parallel for reduction(+ : hred[:BINS])
    for (long i = 0; i < N; i++) hred[v[i]]++;
    printf("reduction[]: %.4f s  %s\n", omp_get_wtime() - t0,
           std::equal(hs.begin(), hs.end(), hred) ? "ok" : "ERRADO");
    return 0;
}
```

Execute: `./ex08` e depois `./ex08 20000000 1` (todos os valores iguais).

### Para fixar
- **Ingênua:** o total fica **menor** que N (incrementos perdidos). Mostra a corrida.
- **`atomic`:** correta; com dados uniformes sofre contenção moderada; com todos os valores iguais, todas as threads batem no **mesmo** endereço e fica quase sequencial (ou pior).
- **Privados:** normalmente muito mais rápido; o custo extra é só a combinação final (256 somas por thread). Escala bem com o nº de threads.
- **Regra geral:** *evite compartilhar o que é escrito com frequência; acumule localmente e combine no fim.*

---

## Exercício 9 — Estatísticas de um conjunto de dados (redução padrão)

### Enunciado
Dado um vetor de `double`, calcule em paralelo: soma, média, mínimo, máximo e soma dos quadrados, usando `reduction(+:soma,soma_quadrados)`, `reduction(min:minimo)`, `reduction(max:maximo)`. Depois calcule média e desvio padrão. Compare com uma solução baseada em `critical` e explique a diferença de desempenho.

### O que você precisa saber
- Uma **mesma** diretiva pode ter **várias** cláusulas `reduction`, cada uma com seu operador.
- Inicializações automáticas: `+` → 0, `min` → maior valor possível, `max` → menor valor possível.
- Forma permitida de atualizar uma variável de redução `min/max`: `if (x < minimo) minimo = x;`.
- Média = `soma/n`. Variância (populacional) = `soma_q/n − média²`. Desvio padrão = `√variância`.
  *Cuidado:* essa fórmula pode perder precisão (cancelamento) se os valores forem grandes e a variância pequena; o algoritmo de Welford é mais estável, mas não é redução simples.
- Soma de `double` em paralelo pode diferir **nos últimos dígitos** da sequencial: a **ordem** das somas muda e ponto flutuante **não é associativo**. Isso não é bug.

### Solução comentada

```cpp
// ex09.cpp
#include <cstdio>
#include <cstdlib>
#include <cfloat>
#include <cmath>
#include <vector>
#include <random>
#include <omp.h>

int main(int argc, char** argv) {
    const long N = (argc > 1) ? atol(argv[1]) : 20'000'000;
    std::vector<double> v(N);
    std::mt19937_64 gen(1);
    std::uniform_real_distribution<double> dist(-100.0, 100.0);
    for (auto& x : v) x = dist(gen);

    // ---------- Versão com reduction ----------
    double soma = 0, soma_q = 0, minimo = DBL_MAX, maximo = -DBL_MAX;
    double t0 = omp_get_wtime();
    #pragma omp parallel for reduction(+ : soma, soma_q) \
                             reduction(min : minimo) reduction(max : maximo)
    for (long i = 0; i < N; i++) {
        double x = v[i];
        soma   += x;
        soma_q += x * x;
        if (x < minimo) minimo = x;       // forma aceita para redução min
        if (x > maximo) maximo = x;       // forma aceita para redução max
    }
    double t_red = omp_get_wtime() - t0;

    double media = soma / N;
    double desvio = std::sqrt(std::fmax(0.0, soma_q / N - media * media));   // fmax evita raiz de negativo por arredondamento
    printf("reduction: soma=%.6f media=%.6f min=%.4f max=%.4f desvio=%.6f  (%.4f s)\n",
           soma, media, minimo, maximo, desvio, t_red);

    // ---------- Versão com critical (ruim de propósito) ----------
    double s2 = 0, q2 = 0, mn2 = DBL_MAX, mx2 = -DBL_MAX;
    t0 = omp_get_wtime();
    #pragma omp parallel for
    for (long i = 0; i < N; i++) {
        double x = v[i];
        #pragma omp critical             // lock a CADA iteração: threads fazem fila
        {
            s2 += x; q2 += x * x;
            if (x < mn2) mn2 = x;
            if (x > mx2) mx2 = x;
        }
    }
    printf("critical : soma=%.6f  (%.4f s)\n", s2, omp_get_wtime() - t0);
    return 0;
}
```

### Para fixar
- `critical` por iteração serializa praticamente todo o laço (só o cálculo de `x` fica paralelo) e paga o custo do lock N vezes → pode ser **mais lento que o sequencial**.
- `reduction` acumula em variáveis privadas e combina **uma vez por thread** → escala.
- (Meio-termo manual): acumular em locais e fazer **um** `critical` por thread no final — é o que o `reduction` faz por baixo.

---

# Nível 3 — Escalonamento

## Exercício 10 — `static` × `dynamic` I: carga irregular

### Enunciado
Crie um laço em que `trabalho(i)` custe ~`i` (ou `i²`) operações, de modo que as últimas iterações sejam bem mais caras. Execute com `schedule(static)` e `schedule(dynamic)`, testando diferentes *chunks*. Cada thread deve registrar **quantas iterações** executou e o **tempo gasto**. Identifique quando `dynamic` compensa seu custo maior.

Complemento (atividade de 31/out): laço com `fib(i)` recursivo, variando `static,1`, `static,4`, `dynamic,1`, `dynamic,4`.

### O que você precisa saber
- **`schedule(static)`** (sem chunk): divide em `nthreads` blocos **contíguos** do mesmo tamanho. Custo de escalonamento ≈ 0, mas se as iterações finais são mais pesadas, a **última thread** fica sobrecarregada e as outras esperam na barreira → **desbalanceamento**.
- **`schedule(static, c)`**: blocos de `c` iterações distribuídos em **rodízio** (round-robin). Com `c` pequeno, o trabalho caro se espalha entre todas as threads.
- **`schedule(dynamic, c)`**: threads **pedem** blocos de `c` iterações quando ficam livres. Balanceia automaticamente; cada pedido tem custo (acesso a uma fila compartilhada).
- **`guided`**: dynamic com blocos que começam grandes e vão diminuindo.
- **`schedule(runtime)`**: a política vem da variável de ambiente `OMP_SCHEDULE` → você testa várias configurações **sem recompilar**.
- Para medir **por thread**: um vetor indexado por `tid`. Cuidado com **false sharing**: elementos vizinhos de um vetor de `double`/`long` caem na mesma linha de cache. Use `struct alignas(64)` para separar.
- Ao testar custo "artificial", faça o resultado **ser usado** (some-o numa redução); senão o compilador pode eliminar o cálculo.

### Tente sozinho
Preveja o tempo de cada thread com `static` e 4 threads quando `trabalho(i) ∝ i`. Qual thread acaba por último? Quanto mais trabalho ela faz do que a primeira?

### Solução comentada

```cpp
// ex10.cpp  —  rode: OMP_SCHEDULE="dynamic,4" ./ex10
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <omp.h>

// Estatística por thread, alinhada em 64 bytes para evitar false sharing
struct alignas(64) Stat { long iters = 0; double tempo = 0.0; };

// Custo proporcional a i*k (as últimas iterações são bem mais caras)
double trabalho(long i, long k) {
    double x = 0;
    for (long j = 0; j < i * k; ++j) x += std::sin(j * 1e-3);
    return x;
}

int main(int argc, char** argv) {
    const long N = (argc > 1) ? atol(argv[1]) : 1000;
    const long K = (argc > 2) ? atol(argv[2]) : 100;

    const int nt = omp_get_max_threads();
    std::vector<Stat> st(nt);
    double acumulado = 0;                                 // evita que o compilador remova o trabalho

    double t0 = omp_get_wtime();
    #pragma omp parallel for schedule(runtime) reduction(+ : acumulado)
    for (long i = 0; i < N; ++i) {
        const int tid = omp_get_thread_num();
        double a = omp_get_wtime();
        acumulado += trabalho(i, K);
        st[tid].tempo += omp_get_wtime() - a;             // cada thread só escreve na sua posição
        st[tid].iters++;
    }
    double total = omp_get_wtime() - t0;

    omp_sched_t kind; int chunk;
    omp_get_schedule(&kind, &chunk);
    printf("schedule(kind=%d, chunk=%d)  tempo total = %.3f s  (acum=%.3f)\n", (int)kind, chunk, total, acumulado);
    for (int t = 0; t < nt; ++t)
        printf("  thread %d: %5ld iterações, %.3f s\n", t, st[t].iters, st[t].tempo);
    return 0;
}
```

Rodando várias políticas de uma vez:

```bash
g++ -fopenmp -O2 ex10.cpp -o ex10
export OMP_NUM_THREADS=4
for s in "static" "static,1" "static,10" "dynamic" "dynamic,1" "dynamic,16" "guided"; do
  echo "== $s =="; OMP_SCHEDULE="$s" ./ex10
done
```

### Complemento: o exercício do Fibonacci (`fib_sched`)

```cpp
// fib_sched.cpp — g++ -fopenmp -O2 fib_sched.cpp -o fib_sched
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <omp.h>

long fib(int n) { return n <= 1 ? n : fib(n - 1) + fib(n - 2); }

int main(int argc, char** argv) {
    // O enunciado fala em 35 iterações, mas o código base usa N=46 (fib(45) recursivo leva muitos segundos!).
    const int N = (argc > 1) ? atoi(argv[1]) : 40;
    std::vector<long> v(N);

    double t0 = omp_get_wtime();
    #pragma omp parallel for schedule(runtime)   // troque por static,1 / static,4 / dynamic,1 / dynamic,4
    for (int i = 0; i < N; i++) {
        v[i] = fib(i);
        printf("thread %d calculou fib(%d) = %ld\n", omp_get_thread_num(), i, v[i]);
    }
    printf("\nTempo total: %f s\n", omp_get_wtime() - t0);
    return 0;
}
```

### Para fixar
- **`static` puro:** a thread que recebe o bloco final faz muito mais trabalho (para custo ∝ i, a última thread faz ~7× o trabalho da primeira com 4 threads). O tempo total = tempo dessa thread.
- **`static,1` / `dynamic`:** distribuem melhor; com custo crescente o ganho é grande.
- **`dynamic` compensa quando** (i) o custo por iteração é **irregular/imprevisível** e (ii) cada iteração (ou chunk) é **grande o bastante** para amortizar o custo de pedir trabalho. Se as iterações são minúsculas e uniformes, `static` vence.
- **Por que o chunk muda o comportamento:** chunk pequeno → melhor balanceamento, mais pedidos de trabalho (overhead). Chunk grande → menos overhead, mas "pedaços" caros podem ficar com uma só thread (volta o desbalanceamento).
- Em `fib_sched` com `static` puro, as threads que pegam `i` pequenos terminam quase instantaneamente — exatamente o que o enunciado quer que você observe.

---

## Exercício 11 — `static` × `dynamic` II: busca de números primos

### Enunciado
Para todos os inteiros em [A, B], determine os primos por divisão por tentativa até √n. Compare `static`, `dynamic,1`, `dynamic,10`, `dynamic,100`. O conjunto de primos deve ser **idêntico** em todas as execuções. **O menor chunk produz necessariamente o menor tempo? Justifique.**

### O que você precisa saber
- Teste de primalidade: se `n` tem divisor, tem um ≤ √n. Para pares, custo ≈ 1 operação; para primos, ~√n/2 divisões. A irregularidade vem **do algoritmo**.
- Para verificar "o mesmo conjunto" sem guardar tudo: compare **contagem** e **soma** (e/ou um vetor de flags).
- `omp_set_schedule(kind, chunk)` + `schedule(runtime)` permite testar várias políticas **dentro do mesmo programa**.
- Contar primos: `reduction(+:contagem, soma)`.

### Solução comentada

```cpp
// ex11.cpp
#include <cstdio>
#include <cstdlib>
#include <omp.h>

bool eh_primo(long n) {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    for (long d = 3; d * d <= n; d += 2)          // só testa ímpares até sqrt(n)
        if (n % d == 0) return false;
    return true;
}

struct Res { long cont; long long soma; };

Res conta_seq(long A, long B) {
    Res r{0, 0};
    for (long n = A; n <= B; ++n) if (eh_primo(n)) { r.cont++; r.soma += n; }
    return r;
}

Res conta_par(long A, long B) {
    long cont = 0; long long soma = 0;
    #pragma omp parallel for schedule(runtime) reduction(+ : cont, soma)
    for (long n = A; n <= B; ++n)
        if (eh_primo(n)) { cont++; soma += n; }
    return Res{cont, soma};
}

int main(int argc, char** argv) {
    const long A = (argc > 1) ? atol(argv[1]) : 1;
    const long B = (argc > 2) ? atol(argv[2]) : 5'000'000;

    double t0 = omp_get_wtime();
    Res ref = conta_seq(A, B);
    printf("sequencial       : %ld primos, soma=%lld, %.3f s\n", ref.cont, ref.soma, omp_get_wtime() - t0);

    struct Cfg { const char* nome; omp_sched_t kind; int chunk; };
    Cfg cfgs[] = { {"static",       omp_sched_static,  0},
                   {"static,100",   omp_sched_static,  100},
                   {"dynamic,1",    omp_sched_dynamic, 1},
                   {"dynamic,10",   omp_sched_dynamic, 10},
                   {"dynamic,100",  omp_sched_dynamic, 100},
                   {"dynamic,1000", omp_sched_dynamic, 1000} };

    for (auto& c : cfgs) {
        omp_set_schedule(c.kind, c.chunk);           // define o que 'schedule(runtime)' vai usar
        t0 = omp_get_wtime();
        Res r = conta_par(A, B);
        double t = omp_get_wtime() - t0;
        printf("%-16s : %ld primos, soma=%lld, %.3f s  %s\n", c.nome, r.cont, r.soma, t,
               (r.cont == ref.cont && r.soma == ref.soma) ? "ok" : "DIFERENTE!");
    }
    return 0;
}
```

### Para fixar
- **Não necessariamente.** `dynamic,1` balanceia melhor, mas cada iteração exige acessar a fila compartilhada de trabalho (operação atômica/lock). Se o custo por iteração é pequeno frente a esse overhead, chunks maiores ganham. Além disso, chunks maiores preservam **localidade** (iterações vizinhas em sequência).
- Aqui as iterações são razoavelmente pesadas (milhares de divisões para primos grandes), então `dynamic,1` pode ficar próximo do melhor; o ponto ótimo costuma ser um chunk **intermediário** (10–1000). Chunk **grande demais** reintroduz desbalanceamento.
- `static` puro costuma ficar pior: números maiores (custo maior) concentram-se nas últimas threads; `static,100` (rodízio) já melhora muito.
- Em todas as execuções, `cont` e `soma` devem coincidir com o sequencial — se não, há bug (o paralelismo não pode mudar o resultado).

---

# Nível 4 — Dependências entre iterações

## Exercício 12 — Dependências: verdadeira, antidependência, saída

### Enunciado (atividade de 31/out)
Para cada caso, diga se o laço pode receber `#pragma omp parallel for` diretamente; se não, reescreva.

```cpp
// Caso 1 (RAW – dependência verdadeira)
for (int i = 1; i < N; i++) a[i] = a[i-1] + 2;

// Caso 2 (WAR – antidependência)
for (int i = 0; i < N-1; i++) b[i] = b[i+1] + 1;

// Caso 3 (WAW – saída, condicional)
int best_idx = -1;
for (int i = 0; i < N; i++) if (a[i] > 0) best_idx = i;
```

### O que você precisa saber
Uma iteração é paralelizável quando **nenhuma iteração depende de dado produzido/consumido por outra** (dependência *loop-carried*, "carregada pelo laço").

| Tipo | Sigla | Significado | Exemplo |
|---|---|---|---|
| Verdadeira | **RAW** (read after write) | iteração `i` **lê** o que `i-1` **escreveu** | `a[i] = a[i-1]+2` |
| Anti | **WAR** (write after read) | `i` **lê** o que `i+1` vai **escrever** | `b[i] = b[i+1]+1` |
| Saída | **WAW** (write after write) | várias iterações **escrevem** o mesmo lugar | `best_idx = i` |

- **RAW** é a "verdadeira": a informação flui de uma iteração para outra → precisa **mudar o algoritmo** (forma fechada, prefixo...).
- **WAR e WAW** são dependências "de nome": resolvem-se com **renomeação** (usar outro vetor, cópias privadas) — não há fluxo real de informação.
- Reducões resolvem WAW quando o resultado é uma agregação (soma, máx...).

### Tente sozinho
Para cada caso responda: qual iteração produz o dado, qual consome, e como eliminar a dependência.

### Solução comentada

```cpp
// ex12.cpp
#include <cstdio>
#include <vector>
#include <algorithm>
#include <omp.h>

int main() {
    const int N = 1'000'000;

    // =========== Caso 1: RAW =============
    // a[i] = a[i-1] + 2  com a[0] dado  =>  a[i] = a[0] + 2*i   (forma fechada)
    std::vector<long> a_seq(N), a_par(N);
    a_seq[0] = a_par[0] = 5;
    for (int i = 1; i < N; i++) a_seq[i] = a_seq[i - 1] + 2;       // versão sequencial

    // A dependência foi ELIMINADA reescrevendo o algoritmo: cada a[i] depende só de a[0] e i.
    #pragma omp parallel for
    for (int i = 1; i < N; i++) a_par[i] = a_par[0] + 2L * i;      // a_par[0] só é LIDO
    printf("Caso 1: %s\n", a_seq == a_par ? "ok" : "ERRO");
    // Para recorrências gerais (que não têm forma fechada) use soma de prefixos (Exercício 13).

    // =========== Caso 2: WAR =============
    // b[i] precisa do valor ANTIGO de b[i+1]. Em paralelo, b[i+1] pode já ter sido sobrescrito.
    std::vector<int> b(N), b_seq(N), b_novo(N);
    for (int i = 0; i < N; i++) b[i] = i % 17;
    b_seq = b;
    for (int i = 0; i < N - 1; i++) b_seq[i] = b_seq[i + 1] + 1;   // sequencial: lê antes de sobrescrever

    // Solução: ler de um vetor e escrever em OUTRO (renomeação). Leitura e escrita não se cruzam.
    b_novo = b;                                                    // preserva b[N-1], que não muda
    #pragma omp parallel for
    for (int i = 0; i < N - 1; i++) b_novo[i] = b[i + 1] + 1;      // lê 'b', escreve 'b_novo'
    printf("Caso 2: %s\n", b_seq == b_novo ? "ok" : "ERRO");

    // =========== Caso 3: WAW condicional =============
    std::vector<int> v(N);
    for (int i = 0; i < N; i++) v[i] = (i % 7 == 3 && i < 900000) ? 1 : -1;

    int best_seq = -1;
    for (int i = 0; i < N; i++) if (v[i] > 0) best_seq = i;        // índice da ÚLTIMA posição positiva

    // O resultado = MAIOR índice que satisfaz a condição => é uma redução de máximo.
    int best_par = -1;
    #pragma omp parallel for reduction(max : best_par)
    for (int i = 0; i < N; i++)
        if (v[i] > 0 && i > best_par) best_par = i;                // cópia privada, combinação por max
    printf("Caso 3 (reduction max): %s (idx=%d)\n", best_seq == best_par ? "ok" : "ERRO", best_par);

    // Alternativa: lastprivate NÃO resolve (a atribuição é condicional; a última iteração pode não atribuir).
    // Alternativa 2: cada thread guarda seu melhor índice local e o master combina (equivale ao reduction).
    // Alternativa 3: percorrer de trás para frente e parar na primeira positiva (sequencial, mas O(distância)).
    return 0;
}
```

### Para fixar
**Caso 1.**
- Quem produz `a[i-1]`? A iteração `i-1`. Quem consome? A iteração `i`. Não podem rodar ao mesmo tempo.
- Direto com `parallel for`: **não**.
- Correção: forma fechada `a[i] = a[0] + 2i`, ou prefixo (Ex. 13).

**Caso 2.**
- A iteração `i` **lê** `b[i+1]`; a iteração `i+1` **escreve** em `b[i+1]`. Se `i+1` rodar antes de `i`, `i` lerá o valor novo (errado).
- Direto: **não**. Correção: vetor temporário/saída.

**Caso 3.**
- Muitas iterações (todas com `a[i]>0`) escrevem em `best_idx`; o valor final depende de quem escreve **por último no tempo**, não da ordem lógica.
- Direto: **não** (é corrida, e não determinístico). Correção: `reduction(max:...)` ou índice por thread + combinação.

---

## Exercício 13 — Soma de prefixos paralela

### Enunciado
```cpp
for (int i = 1; i < N; ++i) A[i] = A[i] + A[i-1];
```
Esse laço não pode receber `parallel for` direto. Produza uma versão paralela correta: (1) cada thread calcula o prefixo sequencial do **seu bloco**; (2) os totais dos blocos são combinados; (3) cada bloco recebe o **deslocamento** dos blocos anteriores. **Conceitual:** identifique a dependência RAW e explique como a transformação a elimina.

### O que você precisa saber
- A soma de prefixos (*scan*) parece inerentemente sequencial, mas é associativa → pode ser feita em **3 fases**:

```
Dados:     [ 1 2 3 | 4 5 6 | 7 8 9 ]          (3 blocos / 3 threads)
Fase 1:    [ 1 3 6 | 4 9 15 | 7 15 24 ]       prefixo LOCAL (paralelo, independentes)
Totais:       6        15        24
Fase 2:    deslocamentos: bloco0=0, bloco1=6, bloco2=6+15=21   (sequencial, minúsculo)
Fase 3:    [ 1 3 6 | 10 15 21 | 28 36 45 ]    soma o deslocamento (paralelo)
```
- Precisamos de **barreiras** entre as fases (`#pragma omp barrier` ou barreira implícita do `single`): a fase 2 só pode começar quando **todas** as totais da fase 1 existirem.
- O número de threads é conhecido só **dentro** da região → o vetor de deslocamentos deve ser dimensionado lá, por uma thread (`single`).
- Custo: ~2N operações (vs N sequencial) → speedup máximo ≈ nT/2; além disso é limitado por memória.
- (Existe a cláusula `scan` no OpenMP 5.0 — mas aqui o objetivo é entender o algoritmo.)

### Solução comentada

```cpp
// ex13.cpp
#include <cstdio>
#include <vector>
#include <omp.h>

void prefixo_seq(std::vector<long long>& A) {
    for (size_t i = 1; i < A.size(); ++i) A[i] += A[i - 1];       // RAW: A[i] lê o que A[i-1] escreveu
}

void prefixo_par(std::vector<long long>& A) {
    const long N = (long)A.size();
    std::vector<long long> off;                                    // off[t+1] = total do bloco t

    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        const int nt  = omp_get_num_threads();

        #pragma omp single                // UMA thread dimensiona o vetor (nt só é conhecido aqui)
        off.assign(nt + 1, 0);            // barreira implícita no fim do single: todos veem 'off'

        // Bloco desta thread: [ini, fim)
        const long ini = N * tid / nt;
        const long fim = N * (tid + 1) / nt;

        // ---- Fase 1: prefixo LOCAL, sequencial dentro do bloco (blocos independentes) ----
        for (long i = ini + 1; i < fim; ++i) A[i] += A[i - 1];
        if (fim > ini) off[tid + 1] = A[fim - 1];     // total do bloco (cada thread escreve numa posição distinta)

        #pragma omp barrier               // espera TODAS as fases 1 terminarem

        // ---- Fase 2: transforma totais em deslocamentos acumulados (só 1 thread; nt é pequeno) ----
        #pragma omp single
        for (int t = 1; t <= nt; ++t) off[t] += off[t - 1];   // off[t] = soma dos blocos 0..t-1
        // (barreira implícita aqui: os deslocamentos estão prontos)

        // ---- Fase 3: soma o deslocamento dos blocos anteriores (paralelo) ----
        const long long d = off[tid];
        for (long i = ini; i < fim; ++i) A[i] += d;
    }
}

int main() {
    const long N = 20'000'000;
    std::vector<long long> A(N), B;
    for (long i = 0; i < N; ++i) A[i] = (i % 10) + 1;
    B = A;

    double t0 = omp_get_wtime(); prefixo_seq(A);  double ts = omp_get_wtime() - t0;
    t0 = omp_get_wtime();        prefixo_par(B);  double tp = omp_get_wtime() - t0;

    printf("iguais? %s | seq=%.4f s par=%.4f s speedup=%.2fx\n",
           A == B ? "SIM" : "NAO", ts, tp, ts / tp);
    return 0;
}
```

### Para fixar
- **RAW original:** a iteração `i` *lê* `A[i-1]`, que a iteração `i-1` *escreveu* → as iterações formam uma cadeia.
- **Como a transformação elimina a dependência:**
  - Na fase 1, cada bloco só usa elementos **do próprio bloco** → a cadeia fica restrita a cada thread (sequencial *dentro* do bloco, independente *entre* blocos).
  - A informação que cruza os blocos (o total dos blocos anteriores) é transmitida **uma vez**, via vetor `off`, entre fases separadas por barreira.
  - Na fase 3, cada elemento só soma uma constante `d` → totalmente independente.
- Ou seja: a dependência não "sumiu" — foi **reorganizada** para uma pequena etapa sequencial (nt valores), em vez de N.

---

## Exercício 14 — Redução definida pelo programador: máximo e posição

### Enunciado
Encontre simultaneamente o maior valor e o índice onde ocorre, usando um tipo:
```cpp
typedef struct { double valor; long indice; } maxloc_t;
```
Crie um operador com `#pragma omp declare reduction(...)`. Em empate, vence o **menor índice**. **Conceitual:** por que duas reduções independentes (uma para o máximo, outra para o índice) não funcionam?

### O que você precisa saber
- Formato: 
  `#pragma omp declare reduction(nome : tipo : omp_out = combinador(omp_out, omp_in)) initializer(omp_priv = valor_neutro)`
  - `omp_out`: acumulador (resultado); `omp_in`: o outro operando.
  - `initializer(omp_priv = ...)`: valor inicial das cópias privadas → deve ser o **elemento neutro** do combinador.
- O combinador precisa ser **associativo** (e, idealmente, comutativo): as parciais são combinadas em ordem imprevisível. A regra "maior valor; empate → menor índice" é associativa e comutativa → ok.
- Neutro: `{ -∞, LONG_MAX }` (qualquer elemento real o vence).
- Depois de declarada, usa-se `reduction(nome : variavel)`.

### Solução comentada

```cpp
// ex14.cpp
#include <cstdio>
#include <vector>
#include <limits>
#include <climits>
#include <random>
#include <omp.h>

struct MaxLoc { double valor; long indice; };

// Combina duas parciais: maior valor; empate => menor índice.
inline MaxLoc combina(const MaxLoc& a, const MaxLoc& b) {
    if (a.valor > b.valor) return a;
    if (b.valor > a.valor) return b;
    return (a.indice < b.indice) ? a : b;       // empate: o menor índice vence
}
inline MaxLoc neutro() { return MaxLoc{ -std::numeric_limits<double>::infinity(), LONG_MAX }; }

// Registro do operador de redução "maxloc" para o tipo MaxLoc
#pragma omp declare reduction(maxloc : MaxLoc : omp_out = combina(omp_out, omp_in)) \
                    initializer(omp_priv = neutro())

int main() {
    const long N = 10'000'000;
    std::vector<double> v(N);
    std::mt19937 gen(7);
    std::uniform_int_distribution<int> dist(0, 1000);          // muitos empates de propósito
    for (auto& x : v) x = dist(gen);

    MaxLoc seq = neutro();
    for (long i = 0; i < N; ++i) seq = combina(seq, MaxLoc{v[i], i});

    MaxLoc par = neutro();                                      // valor inicial da variável original
    #pragma omp parallel for reduction(maxloc : par)
    for (long i = 0; i < N; ++i)
        par = combina(par, MaxLoc{v[i], i});                    // 'par' é a cópia privada de cada thread

    printf("seq: valor=%.0f indice=%ld\npar: valor=%.0f indice=%ld  -> %s\n",
           seq.valor, seq.indice, par.valor, par.indice,
           (seq.valor == par.valor && seq.indice == par.indice) ? "IGUAIS" : "DIFERENTES");
    return 0;
}
```

### Para fixar
- **Por que duas reduções separadas falham:** `max` do valor e `max` (ou `min`) do índice são calculados **independentemente**, então o índice resultante não precisa corresponder ao valor. Exemplo: thread 0 acha `(valor=9, idx=3)`; thread 1 acha `(valor=5, idx=70)`. Reduzindo separado: valor = 9, índice = max(3,70) = 70 → o par `(9, 70)` **não existe** no vetor. O par precisa ser tratado como **uma unidade indivisível**.
- Se o neutro fosse mal escolhido (por ex. `{0, 0}`) e todos os valores fossem negativos, o resultado seria incorreto: sempre valide o neutro.


---

# Nível 5 — Tarefas (`task`)

## Exercício 15 — Criação de tarefas, `single` e `firstprivate` em tarefas

### Enunciado
```cpp
#pragma omp parallel
{
  #pragma omp single
  {
    for (int i = 0; i < 12; i++) {
      #pragma omp task
      { printf("tarefa %d executada pela thread %d\n", i, omp_get_thread_num()); }
    }
  }
}
```
Execute várias vezes. a) Todas as tarefas rodam na thread que as criou? b) A ordem é previsível? c) Qual a função do `single`? Mostre a thread **criadora** × **executora**. Retire o `single` e explique o que ocorre. Depois investigue o comportamento de `i` nas tarefas, use `firstprivate(i)` e explique; e analise `x = 100; #pragma omp task firstprivate(x) { x++; ... }` imprimindo `x` depois.

### O que você precisa saber
- **`task`** cria uma unidade de trabalho que pode rodar **depois**, em **qualquer** thread do grupo. Quem criou **não espera** (continua o laço).
- Num `parallel`, **todas** as threads executam o bloco. Sem controle, **todas** criariam as tarefas (duplicadas). Padrão: `parallel` + `single`: uma thread só **produz** tarefas; as outras ficam livres para **consumi-las** (ao chegarem à barreira final do `single`, as threads ociosas executam as tarefas pendentes).
- **Dados em tarefas:** uma variável é, por padrão, **`firstprivate`** na tarefa se *não for shared* no contexto envolvente (por exemplo, uma variável local declarada **dentro** da região paralela); se é `shared` na região (declarada **fora** dela), continua **`shared`** na tarefa. Como a tarefa roda *depois*, ler uma variável `shared` que muda (como o `i` do laço) é um **erro clássico**.
- `firstprivate(x)` na tarefa tira uma **foto** do valor de `x` **no instante da criação**.
- Ao fim do `parallel` há barreira, e todas as tarefas pendentes terminam antes. Para esperar tarefas **antes** disso, use `taskwait`.

### Solução comentada

```cpp
// ex15.cpp
#include <cstdio>
#include <omp.h>

int main() {
    printf("=== criadora x executora ===\n");
    #pragma omp parallel
    {
        #pragma omp single                       // só UMA thread produz tarefas
        {
            const int criadora = omp_get_thread_num();    // local ao single => firstprivate na task
            for (int i = 0; i < 12; i++) {
                #pragma omp task firstprivate(i, criadora)   // explícito: foto de i no momento da criação
                {
                    printf("tarefa %2d  criada por %d  executada por %d\n",
                           i, criadora, omp_get_thread_num());
                }
            }
        }   // fim do single; as demais threads (e esta) executam as tarefas pendentes
    }

    printf("\n=== sem single: tarefas DUPLICADAS ===\n");
    int total = 0;
    #pragma omp parallel
    {
        for (int i = 0; i < 12; i++) {
            #pragma omp task
            {
                #pragma omp atomic
                ++total;
            }
        }
    }
    printf("tarefas executadas = %d (esperado 12 se não duplicasse; com T threads => 12*T)\n", total);

    printf("\n=== armadilha: i declarado FORA => shared na task ===\n");
    int j;                                          // fora da região => shared
    #pragma omp parallel
    #pragma omp single
    for (j = 0; j < 10; j++) {
        #pragma omp task                            // j é shared: a task lê o valor no instante em que RODA
        printf("j = %d\n", j);                      // pode imprimir valores repetidos/errados (e 10)
    }

    printf("\n=== firstprivate em tarefa ===\n");
    int x = 100;
    #pragma omp parallel
    #pragma omp single
    {
        for (int k = 0; k < 3; k++) {
            #pragma omp task firstprivate(x)        // cada tarefa recebe cópia (100)
            {
                x++;
                printf("dentro da task: x = %d\n", x);      // sempre 101
            }
        }
    }
    printf("x externo = %d\n", x);                   // continua 100
    return 0;
}
```

### Para fixar
- **a)** Não: a tarefa pode ser executada por qualquer thread do grupo (inclusive a criadora).
- **b)** Não é previsível (depende do escalonador).
- **c)** `single` garante que o laço de criação rode **uma vez**; sem ele, cada thread cria as 12 tarefas → **12 × nº de threads** tarefas.
- **`firstprivate` em tarefa:** copia o valor da variável **no momento da criação**; alterações dentro da tarefa afetam só a cópia. O `x` externo continua 100.

---

## Exercício 16 — Tarefas recursivas: Fibonacci

### Enunciado
Paralelize
```cpp
long fib(int n) { if (n < 2) return n; return fib(n-1) + fib(n-2); }
```
com tarefas (`task shared(x) firstprivate(n)`, `taskwait`), chamando-a dentro de `parallel` + `single`. Explique o papel de `task`, `taskwait`, `single`, `firstprivate`, `shared`. Depois evite criar tarefas para `n` pequeno (`if (n < LIMITE) return fib_sequencial(n);`), teste valores de `LIMITE` e explique por que tarefas pequenas demais pioram o desempenho.

### O que você precisa saber
- Divisão e conquista: cada chamada gera **duas subchamadas independentes** → ideais para tarefas.
- `x` e `y` (resultados parciais) vivem na **pilha** da chamada pai. As tarefas filhas escrevem nelas → precisam ser **`shared`**.
- **`taskwait`** é obrigatório: a chamada pai só pode somar `x + y` (e retornar, destruindo sua pilha!) depois que as filhas terminarem.
- `firstprivate(n)`: `n` é local; garante que cada tarefa leve o **seu** `n`, mesmo se a função pai mudar/terminar.
- **Granularidade:** criar, enfileirar e escalonar uma tarefa custa (centenas de ns a µs). `fib(2)` leva ~ns → o overhead supera o trabalho em ordens de grandeza. A solução é um **limiar (cut-off)**: abaixo dele, execute sequencialmente.
- Fibonacci é um exemplo didático, **não** um bom algoritmo (existe o iterativo O(n) e o fib(n) exponencial é só um *benchmark* de recursão).

### Solução comentada

```cpp
// ex16.cpp  —  ./ex16 42
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <omp.h>

long fib_seq(int n) { return n < 2 ? n : fib_seq(n - 1) + fib_seq(n - 2); }

long fib_task(int n, int limite) {
    if (n < 2) return n;
    if (n < limite) return fib_seq(n);       // CUT-OFF: problema pequeno => não vale criar tarefa

    long x, y;                                // pilha desta chamada
    #pragma omp task shared(x) firstprivate(n, limite)   // x é escrita pela tarefa => shared
    x = fib_task(n - 1, limite);

    #pragma omp task shared(y) firstprivate(n, limite)
    y = fib_task(n - 2, limite);

    #pragma omp taskwait                      // espera as DUAS filhas antes de usar x e y
    return x + y;
}

int main(int argc, char** argv) {
    const int n = (argc > 1) ? atoi(argv[1]) : 40;

    double t0 = omp_get_wtime();
    long ref = fib_seq(n);
    double ts = omp_get_wtime() - t0;
    printf("fib(%d) = %ld  sequencial: %.3f s\n", n, ref, ts);

    for (int limite : {2, 10, 15, 20, 25, 30}) {
        long r = 0;
        t0 = omp_get_wtime();
        #pragma omp parallel
        {
            #pragma omp single                // UMA thread inicia a recursão; as demais "roubam" tarefas
            r = fib_task(n, limite);
        }
        double tp = omp_get_wtime() - t0;
        printf("limite=%2d  fib=%ld  %s  tempo=%.3f s  speedup=%.2f\n",
               limite, r, r == ref ? "ok" : "ERRADO", tp, ts / tp);
    }
    return 0;
}
```

### Para fixar
- **`task`:** transforma a chamada em unidade de trabalho assíncrona. **`taskwait`:** sincroniza a pai com as filhas (necessário antes de usar `x`,`y`). **`single`:** garante uma única chamada raiz. **`firstprivate(n)`:** cada tarefa guarda seu `n`. **`shared(x)`:** a tarefa precisa escrever na variável da pai.
- Com `limite=2` (tarefa para quase tudo) o programa paralelo costuma ser **mais lento que o sequencial** (milhões de tarefas minúsculas, filas disputadas, memória).
- Na prática, um `LIMITE` entre 20 e 30 (para `n≈40`) costuma dar o melhor speedup. Procure o "joelho" na sua tabela.
- **Resumo:** tarefas pequenas demais → **overhead de criação/escalonamento > trabalho útil**.

---

## Exercício 17 — Merge Sort com tarefas

### Enunciado
Implemente o Merge Sort sequencial e depois transforme as duas chamadas recursivas em tarefas; `#pragma omp taskwait` antes do `merge`. A chamada inicial vai dentro de `parallel` + `single`. Introduza um limiar: `#pragma omp task if(tamanho > LIMIAR)`. Determine empiricamente um bom limiar e mostre por que criar tarefa para cada chamada pequena piora o desempenho.

### O que você precisa saber
- Merge Sort: ordena a metade esquerda, ordena a direita, **intercala** (merge). As duas metades são **independentes** → tarefas. O `merge` depende das duas → `taskwait`.
- Os dados (`a`, `tmp`) são **ponteiros**: passá-los por valor significa que a tarefa recebe uma cópia do ponteiro (firstprivate), mas os dados apontados são **compartilhados**. Duas tarefas diferentes escrevem em **trechos disjuntos** → sem corrida.
- `if(cond)` em `task`: quando falsa, a tarefa é executada **imediatamente pela thread atual** (tarefa "não adiada"): não paraleliza, mas ainda tem algum overhead de contabilidade. Mais barato ainda é chamar diretamente a versão sequencial abaixo do limiar (veja a versão `ms_par2`).
- O `merge` final é **sequencial** (O(N)): limita o speedup (Lei de Amdahl). Merge paralelo existe mas é bem mais complexo.

### Solução comentada

```cpp
// ex17.cpp
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <random>
#include <algorithm>
#include <omp.h>

// Intercala a[lo,mid) e a[mid,hi) usando 'tmp' como área auxiliar
void merge(int* a, int* tmp, long lo, long mid, long hi) {
    long i = lo, j = mid, k = lo;
    while (i < mid && j < hi) tmp[k++] = (a[i] <= a[j]) ? a[i++] : a[j++];
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi)  tmp[k++] = a[j++];
    for (k = lo; k < hi; ++k) a[k] = tmp[k];
}

void ms_seq(int* a, int* tmp, long lo, long hi) {
    if (hi - lo < 2) return;
    long mid = lo + (hi - lo) / 2;
    ms_seq(a, tmp, lo, mid);
    ms_seq(a, tmp, mid, hi);
    merge(a, tmp, lo, mid, hi);
}

long LIMIAR = 10000;

// Versão pedida no enunciado: task if(tamanho > LIMIAR)
void ms_par(int* a, int* tmp, long lo, long hi) {
    if (hi - lo < 2) return;
    long mid = lo + (hi - lo) / 2;
    const bool grande = (hi - lo) > LIMIAR;

    #pragma omp task if(grande) firstprivate(a, tmp, lo, mid)
    ms_par(a, tmp, lo, mid);

    #pragma omp task if(grande) firstprivate(a, tmp, mid, hi)
    ms_par(a, tmp, mid, hi);

    #pragma omp taskwait                       // o merge só pode ocorrer com as duas metades ordenadas
    merge(a, tmp, lo, mid, hi);
}

// Versão alternativa (geralmente melhor): abaixo do limiar, nem cria tarefa — chama a versão sequencial
void ms_par2(int* a, int* tmp, long lo, long hi) {
    if (hi - lo <= LIMIAR) { ms_seq(a, tmp, lo, hi); return; }
    long mid = lo + (hi - lo) / 2;
    #pragma omp task firstprivate(a, tmp, lo, mid)
    ms_par2(a, tmp, lo, mid);
    #pragma omp task firstprivate(a, tmp, mid, hi)
    ms_par2(a, tmp, mid, hi);
    #pragma omp taskwait
    merge(a, tmp, lo, mid, hi);
}

int main(int argc, char** argv) {
    const long N = (argc > 1) ? atol(argv[1]) : 10'000'000;
    std::vector<int> orig(N);
    std::mt19937 gen(3);
    for (auto& x : orig) x = (int)gen();

    std::vector<int> a = orig, tmp(N);
    double t0 = omp_get_wtime();
    ms_seq(a.data(), tmp.data(), 0, N);
    double ts = omp_get_wtime() - t0;
    printf("sequencial: %.3f s  ordenado? %s\n", ts, std::is_sorted(a.begin(), a.end()) ? "sim" : "NAO");
    std::vector<int> ref = a;

    for (long lim : {1L, 16L, 256L, 1024L, 4096L, 16384L, 65536L, 262144L, 1048576L}) {
        LIMIAR = lim;
        a = orig;
        t0 = omp_get_wtime();
        #pragma omp parallel
        {
            #pragma omp single                 // uma thread dispara a recursão; todas executam tarefas
            ms_par(a.data(), tmp.data(), 0, N);
        }
        double tp = omp_get_wtime() - t0;
        printf("LIMIAR=%8ld  tempo=%.3f s  speedup=%.2f  %s\n", lim, tp, ts / tp, a == ref ? "ok" : "ERRADO");
    }
    return 0;
}
```
(Troque `ms_par` por `ms_par2` para comparar as duas formas de aplicar o limiar.)

### Para fixar
- Limiar muito pequeno (1–256): milhões de tarefas minúsculas → **mais lento até que o sequencial**.
- Limiar muito grande: poucas tarefas → threads ociosas (pouco paralelismo).
- O ponto bom costuma estar entre ~10⁴ e ~10⁵ elementos (depende da máquina). Uma regra prática: ter **~10–50 tarefas por thread**.
- Mesmo no melhor caso, o speedup fica abaixo de `nT` devido ao `merge` sequencial do topo da recursão e à largura de banda de memória.

---

## Exercício 18 — Grafo de dependências com `task depend`

### Enunciado
Uma matriz dividida em blocos; cada bloco passa por: **carregar → processar → salvar**. O processamento de um bloco pode começar assim que **sua** carga terminar; a gravação, assim que **seu** processamento terminar; blocos diferentes podem estar em fases diferentes simultaneamente. Implemente com `depend(out:...)` e `depend(in:...)`, **sem `taskwait` global** entre as fases. Simule cada operação com uma função de custo variável. **Conceitual:** compare "todos carregam → todos processam → todos salvam" (barreiras) com a versão com dependências.

### O que você precisa saber
- `depend(out: x)`: a tarefa **produz** `x`; `depend(in: x)`: a tarefa **consome** `x`. O runtime garante: uma tarefa `in` só começa **após** as tarefas `out` anteriores (na ordem de criação) sobre o mesmo item.
- Os itens de dependência são só **"rótulos"** (endereços): use um vetor de `char` como tokens — um por bloco e por fase. O conteúdo é irrelevante.
- As tarefas devem ser criadas **em ordem lógica** (a produtora antes da consumidora), por uma única thread (`single`): as dependências são calculadas na ordem de criação.
- Resultado: um **DAG** (grafo acíclico dirigido). Sem barreiras globais, o bloco 3 pode estar salvando enquanto o bloco 7 ainda carrega.
- Itens `depend` devem ser *lvalues*: `tok[b]` com array/ponteiro cru funciona bem.

### Solução comentada

```cpp
// ex18.cpp
#include <cstdio>
#include <algorithm>
#include <omp.h>

constexpr int NB = 12;                 // nº de blocos
double T0;                             // instante inicial, para os logs

// Custo variável (segundos) por bloco e fase: 10 a 50 ms
double custo(int b, int fase) { return 0.010 * (1 + (b * 7 + fase * 3) % 5); }

// Simula trabalho ocupando a CPU durante 's' segundos
void espera(double s) { double t = omp_get_wtime(); while (omp_get_wtime() - t < s) {} }

void etapa(const char* nome, int fase, int b) {
    // Em uma versão real, aqui entraria a leitura/processamento/escrita do bloco b da matriz
    espera(custo(b, fase));
}

int main() {
    static char t_carga[NB], t_proc[NB];       // tokens de dependência (um por bloco)

    // ---------- Versão 1: com barreiras entre fases ----------
    T0 = omp_get_wtime();
    #pragma omp parallel
    #pragma omp single
    {
        for (int b = 0; b < NB; ++b) {
            #pragma omp task firstprivate(b)
            etapa("carregar", 0, b);
        }
        #pragma omp taskwait                   // BARREIRA: todos carregam antes de qualquer processamento

        for (int b = 0; b < NB; ++b) {
            #pragma omp task firstprivate(b)
            etapa("processar", 1, b);
        }
        #pragma omp taskwait                   // BARREIRA

        for (int b = 0; b < NB; ++b) {
            #pragma omp task firstprivate(b)
            etapa("salvar", 2, b);
        }
        #pragma omp taskwait
    }
    double t_barreiras = omp_get_wtime() - T0;

    // ---------- Versão 2: com dependências (DAG) ----------
    T0 = omp_get_wtime();
    #pragma omp parallel
    #pragma omp single
    {
        for (int b = 0; b < NB; ++b) {
            // carregar(b) PRODUZ t_carga[b]
            #pragma omp task firstprivate(b) depend(out : t_carga[b])
            etapa("carregar", 0, b);

            // processar(b) CONSOME t_carga[b] e PRODUZ t_proc[b]
            #pragma omp task firstprivate(b) depend(in : t_carga[b]) depend(out : t_proc[b])
            etapa("processar", 1, b);

            // salvar(b) CONSOME t_proc[b]
            #pragma omp task firstprivate(b) depend(in : t_proc[b])
            etapa("salvar", 2, b);
        }
        // Sem taskwait: a barreira ao fim da região paralela já espera todas as tarefas.
    }
    double t_deps = omp_get_wtime() - T0;

    double seq = 0;
    for (int b = 0; b < NB; ++b) for (int f = 0; f < 3; ++f) seq += custo(b, f);
    printf("threads=%d\n", omp_get_max_threads());
    printf("tempo sequencial teórico : %.3f s\n", seq);
    printf("com barreiras            : %.3f s\n", t_barreiras);
    printf("com depend (DAG)         : %.3f s\n", t_deps);
    return 0;
}
```

### Para fixar
- **Com barreiras:** cada fase só termina quando o bloco **mais lento** termina; threads que acabaram ficam ociosas esperando. Tempo ≈ soma, por fase, do "máximo/balanceamento" da fase.
- **Com dependências:** o bloco que acabou de carregar já pode ser processado enquanto outros ainda carregam; o runtime mantém as threads ocupadas. Tempo menor quando os custos são **desiguais**.
- Dependências expressam **apenas as relações reais** (carregar(b) → processar(b) → salvar(b)); barreiras impõem relações **extras** que não existem (processar(1) não precisa esperar carregar(7)).
- Para uma versão real, substitua `etapa` por: ler um bloco de um arquivo/gerar dados (carregar), aplicar uma transformação (processar), escrever num vetor de saída (salvar).

---

# Nível 6 — Integração

## Exercício 19 — Problema integrador

### Enunciado
Dado um vetor de N inteiros, determine: **soma**, **maior elemento**, **quantidade de elementos pares**, e **um valor produzido pela última iteração** (ordem sequencial).
- **Versão 1:** `parallel`, `for`, `private/firstprivate`, `reduction`, `lastprivate`, `atomic/critical` quando adequados — justificando.
- **Versão 2:** divida o vetor em **blocos** e processe cada bloco com `#pragma omp task`; indique para cada variável se é `shared`, `private` ou `firstprivate`, e onde a sincronização é necessária.

### O que você precisa saber
- Revise: `reduction` (soma, pares, máximo), `lastprivate` (valor da última iteração), `atomic` (atualização simples), `critical` (bloco com várias instruções).
- **Versão 1:** o ideal é *só* `reduction` + `lastprivate`; `atomic/critical` seriam alternativas piores (e aparecem apenas como comparação).
- **Versão 2 (blocos):** cada tarefa acumula em **variáveis locais** (declaradas dentro da tarefa → privadas) e, ao terminar, **funde** seu resultado nas variáveis globais (`shared`) com `atomic` (soma, pares) e `critical` (máximo; "se maior que, então atribui" são dois passos). Uma sincronização por tarefa, não por elemento.
- O "valor da última iteração" é produzido apenas pela tarefa do **último** bloco → sem corrida.
- Para ler os resultados **dentro** do `single`, precisa de `taskwait`; ao sair do `parallel` a barreira final já garante que tudo terminou.

### Solução comentada

```cpp
// ex19.cpp
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <vector>
#include <random>
#include <algorithm>
#include <omp.h>

// "valor produzido na iteração i" (determinístico, depende do elemento e da posição)
inline int produz(long i, int x) { return x - (int)(i % 1000); }

int main(int argc, char** argv) {
    const long N = (argc > 1) ? atol(argv[1]) : 20'000'000;
    std::vector<int> v(N);
    std::mt19937 gen(5);
    for (auto& x : v) x = (int)(gen() % 1'000'000);

    // ---------- Referência sequencial ----------
    long long soma0 = 0; long pares0 = 0; int maior0 = INT_MIN, ultimo0 = 0;
    for (long i = 0; i < N; ++i) {
        int x = v[i];
        soma0 += x; if (x % 2 == 0) pares0++; if (x > maior0) maior0 = x;
        ultimo0 = produz(i, x);
    }

    // ---------- VERSÃO 1: for + reduction + lastprivate ----------
    long long soma1 = 0; long pares1 = 0; int maior1 = INT_MIN, ultimo1 = 0;
    double t0 = omp_get_wtime();
    #pragma omp parallel for reduction(+ : soma1, pares1) reduction(max : maior1) lastprivate(ultimo1)
    for (long i = 0; i < N; ++i) {
        int x = v[i];                       // declarada dentro => privada
        soma1 += x;                         // reduction: sem corrida, sem sincronização por elemento
        if (x % 2 == 0) pares1++;           // reduction
        if (x > maior1) maior1 = x;         // reduction(max)
        ultimo1 = produz(i, x);             // lastprivate: a original recebe o valor da iteração N-1
    }
    double t1 = omp_get_wtime() - t0;

    // ---------- VERSÃO 2: blocos + tarefas ----------
    const long B = 200'000;                 // tamanho do bloco (granularidade das tarefas)
    long long soma2 = 0; long pares2 = 0; int maior2 = INT_MIN, ultimo2 = 0;
    t0 = omp_get_wtime();
    #pragma omp parallel
    #pragma omp single
    {
        for (long ini = 0; ini < N; ini += B) {
            const long fim = std::min(N, ini + B);

            // v, soma2, pares2, maior2, ultimo2: shared (declaradas fora da região)
            // ini, fim: firstprivate (valores deste bloco, "fotografados" na criação)
            #pragma omp task firstprivate(ini, fim) shared(v, soma2, pares2, maior2, ultimo2)
            {
                long long s = 0; long p = 0; int m = INT_MIN;   // locais da tarefa => privadas
                for (long i = ini; i < fim; ++i) {
                    int x = v[i];
                    s += x; if (x % 2 == 0) p++; if (x > m) m = x;
                }

                #pragma omp atomic                 // atualização simples de uma variável compartilhada
                soma2 += s;
                #pragma omp atomic
                pares2 += p;
                #pragma omp critical(maior)        // "comparar e atribuir" são 2 passos => critical
                { if (m > maior2) maior2 = m; }

                if (fim == N)                      // só a tarefa do ÚLTIMO bloco escreve => sem corrida
                    ultimo2 = produz(N - 1, v[N - 1]);
            }
        }
        // Se fôssemos usar soma2/pares2/... AQUI dentro do single, seria preciso:
        // #pragma omp taskwait
    }   // barreira ao fim do parallel: todas as tarefas terminaram
    double t2 = omp_get_wtime() - t0;

    printf("seq  : soma=%lld pares=%ld maior=%d ultimo=%d\n", soma0, pares0, maior0, ultimo0);
    printf("v1   : soma=%lld pares=%ld maior=%d ultimo=%d  %.4f s %s\n", soma1, pares1, maior1, ultimo1, t1,
           (soma1 == soma0 && pares1 == pares0 && maior1 == maior0 && ultimo1 == ultimo0) ? "ok" : "ERRO");
    printf("v2   : soma=%lld pares=%ld maior=%d ultimo=%d  %.4f s %s\n", soma2, pares2, maior2, ultimo2, t2,
           (soma2 == soma0 && pares2 == pares0 && maior2 == maior0 && ultimo2 == ultimo0) ? "ok" : "ERRO");
    return 0;
}
```

### Para fixar
- **Justificativas V1:** `reduction` para agregados; `lastprivate` para o "último valor"; variáveis temporárias declaradas no corpo (privadas); `atomic/critical` evitados por serem mais lentos que `reduction`.
- **Classificação V2:**

| Variável | Tipo | Motivo |
|---|---|---|
| `v` | shared | só leitura, grande demais para copiar |
| `ini`, `fim` | firstprivate | variam por tarefa; precisam ser "fotografadas" na criação |
| `s`, `p`, `m`, `x`, `i` | private (locais da tarefa) | acumuladores locais, sem disputa |
| `soma2`, `pares2` | shared + `atomic` | resultado global, atualização simples |
| `maior2` | shared + `critical` | comparar e atribuir |
| `ultimo2` | shared | escrito por **uma** só tarefa |

- **Sincronização necessária:** `atomic`/`critical` ao fundir os parciais; `taskwait` (ou o fim do `parallel`) antes de ler os resultados.

---

## Exercício 20 — Miniaplicação: processamento paralelo de imagem

### Enunciado
Receba uma imagem **PGM** grande e execute: (1) **suavização** por média de vizinhança; (2) cálculo do **gradiente**; (3) **maior gradiente e sua coordenada**. Use: decomposição em regiões paralelas, escolha de escalonamento, redução para estatísticas, sincronização apenas onde necessária. **Extensão:** dividir a imagem em blocos e processar como **tarefas**. Compare resultado e desempenho com a versão sequencial. **Produza uma tabela** de tempo, speedup e eficiência para vários nº de threads e **justifique as decisões**.

### O que você precisa saber
- **Formato PGM binário (P5):** cabeçalho de texto (`P5`, largura, altura, valor máximo; comentários começam com `#`) seguido de `largura×altura` bytes (um por pixel).
- **Dependência entre etapas:** o gradiente de um pixel usa seus **vizinhos** já suavizados. Logo: **todas** as linhas da suavização (ou ao menos as vizinhas) devem terminar antes do gradiente → **barreira** entre as etapas (a barreira implícita do `omp for` serve). Dentro de cada etapa os pixels são independentes.
- **Paralelizar por linhas** (laço externo): grão grande o bastante e acesso contíguo à memória (bom para o cache).
- **Escalonamento:** o custo por pixel é praticamente uniforme → `static` (overhead ≈ 0). `dynamic` só se compensa em imagens irregulares.
- **Maior gradiente + posição** = redução `maxloc` do Exercício 14 (desempate pelo menor índice linear, garantindo resultado idêntico ao sequencial).
- **Uma só região paralela** contendo as duas etapas: evita criar/destruir threads duas vezes.
- **Extensão com tarefas:** blocos de linhas ("faixas"); a tarefa de gradiente da faixa `b` depende das suavizações das faixas `b-1`, `b` e `b+1` (precisa das linhas de fronteira) → `depend(in: tok[b-1], tok[b], tok[b+1])`. Assim o gradiente de uma faixa pode começar sem esperar a imagem inteira.
- Sem uma imagem PGM à mão? O programa **gera uma imagem sintética** quando não recebe argumento (e `convert foto.jpg -colorspace Gray foto.pgm` do ImageMagick cria PGMs; atenção ao formato P5).

### Solução comentada

```cpp
// ex20.cpp  —  g++ -fopenmp -O2 -std=c++17 ex20.cpp -o ex20
//   ./ex20                (imagem sintética 4096x4096)
//   ./ex20 foto.pgm       (PGM P5)
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <climits>
#include <limits>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <omp.h>

// -------------------- Tipo + redução maxloc (Ex. 14) --------------------
struct MaxLoc { double valor; long indice; };
inline MaxLoc combina(const MaxLoc& a, const MaxLoc& b) {
    if (a.valor > b.valor) return a;
    if (b.valor > a.valor) return b;
    return a.indice < b.indice ? a : b;
}
inline MaxLoc neutro() { return MaxLoc{ -std::numeric_limits<double>::infinity(), LONG_MAX }; }
#pragma omp declare reduction(maxloc : MaxLoc : omp_out = combina(omp_out, omp_in)) \
                    initializer(omp_priv = neutro())

// -------------------- Imagem e leitura PGM --------------------
struct Imagem { int w = 0, h = 0; std::vector<unsigned char> px; };

static bool le_int(FILE* f, int& v) {              // lê inteiro pulando espaços e comentários '#'
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '#') { while ((c = fgetc(f)) != '\n' && c != EOF) {} }
        else if (!isspace(c)) { ungetc(c, f); break; }
    }
    return fscanf(f, "%d", &v) == 1;
}

bool ler_pgm(const char* nome, Imagem& im) {
    FILE* f = fopen(nome, "rb");
    if (!f) return false;
    char magic[3] = {0};
    int maxv = 0;
    bool ok = fscanf(f, "%2s", magic) == 1 && std::string(magic) == "P5"
              && le_int(f, im.w) && le_int(f, im.h) && le_int(f, maxv);
    if (ok) {
        fgetc(f);                                   // consome o único espaço após o maxval
        im.px.resize((size_t)im.w * im.h);
        ok = fread(im.px.data(), 1, im.px.size(), f) == im.px.size();
    }
    fclose(f);
    return ok;
}

Imagem imagem_sintetica(int w, int h) {
    Imagem im; im.w = w; im.h = h; im.px.resize((size_t)w * h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            int v = (x * 7 + y * 13 + (x * y) % 251) & 255;           // padrão ruidoso
            if (x > w / 3 && x < w / 2 && y > h / 3 && y < h / 2) v = 255;   // um quadrado claro
            im.px[(size_t)y * w + x] = (unsigned char)v;
        }
    return im;
}

// -------------------- Núcleos que operam sobre um intervalo de linhas [y0,y1) --------------------
// Reutilizados pelas versões sequencial, omp for e tarefas => mesmo cálculo, mesmo resultado.

// Média da vizinhança 3x3 (nas bordas, só os vizinhos existentes)
void suaviza_linhas(const Imagem& im, std::vector<float>& out, int y0, int y1) {
    const int w = im.w, h = im.h;
    for (int y = y0; y < y1; ++y)
        for (int x = 0; x < w; ++x) {
            int soma = 0, cnt = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                int yy = y + dy; if (yy < 0 || yy >= h) continue;
                for (int dx = -1; dx <= 1; ++dx) {
                    int xx = x + dx; if (xx < 0 || xx >= w) continue;
                    soma += im.px[(size_t)yy * w + xx]; ++cnt;
                }
            }
            out[(size_t)y * w + x] = (float)soma / cnt;
        }
}

// Gradiente por diferenças centrais; atualiza o maior gradiente em 'best'
void gradiente_linhas(const std::vector<float>& b, std::vector<float>& g, int w, int h,
                      int y0, int y1, MaxLoc& best) {
    for (int y = y0; y < y1; ++y)
        for (int x = 0; x < w; ++x) {
            size_t idx = (size_t)y * w + x;
            if (x == 0 || y == 0 || x == w - 1 || y == h - 1) { g[idx] = 0; continue; }
            float gx = b[idx + 1] - b[idx - 1];
            float gy = b[idx + w] - b[idx - w];
            float m  = std::sqrt(gx * gx + gy * gy);
            g[idx] = m;
            best = combina(best, MaxLoc{ m, (long)idx });
        }
}

// -------------------- Versão sequencial --------------------
MaxLoc proc_seq(const Imagem& im, std::vector<float>& blur, std::vector<float>& grad) {
    MaxLoc best = neutro();
    suaviza_linhas(im, blur, 0, im.h);
    gradiente_linhas(blur, grad, im.w, im.h, 0, im.h, best);
    return best;
}

// -------------------- Versão paralela: omp for por linhas --------------------
MaxLoc proc_for(const Imagem& im, std::vector<float>& blur, std::vector<float>& grad) {
    MaxLoc best = neutro();
    const int h = im.h, w = im.w;
    #pragma omp parallel                       // UMA região paralela para as duas etapas
    {
        #pragma omp for schedule(static)       // custo uniforme por linha => static
        for (int y = 0; y < h; ++y) suaviza_linhas(im, blur, y, y + 1);
        // barreira implícita aqui: o gradiente precisa dos vizinhos (de outras threads) já suavizados

        #pragma omp for schedule(static) reduction(maxloc : best)
        for (int y = 0; y < h; ++y) gradiente_linhas(blur, grad, w, h, y, y + 1, best);
    }
    return best;
}

// -------------------- Extensão: blocos (faixas de linhas) como tarefas --------------------
MaxLoc proc_tasks(const Imagem& im, std::vector<float>& blur, std::vector<float>& grad, int BH) {
    MaxLoc best = neutro();
    const int h = im.h, w = im.w;
    const int nb = (h + BH - 1) / BH;
    std::unique_ptr<char[]> tokmem(new char[nb]());
    char* tok = tokmem.get();                  // tokens de dependência: um por faixa

    #pragma omp parallel
    #pragma omp single
    {
        for (int b = 0; b < nb; ++b) {         // 1) suavização da faixa b PRODUZ tok[b]
            const int y0 = b * BH, y1 = std::min(h, y0 + BH);
            #pragma omp task firstprivate(y0, y1) depend(out : tok[b])
            suaviza_linhas(im, blur, y0, y1);
        }
        for (int b = 0; b < nb; ++b) {         // 2) gradiente da faixa b CONSOME as faixas b-1, b, b+1
            const int y0 = b * BH, y1 = std::min(h, y0 + BH);
            const int prev = std::max(b - 1, 0), next = std::min(b + 1, nb - 1);
            #pragma omp task firstprivate(y0, y1) depend(in : tok[prev], tok[b], tok[next])
            {
                MaxLoc local = neutro();                       // melhor local da faixa (privado)
                gradiente_linhas(blur, grad, w, h, y0, y1, local);
                #pragma omp critical(melhor)                   // 1 seção crítica por faixa, não por pixel
                best = combina(best, local);
            }
        }
    }
    return best;
}

// -------------------- main --------------------
static double max_dif(const std::vector<float>& a, const std::vector<float>& b) {
    double d = 0; for (size_t i = 0; i < a.size(); ++i) d = std::max(d, (double)std::fabs(a[i] - b[i])); return d;
}

int main(int argc, char** argv) {
    Imagem im;
    if (argc > 1) { if (!ler_pgm(argv[1], im)) { fprintf(stderr, "erro lendo %s\n", argv[1]); return 1; } }
    else im = imagem_sintetica(4096, 4096);
    const size_t n = (size_t)im.w * im.h;
    printf("imagem %dx%d, threads = %d\n", im.w, im.h, omp_get_max_threads());

    std::vector<float> bs(n), gs(n), bp(n), gp(n), bt(n), gt(n);

    // Execuções repetidas: pega o menor tempo
    auto melhor = [](auto&& f) { double m = 1e30; for (int r = 0; r < 3; ++r) { double t = omp_get_wtime(); f(); m = std::min(m, omp_get_wtime() - t); } return m; };

    MaxLoc rs, rp, rt;
    double ts = melhor([&] { rs = proc_seq  (im, bs, gs); });
    double tp = melhor([&] { rp = proc_for  (im, bp, gp); });
    double tt = melhor([&] { rt = proc_tasks(im, bt, gt, 64); });

    printf("%-12s %8s %9s %10s   resultado\n", "versão", "tempo(s)", "speedup", "eficiência");
    printf("%-12s %8.4f %9.2f %10.2f   max=%.3f em (%ld,%ld)\n", "sequencial", ts, 1.0, 1.0,
           rs.valor, rs.indice % im.w, rs.indice / im.w);
    printf("%-12s %8.4f %9.2f %10.2f   %s\n", "omp for", tp, ts / tp, ts / tp / omp_get_max_threads(),
           (rp.indice == rs.indice && rp.valor == rs.valor && max_dif(bs, bp) == 0 && max_dif(gs, gp) == 0) ? "igual ao seq." : "DIFERENTE!");
    printf("%-12s %8.4f %9.2f %10.2f   %s\n", "tasks", tt, ts / tt, ts / tt / omp_get_max_threads(),
           (rt.indice == rs.indice && rt.valor == rs.valor && max_dif(bs, bt) == 0 && max_dif(gs, gt) == 0) ? "igual ao seq." : "DIFERENTE!");
    return 0;
}
```

Gerar a tabela para vários nº de threads:

```bash
g++ -fopenmp -O2 -std=c++17 ex20.cpp -o ex20
for t in 1 2 4 8; do echo "### OMP_NUM_THREADS=$t"; OMP_NUM_THREADS=$t ./ex20; done
```

Monte, a partir da saída, uma tabela como:

| Threads | T (omp for) | Speedup | Eficiência | T (tasks) | Speedup | Eficiência |
|---|---|---|---|---|---|---|
| 1 | | | | | | |
| 2 | | | | | | |
| 4 | | | | | | |
| 8 | | | | | | |

### Para fixar (decisões de projeto que você deve justificar no relatório)
1. **Paralelismo por linhas:** o laço externo tem grão suficiente e as linhas são contíguas na memória (boa localidade).
2. **`schedule(static)`:** custo por pixel uniforme → sem necessidade de balanceamento dinâmico.
3. **Uma só região paralela:** evita recriar threads entre as etapas; a barreira implícita do primeiro `omp for` é a única sincronização **necessária** (dependência de vizinhança).
4. **`reduction(maxloc)`:** evita seções críticas por pixel e garante resultado idêntico ao sequencial pelo desempate do menor índice.
5. **Tarefas por faixas com `depend`:** removem a barreira global entre suavização e gradiente; a faixa só espera as 3 faixas das quais realmente depende. O ganho é pequeno aqui (a imagem é uniforme), mas cresce se houver outras etapas ou cargas irregulares.
6. **Esperado:** speedup abaixo do ideal porque o processamento é limitado pela memória; eficiência cai com mais threads.

---

# Revisão — Questões de pensar e responder

**1. Qual a diferença fundamental entre `private` e `firstprivate`?**
Ambas criam uma cópia por thread; `private` **não inicializa** (valor indefinido), `firstprivate` **inicializa com o valor da variável original** no início da região. Em nenhuma das duas o valor volta para a original.

**2. Qual o significado de `lastprivate`? Dê um exemplo útil.**
Cópia privada por thread, e ao final a variável original recebe o valor da cópia da thread que executou a **última iteração na ordem sequencial**. Útil para obter o "último valor processado" ou o valor final de uma variável de estado após um laço paralelo (ex.: último índice/valor calculado).

**3. Quando uma `reduction` é preferível a um `critical`?**
Sempre que o objetivo é **acumular** um resultado (soma, produto, mín, máx, contagem) sobre muitas iterações: `reduction` acumula em cópias privadas e sincroniza **uma vez por thread**; `critical` sincroniza a **cada iteração**.

**4. Diferença entre `atomic` e `critical`; quando usar cada um; exemplos.**
- `atomic`: protege **uma única atualização simples** de uma posição de memória (`x++`, `x += expr`), usando instruções atômicas do hardware — barato.
- `critical`: exclusão mútua para **um bloco arbitrário** de código, via lock — mais caro, mas mais geral.
- *Uso de `critical` que deveria ser `atomic`:*
  ```cpp
  #pragma omp critical
  contador++;          // deveria ser: #pragma omp atomic
  ```
- *Seção crítica inapropriada por desempenho:* colocar trabalho longo ou E/S dentro dela, por exemplo
  ```cpp
  #pragma omp critical
  { resultado = calculo_pesado(x);  printf("...\n"); }   // serializa todo o cálculo e a E/S
  ```
  A região crítica deve conter só o **mínimo** necessário (atualizar o dado compartilhado); calcule fora e proteja apenas a escrita.

**5. Diferença entre paralelismo de `omp for` e de `task`?**
`omp for` divide as iterações de um **laço de tamanho conhecido** entre as threads (paralelismo de dados, estrutura regular, distribuição por `schedule`). `task` cria **unidades de trabalho dinâmicas e irregulares** (recursão, listas, grafos de dependência), criadas durante a execução e escalonadas pelo runtime; permite sincronização fina (`taskwait`, `depend`).

**6. Função de `single` na criação de tarefas?**
Garante que **uma só thread** execute o bloco que cria as tarefas; sem ele, cada thread do grupo repetiria a criação (tarefas duplicadas). As demais threads ficam disponíveis para executar as tarefas criadas.

**7. Por que tarefas muito pequenas reduzem o desempenho?**
Criar, enfileirar, escalonar e finalizar uma tarefa tem custo fixo (memória, filas compartilhadas, sincronização). Se o trabalho da tarefa é menor que esse custo, o overhead domina. Solução: **aumentar a granularidade** (limiar/cut-off, `if`, blocos maiores).

---

# Cola rápida de diretivas e cláusulas

| Preciso de... | Use |
|---|---|
| Criar threads / replicar código | `#pragma omp parallel` |
| Dividir um laço entre threads | `#pragma omp for` (dentro de `parallel`) ou `parallel for` |
| Escolher distribuição das iterações | `schedule(static\|dynamic\|guided[,chunk])`, `schedule(runtime)` + `OMP_SCHEDULE` |
| Acumular soma/mín/máx | `reduction(+:x)`, `reduction(min:x)`, `reduction(max:x)` |
| Reduzir tipos próprios | `#pragma omp declare reduction(...)` |
| Cópia por thread, sem valor inicial | `private(x)` |
| Cópia por thread, com valor inicial | `firstprivate(x)` |
| Pegar o valor da última iteração | `lastprivate(x)` |
| Proteger uma atualização simples | `#pragma omp atomic` |
| Proteger bloco de código | `#pragma omp critical [(nome)]` |
| Esperar todas as threads | `#pragma omp barrier` |
| Só uma thread executa | `#pragma omp single` / `master` |
| Não esperar no fim do `for` | `nowait` |
| Criar tarefa | `#pragma omp task [shared(...) firstprivate(...) if(cond) depend(in\|out: x)]` |
| Esperar tarefas filhas | `#pragma omp taskwait` |
| Tempo | `omp_get_wtime()` |
| Nº de threads | `OMP_NUM_THREADS`, `omp_set_num_threads(n)` |

## Armadilhas mais comuns (checklist antes de rodar)

1. Esqueceu `-fopenmp`? As diretivas são ignoradas silenciosamente.
2. Variável escrita por várias threads e declarada fora da região? É `shared` → corrida. Use `reduction`, `private` ou sincronização.
3. Dependência entre iterações (`a[i]` usa `a[i-1]` ou `a[i+1]`)? `parallel for` direto está **errado**.
4. Comparou o resultado paralelo com o sequencial? Sempre confira.
5. Testou apenas com 1 thread? Corridas só aparecem com 2 ou mais.
6. Medindo tempo sem `-O2`, com tamanhos minúsculos, ou uma única vez? Os números não significam nada.
7. `critical`/`atomic` dentro do laço quente? Provavelmente você quer `reduction` ou acumulação local.
8. Criando tarefas minúsculas? Coloque um limiar.
9. Em tarefas, variável `shared` do laço (`i`) lida depois? Use `firstprivate`.
10. `printf` dentro do laço medido distorce o tempo: use-o só para visualizar a distribuição, não para medir.

## Roteiro de estudo sugerido

1. **Dia 1:** Ex. 1–3 (rodar, brincar com threads e `schedule`).
2. **Dia 2:** Ex. 4–7 (corridas, `reduction`, `atomic`/`critical`) — o coração de OpenMP.
3. **Dia 3:** Ex. 8–11 (histograma, estatísticas, escalonamento).
4. **Dia 4:** Ex. 12–14 (dependências, prefixos, redução própria).
5. **Dia 5:** Ex. 15–18 (tarefas).
6. **Dia 6:** Ex. 19–20 e as questões de revisão.

Em cada exercício: leia "O que você precisa saber" → feche o guia e implemente → compare com a solução → altere parâmetros e observe. Bons estudos!
