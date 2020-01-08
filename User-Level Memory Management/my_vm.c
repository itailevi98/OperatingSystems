#include "my_vm.h"

int initialized = -1;
int writers;
int writing;
int reading;

//int miss = 0;
//int hits = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t broadcast = PTHREAD_COND_INITIALIZER;

/*
Function responsible for allocating and setting your physical memory 
*/

void set_physical_mem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating

    
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them
	
	int offset = findLogBaseTwo(PGSIZE);	
	int innerPageBits = (int)((32 - offset) + findLogBaseTwo(ENTRYSIZE)) - offset;
	int outerPageBits = 32 - innerPageBits - offset;
	double outerPageSize = pow(2,outerPageBits);
			
	int pageTableSize = pow(2,innerPageBits);
	pageDirectory = (pde_t ***)malloc(outerPageSize * sizeof(pde_t));
	
	unsigned long i;
	unsigned long bytes = 0;	
	
	for(i = 0; i < outerPageSize; i++){
		
		pageDirectory[i] = (pde_t **)malloc(pageTableSize * sizeof(pde_t));
		bytes = bytes + pageTableSize;
	}



	double pageNumberBits = 32 - offset;
	double bitmapSize = pow(2,pageNumberBits);
	allocated = (unsigned char *)malloc(bitmapSize * sizeof(unsigned char));
	freed = (unsigned char *)malloc(bitmapSize * sizeof(unsigned char));

	for(i = 0; i < bitmapSize; i++){

		allocated[i] = 0;
		freed[i] = 0; 
	}

}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t * translate(pde_t *pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address

	int offset = findLogBaseTwo(PGSIZE);	
	int innerPageBits = (int)((32 - offset) + findLogBaseTwo(ENTRYSIZE)) - offset;
	int outerPageBits = 32 - innerPageBits - offset;

	unsigned long PDIndex = (unsigned long)va >> (innerPageBits + offset);
	unsigned long PTIndex =((unsigned long)va >> offset) & ((1 << innerPageBits) - 1);

	void *entry = (void *)(pageDirectory[PDIndex])[PTIndex];

	if(entry){		

		return ((pte_t *)entry);

	}else{

    	//If translation not successfull
		return NULL;

  	}
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/

int
page_map(pde_t *pgdir, void *va, void *pa)
{

    /*HINT: Similar to translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

	int offset = findLogBaseTwo(PGSIZE);
	int innerPageBits = (int)((32 - offset) + findLogBaseTwo(ENTRYSIZE)) - offset;
	int outerPageBits = 32 - innerPageBits - offset;

	pde_t PDIndex = (unsigned long)va >> (innerPageBits + offset);
	pde_t PTIndex =((unsigned long)va >> offset) & ((1 << innerPageBits) - 1);
	
	unsigned long index = (PDIndex * pow(2,innerPageBits)) + PTIndex;

	//printf("PD Index 0x%d | PTIndex 0x%d || Checking Index %lu in bitmap\n",PDIndex,PTIndex,index);

	if(!checkBitInMap(allocated,index)){
		
		(pageDirectory[PDIndex])[PTIndex] = (pte_t *)pa;
		return 1;		

	}else{

		
		(pageDirectory[PDIndex])[PTIndex] = (pte_t *)pa;
		return 1;

	}		

    return -1;
}

/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {
 
    //Use virtual address bitmap to find the next free page
	int i, flag = 0;
	int offset = findLogBaseTwo(PGSIZE);
	int innerPageBits = (int)((32 - offset) + findLogBaseTwo(ENTRYSIZE)) - offset;
	int outerPageBits = 32 - innerPageBits - offset;
	int pageTableEntries = (int)pow(2,(innerPageBits+outerPageBits));
	int pageStartForAllocation = -1, pagesCountedForAllocation = 0;
	for(i = 0; i < pageTableEntries; i++){
		
		if(((!checkBitInMap(allocated,i) && !checkBitInMap(freed,i)) || (!checkBitInMap(allocated,i) && checkBitInMap(freed,i)))  && flag == 0){
		
			pageStartForAllocation = i;
			pagesCountedForAllocation = 1;
			flag = 1;

		}else if(((!checkBitInMap(allocated,i) && !checkBitInMap(freed,i)) || (!checkBitInMap(allocated,i) && checkBitInMap(freed,i))) && flag == 1){

			pagesCountedForAllocation++;

		}else if(checkBitInMap(allocated,i)){

			flag = 0;
			pagesCountedForAllocation = 0;
			pageStartForAllocation = -1;

		} 

		if(pagesCountedForAllocation == num_pages){

			return (void *)pageStartForAllocation;			

		}	
			
	}

	return NULL;
	
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *a_malloc(unsigned int num_bytes) {

    //HINT: If the physical memory is not yet initialized, then allocate and initialize.

   /* HINT: If the page directory is not initialized, then initialize the
   page directory. Next, using get_next_avail(), check if there are free pages. If
   free pages are available, set the bitmaps and map a new page. Note, you will 
   have to mark which physical pages are used. */

	pthread_mutex_lock(&lock); 
	writers++;
	while(reading || writing){

		pthread_cond_wait(&broadcast,&lock);
		
	}
	writing++;
	pthread_mutex_unlock(&lock);

	if(initialized == -1){
		
	
		set_physical_mem();
		initialized = 0;
		
	}	

	//printf("Page %p\n",page);
	int offset = findLogBaseTwo(PGSIZE);
	int innerPageBits = (int)((32 - offset) + findLogBaseTwo(ENTRYSIZE)) - offset;
	int outerPageBits = 32 - innerPageBits - offset;
	
	int pageTableEntries = (int)pow(2,(innerPageBits));
	
	void *virtualAddress;
	virtualAddress = NULL;
	int addExtraPage = 1;

	if((num_bytes % PGSIZE) == 0){

		addExtraPage = 0;

	}
	unsigned int numberOfPagesToAllocate = (unsigned int)(num_bytes/PGSIZE) + addExtraPage;	
	//printf("Number of Pages to allocate: %d\n", numberOfPagesToAllocate);
	int pageOfFreeIndex = (int)get_next_avail(numberOfPagesToAllocate);

	unsigned long innerBits = (unsigned long)((int)(pageOfFreeIndex % pageTableEntries)); 
	unsigned long outerBits = (unsigned long)((int)(pageOfFreeIndex / pageTableEntries));
	unsigned long pageNumber = outerBits;

	pageNumber = (pageNumber << innerPageBits ) | innerBits;

	unsigned long tempVirtualAddress = (pageNumber << offset);
	virtualAddress = (void *)(tempVirtualAddress);
	//printf("First Free Page: %d\n", pageOfFreeIndex);

	unsigned int i;
	for(i = 0; i < numberOfPagesToAllocate;i++){
		
		innerBits = (unsigned long)((int)(pageOfFreeIndex % pageTableEntries)); 
		outerBits = (unsigned long)((int)(pageOfFreeIndex / pageTableEntries));
		pageNumber = outerBits;

		pageNumber = (pageNumber << innerPageBits ) | innerBits;

		tempVirtualAddress = (pageNumber << offset);
				
		void *mapVirtualAddress = (void *)(tempVirtualAddress);
		
		//printf("0x%x: %p\n",tempVirtualAddress,page);
		if(!checkBitInMap(freed,pageOfFreeIndex)){

			pte_t *page = (pte_t *)malloc(PGSIZE);
			int status = page_map(NULL, mapVirtualAddress, page);

		}	
		
		setBitInMap(allocated,pageOfFreeIndex);
		clearBitInMap(freed,pageOfFreeIndex);
		pageOfFreeIndex++;

	}	

	pthread_mutex_lock(&lock);
	writing--;
	writers--;
	pthread_cond_broadcast(&broadcast);
	pthread_mutex_unlock(&lock);

    return virtualAddress; // virtualAddress
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void a_free(void *va, int size) {
	
    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid

	pthread_mutex_lock(&lock); 
	writers++;
	while(reading || writing){

		pthread_cond_wait(&broadcast,&lock);
		
	}
	writing++;
	pthread_mutex_unlock(&lock);

	int offset = findLogBaseTwo(PGSIZE);
	int innerPageBits = (int)((32 - offset) + findLogBaseTwo(ENTRYSIZE)) - offset;
	int outerPageBits = 32 - innerPageBits - offset;
	
	unsigned long PDIndex = (unsigned long)va >> (innerPageBits + offset); //index of page table we want
	unsigned long PTIndex =((unsigned long)va >> offset) & ((1 << innerPageBits) - 1); //index of page we want
	
	int i, pagesToFree,addExtraPage = 1;
	int tempPTIndex = (PDIndex * pow(2,innerPageBits)) + PTIndex;

	if((size % PGSIZE) == 0){

		addExtraPage = 0;

	}

	pagesToFree = (size/PGSIZE) + addExtraPage;
	for(i=0;i<pagesToFree;i++){

		if(checkBitInMap(freed,tempPTIndex)){

			fprintf(stderr,"CANNOT FREE ALREADY FREED REGION\n");
			exit(1);

		}
			
		if((i*PGSIZE) >= pow(2,(innerPageBits+offset))){
				
			PDIndex = PDIndex + 1;
			tempPTIndex = (PDIndex * pow(2,innerPageBits));
				
		}
		
		clearBitInMap(allocated,tempPTIndex);
		setBitInMap(freed,tempPTIndex);

		tempPTIndex++;
		
	}
	pthread_mutex_lock(&lock);
	writing--;
	writers--;
	pthread_cond_broadcast(&broadcast);
	pthread_mutex_unlock(&lock);
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/

void put_value(void *va, void *val, int size) {

    /* HINT: Using the virtual address and translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using translate() //0001 0000
       function.*/
		
		//printf("Physical Address: %p | VPN: %x\n",tlb_store[vpn].ppn, vpn);

		pthread_mutex_lock(&lock); 
		writers++;
		while(reading || writing){

			pthread_cond_wait(&broadcast,&lock);
			
		}
		writing++;
		pthread_mutex_unlock(&lock);
	
		int pageOffset = findLogBaseTwo(PGSIZE);
	
		unsigned long vpn = (unsigned long)va;
		unsigned long offset = vpn & ~(1 << pageOffset);
	
		pte_t *pa;
		pa = translate(NULL, va) + offset;		
		*pa = *((pte_t *)val);
		va = va + size;	

		pthread_mutex_lock(&lock);
		writing--;
		writers--;
		pthread_cond_broadcast(&broadcast);
		pthread_mutex_unlock(&lock);

}


/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */
	
		pthread_mutex_lock(&lock); 
		writers++;
		while(reading || writing){

			pthread_cond_wait(&broadcast,&lock);
			
		}
		writing++;
		pthread_mutex_unlock(&lock);

		int pageOffset = findLogBaseTwo(PGSIZE);

		unsigned long vpn = (unsigned long)va;
		unsigned long offset = vpn & ~(1 << pageOffset);

		pte_t *pa;
		pa = translate(NULL, va) + offset;
		*((pte_t *)val) = *pa;
		va = va + size;	
	
		pthread_mutex_lock(&lock);
		writing--;
		writers--;
		pthread_cond_broadcast(&broadcast);
		pthread_mutex_unlock(&lock);

		//printf("{GET - VALUE - 0x%x} Virtual Address: %p | Physical Address: %p | Value = %lu\n",va,pa,*pa);	
}

/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
    matrix accessed. Similar to the code in test.c, you will use get_value() to
    load each element and perform multiplication. Take a look at test.c! In addition to 
    getting the values from two matrices, you will perform multiplication and 
    store the result to the "answer array"*/

	//printf("{Matrix - Multiply} Address A: %p | Address: %p\n", mat1, mat2);

	pte_t y, z, c = 0;
	int address_a = 0, address_b = 0, address_d = 0;
	void *a = mat1, *b = mat2, *d = answer, *aTemp = a;
	int i,j,k;

	for (i = 0; i < size; i++) {
		for(k = 0; k < size; k++){
			address_b = (unsigned int)b;
		    for (j = 0; j < size; j++) {
		        address_a = (unsigned int)a + ((i * size * sizeof(int))) + (j * sizeof(int));
		        get_value((void *)address_a, &y, sizeof(int));
		        get_value( (void *)address_b, &z, sizeof(int));
				//printf("A: %p | B: %p\n",address_a, address_b);		
				c = c + (y * z);	
				address_b = address_b + (size * 0x4);				
		    }
		    b += 0x4;
		    a = aTemp;
        	//printf("%d ", c);
        	address_d = (unsigned int)d + ((i * size * sizeof(int))) + (k * sizeof(int));
			//printf("Address C: %x\n",address_d);
        	put_value((void *)address_d, &c, sizeof(int));
			c = 0;
		}
		//printf("A: %p | ",a);	
		aTemp = a;
		b = mat2;
		//printf("Completed %d iterations\n",i);
    }
  //  printf("Misses: %d | Hits: %d\n", miss,hits);
}

/*
#################################################################################################
#																								#
#																								#
#										HELPER FUNCTIONS										#
#								(MAKE SURE TO DECLARE PROTOTYPES)								#
#																								#
#																								#
#################################################################################################
*/

double findLogBaseTwo(double number){

	return log(number)/log(2);

}

/*
		long long virtualAddress = (long long)va;
		int tlbBits = findLogBaseTwo(TLB_SIZE);
		int virtualAddressBits = findLogBaseTwo(virtualAddress);
*/

