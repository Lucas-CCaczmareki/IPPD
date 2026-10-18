#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <omp.h>
#include <iostream>

#define N 900

double trabalho (long i, long k, long long& t_count) {
    double x = 0;
    long long n = i * i * k;
    t_count += n;

    // ou seja, imagina 1 for que roda 100 vezes essa função aqui
    // na iteração 1-25 ele vai rodar menos vezes (pq o tanto de vezes que essa função roda depende do número da iteração do outro for)
    // do que na iteração 75-100. Isso faz com que a thread 4 do pool tenha muuuuuito mais operações pra realizar que as outras se o escalonamento (divisão)
    // for static
    for (long long j = 0; j < n; ++j) {
        x += std::sqrt((double)j);
    }
    return x;
}

int main (int argc, char* argv[]) {
    double static_res = 0, dynamic_res = 0, guided_res = 0;
    double t0;

    t0 = omp_get_wtime();
    //schedule static divide as iterações do for em partes iguais
    // ex: threads = 4, iterações = 100, então cada 1 tem 25 iterações (porém desse jeito é desbalanceado nessa f())
    #pragma omp parallel 
    {
        int t_id = omp_get_thread_num();
        long long t_count = 0;

        #pragma omp for schedule(static) reduction(+ : static_res) 
        for(int i = 0; i < N; ++i) {
            static_res += trabalho (i, 2, t_count);
        }   
        
        # pragma omp critical
        {
            std::cout << "iterations thread " << t_id << ": " << t_count << "\n";
        }

    }
    std::cout << "static time: " << omp_get_wtime()-t0 << "\n value: " << static_res << "\n\n";

    t0 = omp_get_wtime();
    static_res = 0;
    //schedule static divide as iterações do for em partes iguais
    // ex: threads = 4, iterações = 100, então cada 1 tem 25 iterações (porém desse jeito é desbalanceado nessa f())
    #pragma omp parallel 
    {
        int t_id = omp_get_thread_num();
        long long t_count = 0;


        // limitar o tamanho das chunks pra 1 já reduz o impacto do problema de todas iterações caras irem pra uma única thread
        #pragma omp for schedule(static, 1) reduction(+ : static_res) 
        for(int i = 0; i < N; ++i) {
            static_res += trabalho (i, 2, t_count);
        }   
        
        # pragma omp critical
        {
            std::cout << "iterations thread " << t_id << ": " << t_count << "\n";
        }

    }
    std::cout << "static time, chunk size 1: " << omp_get_wtime()-t0 << "\n value: " << static_res << "\n\n";


    t0 = omp_get_wtime();
    // schedule [,dynamic] define o tamanho do chunk e a maneira que cada thread vai executar
    // por padrão o tamanho é uma iteração por chunk
    // e cada thread pega a próxima chunk quando ficar livre. 
    // Assim diminui o problema de uma thread ficar esperando enquanto a outra pegou uma chunk com muito mais trabalho
    #pragma omp parallel 
    {
        int t_id = omp_get_thread_num();
        long long t_count = 0;

        # pragma omp for schedule(dynamic) reduction(+ : dynamic_res)
        for(int i = 0; i < N; ++i) {
            dynamic_res += trabalho(i, 2, t_count);
        }

        // área crítica necessária. As threads podem acabar terminando o for junto e executarem o print misturando à si mesmas.
        # pragma omp critical
        {
            std::cout << "iterations thread " << t_id << ": " << t_count << "\n";
        }
    }
    std::cout << "dynamic time: " << omp_get_wtime()-t0 << "\n value: " << dynamic_res << "\n\n";    

    t0 = omp_get_wtime();
    // schedule [,guided] funciona de maneira similar, porém aqui os chunks iniciam grandes e vão diminuindo até o número mínimo passaado
    // por padrão é 1. (ex 255 até 1)
    # pragma omp parallel
    {
        int t_id = omp_get_thread_num();
        long long t_count = 0;

        # pragma omp for schedule(guided) reduction(+ : guided_res)
        for(int i = 0; i < N; ++i) {
            guided_res += trabalho(i, 2, t_count);
        }

        // área crítica necessária para as threads 
        # pragma omp critical
        {
            std::cout << "iterations thread " << t_id << ": " << t_count << "\n";
        }
    }
    std::cout << "guided time: " << omp_get_wtime()-t0 << "\n value: " << guided_res << "\n\n";    

    return 0;
}