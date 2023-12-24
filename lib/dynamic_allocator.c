/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"

struct MemBlock_LIST HEAP_List;
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free) ;
	}
	cprintf("=========================================\n");

}
//
////****************************//
////****************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================

bool is_initialized = 0;
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	if (initSizeOfAllocatedSpace == 0)
		return ;
	else if(initSizeOfAllocatedSpace > 0)
	{
LIST_INIT(&HEAP_List);
struct BlockMetaData* block=(struct BlockMetaData*)daStart;
block->is_free=1;
//block->prev_next_info.le_prev=NULL;
//block->prev_next_info.le_next=NULL;
block->size=initSizeOfAllocatedSpace;
LIST_INSERT_TAIL(&HEAP_List, block);
is_initialized = 1;
	}
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	if(size==0)
		return NULL;

	if (!is_initialized)
	{
		uint32 required_size = size + sizeOfMetaData();
		uint32 da_start = (uint32)sbrk(required_size);
		//get new break since it's page aligned! thus, the size can be more than the required one
		uint32 da_break = (uint32)sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}

	uint32 SizeMeta_Block = size + sizeOfMetaData();
	struct BlockMetaData* search ;
	bool found = 0;
	LIST_FOREACH(search , &HEAP_List)
	{
		        if(search->is_free == 1)
				{
					if(search->size == SizeMeta_Block)
					{
						search->is_free = 0;
						found = 1;
					}
					else if (search->size > SizeMeta_Block)
					{
						found = 1;
						uint32 freeSpace = search->size - SizeMeta_Block;
						search->size = SizeMeta_Block;
						search->is_free = 0;
						if(freeSpace > sizeOfMetaData())
						{
							struct BlockMetaData* freeMeta=(struct BlockMetaData*)((uint32)search + SizeMeta_Block);
							freeMeta->size = freeSpace;
							freeMeta->is_free = 1;
							LIST_INSERT_AFTER(&HEAP_List, search, freeMeta);
						}
						else
						{
							search->size += freeSpace;
						}
					}
					if(found == 1)
						return (uint32*)((uint32)search + sizeOfMetaData());
					}
	}
	if(found == 0)
	{
		struct BlockMetaData* sbrk_return = (struct BlockMetaData*)sbrk(SizeMeta_Block);

		if((void*)sbrk_return!=(void*)-1)
		{
			    SizeMeta_Block=ROUNDUP(SizeMeta_Block,PAGE_SIZE);
				// the returning size from sbrk
				if(SizeMeta_Block == size + sizeOfMetaData())
				{
					struct BlockMetaData* last_block = sbrk_return;
					last_block->size = SizeMeta_Block;
					last_block->is_free=0;
					LIST_INSERT_TAIL(&HEAP_List, last_block);
					return (void*)((uint32)last_block + sizeOfMetaData());
				}
				else if(SizeMeta_Block > size + sizeOfMetaData())//then split
				{
					struct BlockMetaData* first_block = sbrk_return;
					first_block->size = size + sizeOfMetaData();
					first_block->is_free = 0;
					LIST_INSERT_TAIL(&HEAP_List, first_block);
					uint32 freeSpace = SizeMeta_Block - first_block->size;

					if(freeSpace > sizeOfMetaData())
					{
						struct BlockMetaData* freeMeta=(struct BlockMetaData*)((uint32)sbrk_return + first_block->size);
						freeMeta->size = freeSpace;
						freeMeta->is_free = 1;
						LIST_INSERT_TAIL(&HEAP_List, freeMeta);
					}
					else
					{
						first_block->size += freeSpace;
					}
					return (void*)((uint32)first_block + sizeOfMetaData());

				}
			}
		else
			return NULL;

		}
	return NULL;

}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
		if(size==0)
			return NULL;

		uint32 SizeMeta_Block = size + sizeOfMetaData();
		struct BlockMetaData* search = HEAP_List.lh_first;
		struct BlockMetaData* BFadd = NULL;
		uint32 minsize = 99999999;

		do{
			 if(search->is_free == 1)
			 {
				if(search->size == SizeMeta_Block)
				{
					search->is_free = 0;
					return (struct BlockMetaData*)((uint32)search + sizeOfMetaData());
				}
				else if ((search->size > SizeMeta_Block) && (search->size < minsize))
				{
					BFadd = (struct BlockMetaData*) search;
					minsize = search->size;
				}
			 }
			 search = search->prev_next_info.le_next;
		}while(search != NULL);

		if(BFadd != NULL)
		{
			uint32 freeSpace = BFadd->size - SizeMeta_Block;
			BFadd->size = SizeMeta_Block;
			BFadd->is_free=0;
			if(freeSpace >= sizeOfMetaData())
			{
				struct BlockMetaData* freeMeta = (struct BlockMetaData*)((uint32)BFadd + SizeMeta_Block);
				freeMeta->size = freeSpace;
				freeMeta->is_free = 1;
				LIST_INSERT_AFTER(&HEAP_List, BFadd, freeMeta);
			}
			else
			{
				BFadd->size += freeSpace;
			}
			return (struct BlockMetaData*)((uint32)BFadd + sizeOfMetaData());
		}
		else
		{
			struct BlockMetaData* sbrk_return = (struct BlockMetaData*)sbrk(SizeMeta_Block);
				if(sbrk_return!=(void*)-1)
				{
					struct BlockMetaData* new_Block=sbrk_return;
					new_Block->size=SizeMeta_Block;
				    new_Block->is_free=0;
					new_Block->prev_next_info.le_prev = HEAP_List.lh_last;
					HEAP_List.lh_last->prev_next_info.le_next=new_Block;
					HEAP_List.lh_last=new_Block;
					return (struct BlockMetaData*)((uint32)sbrk_return + sizeOfMetaData());
				}
		}
		return NULL;
}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	if(va == NULL)
		return;

	struct BlockMetaData* meta_free=(struct BlockMetaData*)(va) - 1;

	// first case-> there is only one block
	if(meta_free->prev_next_info.le_next == NULL && meta_free->prev_next_info.le_prev==NULL)
	{
		meta_free->is_free = 1;
	}

	// first 2 blocks in list (meta_free is the one (1st block)) traced
	if(meta_free->prev_next_info.le_prev == NULL && (meta_free->prev_next_info.le_next != NULL && meta_free->prev_next_info.le_next->is_free == 1))
	{
		struct BlockMetaData* HOLD_TO_DELETE= meta_free->prev_next_info.le_next;
		meta_free->size += HOLD_TO_DELETE->size;
		meta_free->is_free = 1;
		if(HOLD_TO_DELETE->prev_next_info.le_next != NULL)
		{
			HOLD_TO_DELETE->prev_next_info.le_next->prev_next_info.le_prev = meta_free;
		}
		meta_free->prev_next_info.le_next = HOLD_TO_DELETE->prev_next_info.le_next;
		HOLD_TO_DELETE->size = 0;
		HOLD_TO_DELETE->is_free = 0;
	}

	// 3rd case ->the last 2 blocks (meta_free is the last one) traced
	else if(meta_free->prev_next_info.le_next == NULL && (meta_free->prev_next_info.le_prev!=NULL && meta_free->prev_next_info.le_prev->is_free==1))
	{
		struct BlockMetaData* ptr_pre = meta_free->prev_next_info.le_prev;
		ptr_pre->size += meta_free->size;
		meta_free->size = 0;
		meta_free->is_free = 0;
		HEAP_List.lh_last = ptr_pre;
	}

	// 4th case -> 3 free blocks (meta_free is in the middle) traced
	else if((meta_free->prev_next_info.le_next != NULL && meta_free->prev_next_info.le_next->is_free==1)&&(meta_free->prev_next_info.le_prev!=NULL && meta_free->prev_next_info.le_prev->is_free==1))
	{
		struct BlockMetaData* ptr_pre = meta_free->prev_next_info.le_prev;
		struct BlockMetaData* ptr_next = meta_free->prev_next_info.le_next;
		ptr_pre->size += meta_free->size + ptr_next->size;
		if(ptr_next->prev_next_info.le_next!=NULL)
		{
			ptr_next->prev_next_info.le_next->prev_next_info.le_prev = ptr_pre;
		}
		ptr_pre->prev_next_info.le_next = ptr_next->prev_next_info.le_next;
		ptr_next->size=0;
		ptr_next->is_free=0;
		meta_free->size=0;
		meta_free->is_free=0;
	}

	//the block after it is free
	else if(meta_free->prev_next_info.le_next != NULL && meta_free->prev_next_info.le_next->is_free==1)
	{
		struct BlockMetaData* ptr_next = meta_free->prev_next_info.le_next;
		meta_free->size += meta_free->prev_next_info.le_next->size;
		if(ptr_next->prev_next_info.le_next != NULL)
		{
			ptr_next->prev_next_info.le_next->prev_next_info.le_prev = meta_free;
		}
		meta_free->is_free=1;
		ptr_next->size=0;
		ptr_next->is_free=0;
		meta_free->prev_next_info.le_next=ptr_next->prev_next_info.le_next;
	}

	//the block before it is free
	else if(meta_free->prev_next_info.le_prev != NULL && meta_free->prev_next_info.le_prev->is_free==1)
	{
		struct BlockMetaData* ptr_pre = meta_free->prev_next_info.le_prev;
		ptr_pre->size += meta_free->size;
		ptr_pre->prev_next_info.le_next = meta_free->prev_next_info.le_next;
		if(meta_free->prev_next_info.le_next!=NULL)
		{
			meta_free->prev_next_info.le_next->prev_next_info.le_prev = ptr_pre;
		}
		meta_free->size=0;
		meta_free->is_free=0;
	}
	else
		meta_free->is_free=1;
}
	//panic("free_block is not implemented yet");
