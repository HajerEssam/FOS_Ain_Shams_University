#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

//struct pages module[NUM_OF_KHEAP_PAGES] ={0};
int module[NUM_OF_KHEAP_PAGES] ={0};
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	uint32 rounding = ROUNDUP(initSizeToAllocate,PAGE_SIZE);

	if(initSizeToAllocate + daStart <= daLimit)
	{
		Kernel_Heap_start = daStart;
		brk = (rounding + daStart);
		Hard_Limit = daLimit;
		uint32 ptr = daStart;
			while(ptr < brk)
			{
				struct FrameInfo* Returned_Alloc;
				int check_alloc = allocate_frame(&Returned_Alloc);
				if(check_alloc == 0)
				{
					Returned_Alloc->va= ptr;
					check_alloc = map_frame(ptr_page_directory,Returned_Alloc,ptr,PERM_PRESENT | PERM_WRITEABLE);
				}
					else
					return E_NO_MEM;

				ptr = ptr + PAGE_SIZE;
			}
			initialize_dynamic_allocator(daStart,initSizeToAllocate);
			return 0;
	}
	else
		return E_NO_MEM;
}
void* sbrk(int increment)
{
		if(increment == 0)
			return (void*) brk;

		int value = increment + brk;
		int value1 = ROUNDUP(value ,PAGE_SIZE);
		 uint32 Obrk = brk;
		 uint32 Rbrk = brk;
		 Obrk = ROUNDUP(Obrk , PAGE_SIZE);
		if(increment > 0)
		{
		 if (value1 > Hard_Limit )
		 {
		 	panic("\n above hard limit \n");
		 }else
		 {
			 brk = value1;
		 }
			uint32 NumberOfPages = value1 / PAGE_SIZE;
			for(int i = Obrk ; i < brk ; i+= PAGE_SIZE)
			{
				struct FrameInfo* Returned_Alloc;
				int ret = allocate_frame(&Returned_Alloc) ;
				if(ret == 0)
				{
					Returned_Alloc->va = i;
					ret = map_frame(ptr_page_directory,Returned_Alloc,i,PERM_PRESENT | PERM_WRITEABLE);
				}
				else
				{
					panic("No enough frames for incrementing sbrk");
				}
			}
				return (void*)Rbrk;
		}
		else if(increment < 0)
		{
			 if ((brk + increment) < Kernel_Heap_start)
			 {
			 	panic("\n brk should be above or aligned to kheap start \n");
			 }
			 else
			 {
				 brk = brk + increment;
			 }
				uint32 NumberOfPages = increment / PAGE_SIZE;
				for(int i = Obrk - PAGE_SIZE ; i >=  ROUNDUP(brk, PAGE_SIZE) ; i-= PAGE_SIZE)
				{
					uint32 *ptr_page_table =NULL;
					struct FrameInfo* frame = get_frame_info(ptr_page_directory,i, &ptr_page_table);
					if(frame != 0)
					{
						frame->va = 0;
						free_frame(frame);
						unmap_frame(ptr_page_directory , i);
					}
				}
					return (void*)brk;
		}
		return (void*)-1 ;
}

