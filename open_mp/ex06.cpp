#include <cstdio>
#include <cstdlib>
#include <omp.h>

int main(int argc, char* argv[]) {
    
    int ultimo = -1;

    # pragma omp parallel for
    for(int i = 0; i < 20; i++) {
        ultimo = i;
        printf("thread %d -> i = %d\n", omp_get_thread_num(), i);
    }

    // Dessa maneira o valor de último não é previsível, pois ela é shared, o que significa que tem condição de corrida entre as threads nela
    // vence a que escreve por último, sobreescrevendo os dados. O que varia a cada execução.
    printf("ultimo: %d\n\n", ultimo);


    ultimo = -1;
    int valor = 0;
    # pragma omp parallel for lastprivate(ultimo, valor)
    for (int i = 0; i < 20; i++) {
        ultimo = i;
        valor = 10 * i; // toda thread calcula, i(1, 2, 3, ...) * 10 com suas cópias privadas, porém só a última lógica 19 * 10, é atribuída na variável original
        printf("thread %d -> i = %d\n", omp_get_thread_num(), i);
    }
    // como último é lastprivate, cada thread recebe sua cópia privada dele, e ao final (no join), o último valor lógico é atribuído à variável original
    printf("ultimo: %d\n", ultimo); // é pra printar 19 nesse caso
    printf("valor: %d\n", valor); // teoricamente vai printar 190, pois só atribui o último valor lógico
}