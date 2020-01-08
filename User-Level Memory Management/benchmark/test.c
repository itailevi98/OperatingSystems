#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "../my_vm.h"

#define SIZE 32

int main() {

    printf("Allocating three arrays of 400 bytes\n");
    void *a = a_malloc(1024*4);//4198401
    int old_a = (int)a;
    void *b = a_malloc(1024*4);
    void *c = a_malloc(1024*4);
    int x = 1;
    int y, z;
    int i = 0, j=0;
    int address_a = 0, address_b = 0;
    int address_c = 0;

    printf("Addresses of the allocations: %x, %x, %x\n", (int)a, (int)b, (int)c);
	//outputBitMap();
    printf("Storing integers to generate a SIZExSIZE matrix\n");
	int amountOfIntsCreated = 0;
    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            address_a = (unsigned int)a + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            address_b = (unsigned int)b + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            put_value((void *)address_a, &x, sizeof(int));
            put_value((void *)address_b, &x, sizeof(int));
			amountOfIntsCreated++;
        }
    } 
	
    printf("Fetching matrix elements stored in the arrays\n");

    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            address_a = (unsigned int)a + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            address_b = (unsigned int)b + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            get_value((void *)address_a, &y, sizeof(int));
            get_value( (void *)address_b, &z, sizeof(int));
			//printf("Address A: %p | Address: %p\n", address_a,address_b);
            printf("%d ", y);
        }
        printf("\n");
    } 

    printf("Performing matrix multiplication with itself!\n");
    mat_mult(a, b, SIZE, c);


    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            address_c = (unsigned int)c + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            get_value((void *)address_c, &y, sizeof(int));
            printf("%d ", y);
        }
        printf("\n");
    }
    printf("Freeing the allocations!\n");
    a_free(a, 1024*4);
    a_free(b, 1024*4);
    a_free(c, 1024*4);
    printf("Stored %d integers.\n",amountOfIntsCreated);
    printf("Checking if allocations were freed!\n");
    a = a_malloc(1024*4);
    if ((int)a == old_a)
        printf("free function works\n");
    else
        printf("free function does not work\n");

    return 0;
}