void* kmalloc(unsigned int size)
{
	if(isKHeapPlacementStrategyFIRSTFIT() && size <= (uint32)KERNEL_HEAP_MAX - Kernel_Heap_start)
	{
			if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
			{
				return alloc_block_FF(size);
			}
			else
			{
				uint32 rounding = ROUNDUP(size,PAGE_SIZE);
				uint32 NumberOfPages = rounding / PAGE_SIZE;
				uint32 found_pages = 0;
				uint32 startva;
				uint32 search = Hard_Limit + PAGE_SIZE;
				while(search < (uint32)KERNEL_HEAP_MAX)
				{
					if(found_pages == NumberOfPages)
					{
						break;
					}
					uint32 *ptr_page_table;
					struct FrameInfo* CheckIfNull = get_frame_info(ptr_page_directory,search,&ptr_page_table);
					if(CheckIfNull == 0)
					{
						if(found_pages == 0)
							startva = search;
						found_pages++;
					}
					else
					{
						found_pages = 0;
					}
					search += PAGE_SIZE;
				}
				if(found_pages == NumberOfPages)
				{
					uint32 search = startva;
					//saving info for free
					int index = (startva - KERNEL_HEAP_START) /PAGE_SIZE;
					module[index] = NumberOfPages;

					while(found_pages > 0)
					{
						struct FrameInfo* Returned_Alloc;
						allocate_frame(&Returned_Alloc) ;
						Returned_Alloc->va = search;
						map_frame(ptr_page_directory,Returned_Alloc,search,(int)(PERM_PRESENT | PERM_WRITEABLE));
						search += PAGE_SIZE;
						found_pages--;
					}
					return (void*)startva;
				}
			}
	}
		return NULL;
}
void kfree(void* virtual_address)
{
	if((uint32)virtual_address < Hard_Limit && (uint32)virtual_address >= KERNEL_HEAP_START)
	{
			free_block(virtual_address);
			return;
	}
	else if((uint32)virtual_address >= (Hard_Limit + PAGE_SIZE) && (uint32)virtual_address < KERNEL_HEAP_MAX)
	{
		bool found = 0;
		int index = ((uint32)virtual_address - KERNEL_HEAP_START) / PAGE_SIZE;
		if(module[index]> 0)
		{
			found = 1;
			while(module[index]!=0)
			{
				uint32 *ptr_page_table=NULL;
				struct FrameInfo* frame = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);
				frame->va = 0;
				free_frame(frame);
				unmap_frame(ptr_page_directory , (uint32)virtual_address);
				virtual_address += PAGE_SIZE;
				module[index]--;
			}
			module[index]=0;
			return;
		}
		if(found == 0)
		{
			panic("invalid virtual address\n");
			return;
		}
	}
	else
	{
		panic("invalid virtual address\n");
		return;
	}
}
//NEEDS WORK !!!!!!!!!
unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//cprintf("in kheap_virtual_address \n");
	struct FrameInfo *ptr_frame_info = to_frame_info(physical_address);
	uint32 offset = physical_address & 0x00000FFF;
	if(ptr_frame_info != NULL && ptr_frame_info->references == 1)
	{
		//cprintf("in if(ptr_frame_info != NULL && ptr_frame_info->references == 1)  OUT\n");
		return (unsigned int)((ptr_frame_info->va) + offset);
	}
	//cprintf("out kheap_virtual_address \n");
	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	unsigned int offset = virtual_address & 0x00000FFF;
		uint32 *ptr_page_table=NULL;
		get_page_table(ptr_page_directory,(uint32)virtual_address,&ptr_page_table);
		struct FrameInfo *ptr_frame_info = get_frame_info(ptr_page_directory,(uint32)virtual_address,&ptr_page_table);
        if(ptr_frame_info!=NULL)
        {
        	uint32 frame_number = to_frame_number(ptr_frame_info);
        	unsigned int physical_address=(frame_number*PAGE_SIZE)+offset;
        	return physical_address;
        }
		return 0;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	if((uint32)virtual_address < Hard_Limit && (uint32)virtual_address >= KERNEL_HEAP_START)
	{
		return realloc_block_FF(virtual_address , new_size);
	}
		else if((uint32)virtual_address >= (Hard_Limit + PAGE_SIZE) && (uint32)virtual_address < KERNEL_HEAP_MAX)
		{
			if ((new_size==0) && (virtual_address!=(void*)0))
			{
				kfree(virtual_address);
			}
			else if ((new_size!=0) && (virtual_address==(void*)0))
			{
				kmalloc(new_size);
			}
			else if ((new_size==0) && (virtual_address==NULL))
			{
				return NULL;
			}
				uint32 Total_Pages = ROUNDUP(new_size, PAGE_SIZE) / PAGE_SIZE;
				int index = ((uint32)virtual_address - KERNEL_HEAP_START) / PAGE_SIZE;
				uint32 req_pages = Total_Pages - module[index];
				if(module[index] == Total_Pages)
				{
					return virtual_address;
				}
				else if (Total_Pages > module[index])
				{
					uint32 *ptr_page_table =NULL;
					uint32 va = (uint32)virtual_address + (module[index]*PAGE_SIZE);
					struct FrameInfo* CheckIfNull;
					uint32 pages_found = 0;
					for(int i = 0 ; i < req_pages ; i++)
					{
						CheckIfNull = get_frame_info(ptr_page_directory,va,&ptr_page_table);
						if(CheckIfNull == 0) //the page is empty
							pages_found++;
						else
							break;

						va+=PAGE_SIZE;
					}
					if(pages_found == req_pages)
					{
						uint32 addva = (uint32)virtual_address + (module[index] * PAGE_SIZE);
						while(pages_found > 0)
						{
							struct FrameInfo* Returned_Alloc;
							allocate_frame(&Returned_Alloc) ;
							Returned_Alloc->va = addva;
							map_frame(ptr_page_directory,Returned_Alloc,addva,(int)(PERM_PRESENT | PERM_WRITEABLE));
							addva += PAGE_SIZE;
							pages_found--;
						}
						module[index] = Total_Pages;
						return virtual_address;
					}
					else //the pages after the va is not suitable for the desired required size
					{
						uint32* ret = kmalloc(new_size);
						if(ret != NULL)
						{
							memcpy( ret , virtual_address , (module[index]*PAGE_SIZE));
							kfree((uint32*)virtual_address);
							module[index]= 0;
							uint32 retindex = ((uint32)ret - KERNEL_HEAP_START) / PAGE_SIZE;
							module[retindex] = Total_Pages;
							return ret;
						}
						else //there isn't enough space for the new_size
							return NULL;
					}
				}
				else if (Total_Pages < module[index])
				{
					uint32 delva = (uint32)virtual_address + (Total_Pages * PAGE_SIZE);
					uint32 extra_pages = module[index] - Total_Pages;
					for(int i = 0 ; i < extra_pages ; i++)
					{
						uint32 *ptr_page_table=NULL;
						struct FrameInfo* frame = get_frame_info(ptr_page_directory, (uint32)delva, &ptr_page_table);
						frame->va = 0;
						free_frame(frame);
						unmap_frame(ptr_page_directory , (uint32)delva);
						delva += PAGE_SIZE;
					}
					module[index] = Total_Pages;
					return virtual_address;
				}

		}
	return NULL;
}
