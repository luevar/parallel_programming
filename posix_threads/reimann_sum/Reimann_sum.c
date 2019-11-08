#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define ERROR_CREATE_THREAD -11
#define ERROR_JOIN_THREAD -12

typedef double(*pointFunc)(double);

double f(double x) {
    return (x*x + 25*x + 23);
}

typedef struct integrationArgs_tag {
    pointFunc f;
    double a, b;
    int n;
} integrationArgs_t;


void* rectangle_integral(void* args) {
  integrationArgs_t intArgs = *(integrationArgs_t*)args;
  double x, h;
  double sum = 0.0;
  h = (intArgs.b - intArgs.a) / intArgs.n;  //step

  for (int i = 0; i < intArgs.n; i++) {
    sum += intArgs.f(intArgs.a + i*h);
}
double* result = (double*)malloc(sizeof(double));
*result = sum * h;
return (void*)result;
}

int main() {
	int threads_count, status;
    struct timespec start, finish;
    double elapsed;
    double* returnedValue = (double*)malloc(sizeof(double));

    //algorythm based variables
    int n = 1;   //initial number of steps
    double s1 = 0, s;
    double a, b, eps;
    printf("Input left border a = ");
    scanf("%lf", &a);
    printf("Input right border b = ");
    scanf("%lf", &b);
    printf("Input required accuracy eps = ");
    scanf("%lf", &eps);
    printf("Input amount of threads = ");
    scanf("%d", &threads_count);

    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * threads_count);
    integrationArgs_t* args = (integrationArgs_t*)malloc(sizeof(integrationArgs_t) * threads_count);

    double intervalLength = (b - a) / (double)threads_count;     
    for (int i = 0; i < threads_count; i++) {
        args[i].f = f;
        args[i].a = a + intervalLength * i;
        args[i].b = a + intervalLength + intervalLength * i;
        args[i].n = n; 
    }

    for (int i = 0; i < threads_count; i++) {
        s1 += f(args[i].a);
    }
    s1 *= intervalLength; //first approximation for the integral


    clock_gettime(CLOCK_MONOTONIC, &start);
    do {
        s = s1;                 //second approximation
        s1 = 0;
        for(int i = 0; i < threads_count; i++) {
            args[i].n *= 2;     //an increase in the number of steps by a factor of two,
                                // i.e., a decrease in the value of the step by a factor of two

            status = pthread_create(&threads[i], NULL, rectangle_integral,(void*)&args[i]);
            if (status != 0) {
                printf("Can not create thread number %d\n", i + 1);
                exit(ERROR_CREATE_THREAD);
            }
        }

        for (int i = 0; i < threads_count; i++) {
            status = pthread_join(threads[i], (void**)&returnedValue);
            if (status != 0) {
                printf("Error, cant join thread %d from main thread.\n", i + 1);
                exit(ERROR_JOIN_THREAD);
            }
            s1 += *returnedValue;
        }
    } while (fabs(s1 - s) > eps);  //comparison of approximations with a given accuracy
    printf("%0.10lf\n", s1);
    clock_gettime(CLOCK_MONOTONIC, &finish);
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Program runtime: %f\n", elapsed);
    free(returnedValue);
    return 0;
}


