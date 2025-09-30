#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// macro para verificar o menor de 2 números

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// definição dos valores de CHUNK_SIZE e MAX_NUMBER

#define CHUNK_SIZE 5000
#define MAX_NUMBER 5000000

// struct de início e fim do chunk

struct pair
{
    int begin;
    int end;
};

// struct de resultado de uma Run

struct runResult
{
    double time;
    int number_of_primes;
};

// variáveis globais de controle

int next;
int total;

bool singleResult = true;

// mutex para o next, total, e barra de progresso

static pthread_mutex_t mtx_for_next = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_for_total = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_for_progess_bar = PTHREAD_MUTEX_INITIALIZER;

// função para pegar a diferença de 2 tempos

static inline long long diff_ns(struct timespec a, struct timespec b)
{
    return (b.tv_sec - a.tv_sec) * 1000000000LL + (b.tv_nsec - a.tv_nsec);
}

// função para verificar se um número é primo

bool isPrime(int n)
{
    if (n < 2)
        return false;
    if (n == 2)
        return true;
    if ((n & 1) == 0)
        return false;
    int limit = (int)sqrt((double)n);
    for (int i = 3; i <= limit; i += 2)
    {
        if ((n % i) == 0)
            return false;
    }
    return true;
}

// função para contar quantos primos exitem entre dois números (inclusive)

int getPrimesInChunk(int begin, int end)
{
    int total_local = 0;
    for (int i = begin; i <= end; i++)
    {
        if (isPrime(i) == true)
            total_local++;
    }
    return total_local;
}

// função para printar barra de progresso

void printProgress(size_t count, size_t max)
{
    const int bar_width = 50;

    float progress = (float)count / max;
    int bar_length = progress * bar_width;

    pthread_mutex_lock(&mtx_for_progess_bar);

    printf("\rProgresso: [");
    for (int i = 0; i < bar_length; ++i)
    {
        printf("#");
    }
    for (int i = bar_length; i < bar_width; ++i)
    {
        printf(" ");
    }
    printf("] %.2f%%", MIN(progress * 100, 100));

    fflush(stdout);

    pthread_mutex_unlock(&mtx_for_progess_bar);
}

// função para pegar os valores do próximo chunk

struct pair getChunk()
{
    pthread_mutex_lock(&mtx_for_next);
    int begin = next;
    next += CHUNK_SIZE;
    pthread_mutex_unlock(&mtx_for_next);
    int end = MIN(begin + CHUNK_SIZE - 1, MAX_NUMBER);
    if (singleResult)
        printProgress(begin, MAX_NUMBER);
    return (struct pair){
        begin, end};
}

// função que será executada por cada thread

void *worker()
{
    int total_local = 0;
    while (true)
    {
        struct pair chunk = getChunk();

        if (chunk.begin > chunk.end)
            break;

        total_local += getPrimesInChunk(chunk.begin, chunk.end);
    }

    pthread_mutex_lock(&mtx_for_total);
    total += total_local;
    pthread_mutex_unlock(&mtx_for_total);

    return NULL;
}

// função para printar o resultado de uma run

void printSingleResult(int primes, double time_elapsed)
{
    printf("\nNúmero de primos: %d -- %d ms\n", primes, (int)time_elapsed);
}

// função que cria as threads e retorna o tempo de execução e o número de primos

struct runResult run(int number_of_threads)
{
    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);

    total = 0;
    next = 0;

    pthread_t th[number_of_threads];

    for (int i = 0; i < number_of_threads; i++)
    {
        if (pthread_create(&th[i], NULL, worker, NULL) != 0)
        {
            perror("pthread_create");
            exit(1);
        }
    }
    for (int i = 0; i < number_of_threads; i++)
        pthread_join(th[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    long long ns = diff_ns(t0, t1);

    if (singleResult)
        printSingleResult(total, ns / 1e6);

    return (struct runResult){ns / 1e6, total};
}

// função para printar os cabeçalhos da tabela

void printTableHeader()
{
    printf("k   tempo_ms    total_primos    speedup_vs_k1\n");
}

// array de espaços entre as colunas

int columnWidths[] = {0, 11, 16, 17};

// função para printar uma linha da tabela

void printTableLine(int k, double time, int primos, double speedUp)
{
    printf("%*d%*d%*d%*.3f\n",
           columnWidths[0], k, columnWidths[1], (int)time, columnWidths[2], primos, columnWidths[3], speedUp);
}

// função para verificar o SpeedUp de um tempo VS outro

double getSpeedVsK1(double time, double k1)
{
    return k1 * 100 / time;
}

// função para executar o benchmark e printar a tabela

void bench()
{
    int threads[] = {1, 2, 4, 6, 8};

    struct runResult k1 = run(1);

    printTableHeader();
    printTableLine(1, k1.time, k1.number_of_primes, getSpeedVsK1(k1.time, k1.time));

    for (int i = 1; i < 5; i++)
    {
        struct runResult result = run(threads[i]);
        printTableLine(threads[i], result.time, result.number_of_primes, getSpeedVsK1(result.time, k1.time));
    }
}

// função para pegar o número de CPUs da máquina

int getNumberOfCPUs()
{
    long num_processors = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_processors == -1)
    {
        perror("sysconf");
        return 1;
    }
    return num_processors;
}

// função main: verifica os argumentos e chama as funções correspondentes

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        printf("Missing arguments. Usage: ./primos <number of threads>\n");
        return 1;
    }

    if (strcmp("bench", argv[1]) == 0)
    {
        singleResult = false;
        bench();
        return 0;
    }

    const char digits[] = "0123456789";
    if (strspn(argv[1], digits) != strlen(argv[1]))
    {
        printf("Argument must be 'bench' or a integer. Usage: ./primos <number of threads>\n");
        return 1;
    }

    int number_of_threads = atoi(argv[1]);

    if (number_of_threads < 1)
    {
        int cpus = getNumberOfCPUs();
        printf("Rodando código com %d thread(s) (Número de CPUs)\n", cpus);
        run(getNumberOfCPUs());
    }
    else
    {
        printf("Rodando código com %d thread(s)\n", number_of_threads);
        run(number_of_threads);
    }

    return 0;
}