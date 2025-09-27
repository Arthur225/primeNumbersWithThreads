#include<stdbool.h>
#include<math.h>
#include<stdio.h>

#define CHUNK_SIZE = 5000;
#define MAX_NUMBER = 5000000;



bool isPrime(int n){
    if(n < 2) return false;
    if(n == 2) return true;
    if((n & 1) == 0) return false;
    int sqrt = pow(n, 0.5);
    for(int i = 3; i <= sqrt; i+=2){
        if((n % i) == 0) return false;
    }
    return true;
}

int getPrimesInChunk(int begin, int end){
    int total = 0;
    for(int i = begin; i <= end; i++){
        if(isPrime(i)==true) total++;
    }
    return total;
}

void printProgress(size_t count, size_t max) {
    const int bar_width = 50;

    float progress = (float) count / max;
    int bar_length = progress * bar_width;

    printf("\rProgress: [");
    for (int i = 0; i < bar_length; ++i) {
        printf("#");
    }
    for (int i = bar_length; i < bar_width; ++i) {
        printf(" ");
    }
    printf("] %.2f%%", progress * 100);

    fflush(stdout);
}

int main(int argc, char* argv[]){
    printf("%d\n",getPrimesInChunk(1,10));

    print_progress(10,20);

}