//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void *va, uint32 new_size)
{
    if ((new_size == 0) && (va != NULL))
    {
        free_block(va);
        return NULL;
    }
    else if ((new_size != 0) && (va == NULL))
    {
        return alloc_block_FF(new_size);
    }
    else if ((new_size == 0) && (va == NULL))
    {
        return NULL;
    }

    struct BlockMetaData *current_Block = (struct BlockMetaData *)va - 1;
    uint32 current_size = current_Block->size - sizeOfMetaData();
    uint32 newSize_MetaSize = new_size + sizeOfMetaData();
    if (newSize_MetaSize == current_Block->size)
    {
        return va;
    }
    else if (newSize_MetaSize > current_Block->size) // increase size
    {
        uint32 req_size = newSize_MetaSize - current_Block->size;
        struct BlockMetaData *next_Block = (struct BlockMetaData *)((uint32)current_Block + current_Block->size);

        if ((next_Block->size >= req_size) && (next_Block->is_free == 1))
        {
            // Use the next block to fulfill the additional requested size
            if (next_Block->size >= req_size + sizeOfMetaData())
            {
                // Split the next block
                struct BlockMetaData *new_next_Block = (struct BlockMetaData *)((uint32)next_Block + req_size);
                new_next_Block->size = next_Block->size - req_size;
                new_next_Block->is_free = 1;
                LIST_INSERT_AFTER(&HEAP_List, current_Block, new_next_Block);
                current_Block->size = newSize_MetaSize;
            }
            else
            {
                // Use the entire next block
                current_Block->size += next_Block->size;
            }
            next_Block->size = 0;
            next_Block->is_free = 0;
            LIST_REMOVE(&HEAP_List, next_Block);
            return va;
        }
        else
        {
				void* returned = alloc_block_FF(new_size);
				 if(returned!= NULL)
				 {
				 	 memcpy(returned, va, current_size);
					 free_block(va);
					 return returned;
				 }
				else
				{
					struct BlockMetaData* sbrk_return = (struct BlockMetaData*)sbrk(newSize_MetaSize);
					if(sbrk_return!=(void*)-1)
					{
						struct BlockMetaData* new_Block = sbrk_return;
						new_Block->size = newSize_MetaSize;
						new_Block->is_free = 0;
						LIST_INSERT_TAIL(&HEAP_List, new_Block);
						free_block(va);
						return (struct BlockMetaData*)((uint32)sbrk_return + sizeOfMetaData());
					}
					else
					{
						return va;
					}
				}
        }
    }
    else // decrease size
    {
    	struct BlockMetaData *next_Block= current_Block->prev_next_info.le_next;
        uint32 remaining_size = current_Block->size - newSize_MetaSize;
        if(next_Block->is_free == 1)
        {
            struct BlockMetaData *new_Block = (struct BlockMetaData *)((uint32)current_Block + newSize_MetaSize);
            new_Block->size = remaining_size + next_Block->size;
            new_Block->is_free = 1;
            LIST_INSERT_AFTER(&HEAP_List, current_Block, new_Block);
            next_Block->is_free = 0;
            next_Block->size = 0;
            LIST_REMOVE(&HEAP_List, next_Block);
            current_Block->size = newSize_MetaSize;
        }
        else if (remaining_size >= sizeOfMetaData())
        {
            // Split the current block
            struct BlockMetaData *new_Block = (struct BlockMetaData *)((uint32)current_Block + newSize_MetaSize);
            new_Block->size = remaining_size;
            new_Block->is_free = 1;
            LIST_INSERT_AFTER(&HEAP_List, current_Block, new_Block);
            current_Block->size = newSize_MetaSize;
        }
        return va;
    }

    return NULL;
}
