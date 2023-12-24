#include <inc/lib.h>

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
//struct Upages module[NUM_OF_UHEAP_PAGES] = {0};
int module[NUM_OF_UHEAP_PAGES] = { 0 };
int FirstTimeFlag = 1;
void InitializeUHeap() {
	if (FirstTimeFlag) {
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment) {
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
//NEEDS WORK !!!!!!!!!!
void* malloc(uint32 size) {
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0)
		return NULL;
	//==============================================================
	else if (sys_isUHeapPlacementStrategyFIRSTFIT()
			&& size <= (uint32) (USER_HEAP_MAX - USER_HEAP_START)) {
		if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		{
			return alloc_block_FF(size);
		}
		else
		{
			uint32 HARD_LIMIT = sys_hard_limit();
			cprintf("HARDLIMIT : %x", HARD_LIMIT);
			uint32 required_pages = ROUNDUP(size , PAGE_SIZE) / PAGE_SIZE;
			uint32 startva;
			uint32 found_pages = 0;
			for (uint32 i = (HARD_LIMIT + PAGE_SIZE);i < (uint32) USER_HEAP_MAX; i += PAGE_SIZE)
					{
				if (found_pages == required_pages) {
					break;
				}
				uint32 page_perm = sys_get_perm(i);
				if ((page_perm & PERM_AVAILABLE) != PERM_AVAILABLE || page_perm == -1) //(not reserved yet)
				{
					if (found_pages == 0)
						startva = i;
					found_pages++;
				} else {
					found_pages = 0;
				}
			}
			if (found_pages == required_pages) {
				int index = (startva - USER_HEAP_START) / PAGE_SIZE;
				module[index] = required_pages;
				sys_allocate_user_mem(startva, (required_pages * PAGE_SIZE));
				return (uint32*) startva;
			} else
				return NULL;
		}
	}
	return NULL;
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
//NEEDS WORK !!!!!!!!!!
void free(void* virtual_address) {

	//cprintf("virtual address free required : %x \n", virtual_address);

	uint32* HARD_LIMIT = (uint32*) sys_hard_limit();
	//TODO: [PROJECT'23.MS2 - #11] [2] USER HEAP - free() [User Side]
	// Write your code here, remove the panic and write your code
	if ((uint32*) virtual_address < HARD_LIMIT && (uint32)virtual_address >= USER_HEAP_START) {
		free_block(virtual_address);
	}
	else if (((uint32) virtual_address >=  ((uint32)HARD_LIMIT + PAGE_SIZE))&& ((uint32)virtual_address < USER_HEAP_MAX))
	{
		int index = ((uint32) virtual_address - USER_HEAP_START) / PAGE_SIZE;
		if (module[index] > 0)
		{
			sys_free_user_mem((uint32) virtual_address,(module[index] * PAGE_SIZE));
			module[index] = 0;
		}
	}
	else {
		panic("invalid virtual address");
	}
}

//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable) {
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0)
		return NULL;
	//==============================================================
	panic("smalloc() is not implemented yet...!!");
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName) {
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// Write your code here, remove the panic and write your code
	panic("sget() is not implemented yet...!!");
	return NULL;
}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size) {
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================

	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address) {
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}

//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize) {
	panic("Not Implemented");

}
void shrink(uint32 newSize) {
	panic("Not Implemented");

}
void freeHeap(void* virtual_address) {
	panic("Not Implemented");

}
