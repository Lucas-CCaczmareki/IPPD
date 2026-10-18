#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char *argv[]) {
    #pragma omp parallel
    {
        printf("Ola da thread %d\n", omp_get_thread_num());
    }
}
