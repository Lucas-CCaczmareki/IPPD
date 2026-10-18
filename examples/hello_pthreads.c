#include <pthread.h>
#include <stdio.h>

// Thread criada 1
// corpo da thread criada;
// Assinatura sempre seuge void *funcao(void *arg)
// recebendo ponteiro genérico e retornando ponteiro genérico
// isso é assim por que a lib n sabe qual tipo de arg ou retorno vai ser usado
// então ela padroniza tudo como void* e fica à cargo do programador fazer o casting pro tipo certo

void *hello(void *arg) {
    printf("Hello thread!\n");
    return NULL;
}

// Thread principal
int main() {
    pthread_t t; //identificador da nova thread

    // inicia um fork
    // 1o arg: quando faz o create, joga o identificador da thread na variável
    // 2o arg: não define atributos da thread. NULL significa o padrão
    // 3o arg: coloca a função que a thread vai executar
    // 4o arg: argumento que vai ser passado pra dentro da thread. Nesse caso NULL pq n passamos nenhum
    pthread_create(&t, NULL, hello, NULL);
    // após o create o programa já tem uma thread rodadno no sistema operacional
    // que é escalonável. Ela vai rodar concorrentemente com o resto da thread principal

    // o thread_join bloqueia a thread main até que a thread passada como parâmetro 1 finalize
    // o segundo parâmetro seria onde receberíamos o retorno da thread (por exemplo, se ela calculasse algo e recebêssemos um número)
    pthread_join(t, NULL);
    return 0;
}