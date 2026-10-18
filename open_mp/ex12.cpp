#include <cstdio>
#include <cstdlib>
#include <omp.h>
#include <vector>
#include <iostream>

#define N 100

int main (int argc, char* argv[]) {
    std::vector<int> a_seq(N, 0), b_seq(N, 0); //vetores de N posições com 0
    std::vector<int> a(N, 0), b(N, 0); //vetores de N posições com 0
    int best_idx = 0, best_idx_seq;
    std::vector<int> original_b;
    
    // versões sequenciais pra gabarito
    for (int i = 1; i < N; i++) {
        a_seq[i] = a_seq[i-1] + 2;
    }
    for (int i = 0; i < N; i++) {
        b_seq[i] = b_seq[i+1] + 1;
    }
    for (int i = 0; i < N; i++) {
        if (a_seq[i] > 0) {
            best_idx_seq = i;
        }
    }

    // iterações de laços são paralelizáveis quando nenhumma a iteração atual 
    // depende do dado que foi produzido consumido pela outra iteração

    //--------------------------------------------------------------
    // RAW (read after write)
    // a atual precisa do que a anterior escreveu

    // como a operação é uma soma 2 no resultado anterior
    // a dependência pode ser resolvida simplesmente reescrevendo a operação
    // soma 2 na posição i=5 é igual a ter somado 2 na posição 0 i vezes
    # pragma omp parallel for
    for (int i = 1; i < N; i++) {
        a[i] = a[0] + i * 2;
    }
    std::cout << (a_seq == a ? "ok - caso1\n" : "ERRO RAW - CASO1\n");
    //--------------------------------------------------------------


    //--------------------------------------------------------------
    // WAR (write after read)
    // a atual precisa ler oq a próxima ainda vai escrever

    // dá pra resolver com uma cópia do vetor original.
    // já que cada iteração só depende do valor da próxima (inalterado)
    // e a incerteza surge de se ele foi alterado ou não
    original_b = b;
    # pragma omp parallel for
    for (int i = 0; i < N; i++) {
        b[i] = original_b[i+1] + 1;
    }
    std::cout << (b_seq == b ? "ok - caso2\n" : "ERRO WAR - CASO2\n");
    //--------------------------------------------------------------

    //--------------------------------------------------------------
    // WAW
    // várias iterações escrevendo no mesmo lugar

    // dá pra resolver transformando best_idx numa variável privada
    // e atribuir ao final da área paralela o máximo local de cada thread
    // via reduction(max: best_idx)
    # pragma omp parallel for reduction(max : best_idx)
    for (int i = 0; i < N; i++) {
        if (a[i] > 0) {
            best_idx = i;
        }
    }
    std::cout << (best_idx == best_idx ? "ok - caso3\n" : "ERRO WAW - CASO3\n");
    //--------------------------------------------------------------

    return 0;
}