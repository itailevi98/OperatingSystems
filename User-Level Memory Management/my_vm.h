#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need

#define PGSIZE 32

// Maximum size of your memory
#define MAX_MEMSIZE 4*1024*1024*1024

#define MEMSIZE 1024*1024*1024

#define setBitInMap(bitmap,position)     (bitmap[(position/8)] |= (1<<(position%8)))   
#define clearBitInMap(bitmap,position)   (bitmap[(position/8)] &= ~(1<<(position%8)))  
#define checkBitInMap(bitmap,position)    (bitmap[(position/8)] & (1 << (position%8)))    

#define ENTRYSIZE 4
// Represents a page table entry
typedef unsigned long pte_t;

// Represents a page directory entry
typedef unsigned long pde_t;




pde_t ***pageDirectory;

unsigned char *allocated;
unsigned char *freed;
#define TLB_SIZE 128 //((1024*1024*1024/PGSIZE) * 4)

//Structure to represents TLB
struct tlb {

	int valid;
	pte_t *ppn;
	unsigned long tag;
    //Assume your TLB is a direct mapped TLB of TBL_SIZE (entries)
    // You must also define wth TBL_SIZE in this file.
    //Assume each bucket to be 4 bytes
};

struct tlb *tlb_store;

double findLogBaseTwo(double number);
void outputBitMap();
void set_physical_mem();
pte_t* translate(pde_t *pgdir, void *va);
int page_map(pde_t *pgdir, void *va, void* pa);
bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *a_malloc(unsigned int num_bytes);
void a_free(void *va, int size);
void put_value(void *va, void *val, int size);
void get_value(void *va, void *val, int size);
void mat_mult(void *mat1, void *mat2, int size, void *answer);

#endif
