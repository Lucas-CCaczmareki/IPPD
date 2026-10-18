/* Nome do aluno: Lucas Cavallin Caczmareki */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <omp.h>

typedef struct {
    long long soma;
    int linha;
} Resultado;

/* Declare aqui seu operador de redução personalizado. */
// #pragma omp declare reduction(nome_da_reducao : tipo : combinador) [initializer(inicializador)]
//omp_out é o valor da thread atual
//omp_in é o valor da outra thread
// compara a soma da outra thread com a atual, se for maior, troca quem ganha
// OU confere se são iguais, ai SE FOR (avaliação curto circ) a outra thread tem linha menor, troca pra outra thread, se n mantém
#pragma omp declare reduction(maiorLinha : Resultado : \
    omp_out = (omp_in.soma > omp_out.soma || omp_in.soma == omp_out.soma && omp_in.linha < omp_out.linha ? omp_in : omp_out)) \
    initializer(omp_priv = (Resultado){LLONG_MIN, INT_MAX})

Resultado sua_implementacao(int **m, int linhas, int colunas)
{
    Resultado resultado = {LLONG_MIN, INT_MAX};

    #pragma omp parallel for reduction(maiorLinha:resultado)
    for(int i = 0; i < linhas; i++) {

        Resultado r_linha;
        r_linha.linha = i;
        r_linha.soma = 0;

        for(int j = 0; j < colunas; j++) {
            r_linha.soma += m[i][j];
        }

        int maiorSoma = r_linha.soma > resultado.soma;
        int menorLinha = r_linha.soma == resultado.soma && r_linha.linha < resultado.linha;

        if (maiorSoma || menorLinha) {
            resultado.soma = r_linha.soma;
            resultado.linha = r_linha.linha;
        }
    }

    return resultado;
}

int main(int argc, char **argv)
{
    const int linhas = 100;
    const int colunas = 100;
    char *fim;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <semente>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argv[1][0] == '\0') {
        fprintf(stderr, "Semente inválida.\n");
        return EXIT_FAILURE;
    }

    for (const char *p = argv[1]; *p != '\0'; p++) {
        if (*p < '0' || *p > '9') {
            fprintf(stderr, "Semente inválida.\n");
            return EXIT_FAILURE;
        }
    }

    errno = 0;
    unsigned long semente = strtoul(argv[1], &fim, 10);

    if (errno == ERANGE || *fim != '\0' || semente > UINT_MAX) {
        fprintf(stderr, "Semente fora do intervalo permitido.\n");
        return EXIT_FAILURE;
    }

    int **m = malloc(linhas * sizeof *m);
    if (m == NULL) {
        fprintf(stderr, "Falha na alocação da matriz.\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < linhas; i++) {
        m[i] = malloc(colunas * sizeof *m[i]);
        if (m[i] == NULL) {
            for (int k = 0; k < i; k++)
                free(m[k]);
            free(m);
            fprintf(stderr, "Falha na alocação da matriz.\n");
            return EXIT_FAILURE;
        }
    }

    /* Inicialização sequencial da matriz. */
    srand((unsigned int) semente);
    for (int i = 0; i < linhas; i++)
        for (int j = 0; j < colunas; j++)
            m[i][j] = rand() % 201 - 100;
            // m[i][j] = 1;

    Resultado resultado = sua_implementacao(m, linhas, colunas);

    printf("Linha: %d\n", resultado.linha);
    printf("Soma: %lld\n", resultado.soma);

    for (int i = 0; i < linhas; i++)
        free(m[i]);
    free(m);
    return EXIT_SUCCESS;
}