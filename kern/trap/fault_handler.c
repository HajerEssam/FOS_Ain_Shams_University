/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;
//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif
		//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
		//refer to the project presentation and documentation for details
if(isPageReplacmentAlgorithmFIFO())
{
				//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
				// Write your code here, remove the panic and write your code
				//REPLACEMENT
			fault_va = ROUNDDOWN(fault_va , PAGE_SIZE);
			if(wsSize < (curenv->page_WS_max_size)) //PLACEMENT
			{
					struct FrameInfo* frame ;
					allocate_frame(&frame);
					map_frame(curenv->env_page_directory,frame, fault_va,(PERM_PRESENT | PERM_WRITEABLE | PERM_USER | PERM_AVAILABLE));
					int ret = pf_read_env_page(curenv, (void*)fault_va);
					if((ret == E_PAGE_NOT_EXIST_IN_PF) && !((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)))
					{
						unmap_frame(curenv->env_page_directory,fault_va);
						sched_kill_env(curenv->env_id);
					}
					struct WorkingSetElement* WSElement = env_page_ws_list_create_element(curenv , fault_va);
					LIST_INSERT_TAIL(&curenv->page_WS_list, WSElement);
					if (LIST_SIZE(&(curenv->page_WS_list)) == curenv->page_WS_max_size)
						curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));

			}
			else //REPLACEMENT
			{
				struct WorkingSetElement *deleted_ws = curenv->page_last_WS_element;
				uint32 perm = pt_get_page_permissions(curenv->env_page_directory,deleted_ws->virtual_address);
				uint32 *ptr_page_table = NULL;
				struct FrameInfo *frame = get_frame_info(curenv->env_page_directory,deleted_ws->virtual_address,&ptr_page_table);
				if((perm & PERM_MODIFIED) == PERM_MODIFIED)
				{
					pf_update_env_page(curenv, deleted_ws->virtual_address,frame);
				}
				map_frame(curenv->env_page_directory, frame, fault_va,  PERM_USER | PERM_PRESENT | PERM_WRITEABLE | PERM_AVAILABLE);
				env_page_ws_invalidate(curenv, deleted_ws->virtual_address);
				unmap_frame(curenv->env_page_directory,deleted_ws->virtual_address);

				int ret = pf_read_env_page(curenv, (void*)fault_va);
				if((ret == E_PAGE_NOT_EXIST_IN_PF) && !((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)))
				{
						unmap_frame(curenv->env_page_directory,fault_va);
						sched_kill_env(curenv->env_id);
				}
				struct WorkingSetElement *WSElement = env_page_ws_list_create_element(curenv,fault_va);
				if(curenv->page_last_WS_element == NULL)
				{
					curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
					//LIST_INSERT_TAIL(&(curenv->page_WS_list), WSElement);
				}
				//else
					LIST_INSERT_BEFORE(&(curenv->page_WS_list),curenv->page_last_WS_element,WSElement);

			}
}

if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
{
	//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
				uint32 sum = (LIST_SIZE(&(curenv->ActiveList)) + LIST_SIZE(&(curenv->SecondList)));
				if( sum < curenv->page_WS_max_size) //PLACEMENT
				{
						if(LIST_SIZE(&(curenv->ActiveList)) < curenv->ActiveListSize)
						{
							struct FrameInfo* frame ;
							allocate_frame(&frame);
							frame->va = fault_va;
							map_frame(curenv->env_page_directory,frame, fault_va,(PERM_PRESENT | PERM_WRITEABLE | PERM_USER | PERM_AVAILABLE));
							int ret = pf_read_env_page(curenv, (void*)fault_va);
							if((ret == E_PAGE_NOT_EXIST_IN_PF) && !((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)))
							{
								unmap_frame(curenv->env_page_directory,fault_va);
								sched_kill_env(curenv->env_id);
							}
							else
							{
								struct WorkingSetElement* WSElement = env_page_ws_list_create_element(curenv , fault_va);
								LIST_INSERT_HEAD(&curenv->ActiveList, WSElement);
							}
						}
						else if(LIST_SIZE(&(curenv->ActiveList)) == curenv->ActiveListSize && LIST_SIZE(&(curenv->SecondList)) < curenv->SecondListSize)
						{
							bool found = 0;
							struct WorkingSetElement* element;
							LIST_FOREACH(element , &curenv->SecondList)
							{
								if(ROUNDDOWN(element->virtual_address, PAGE_SIZE) == ROUNDDOWN(fault_va , PAGE_SIZE))
								{
									found = 1;
									struct WorkingSetElement* act_moved_ele = LIST_LAST(&curenv->ActiveList);
									struct WorkingSetElement* act_moved_ele1 = LIST_LAST(&curenv->ActiveList);
									struct WorkingSetElement* sec_moved_ele = element;
									LIST_REMOVE(&curenv->SecondList, element);
									pt_set_page_permissions(curenv->env_page_directory, sec_moved_ele->virtual_address , PERM_PRESENT , 0);
									pt_set_page_permissions(curenv->env_page_directory, LIST_LAST(&curenv->ActiveList)->virtual_address ,0 ,PERM_PRESENT);
									LIST_REMOVE(&curenv->ActiveList, act_moved_ele1);
									LIST_INSERT_HEAD(&curenv->SecondList , act_moved_ele);
									LIST_INSERT_HEAD(&curenv->ActiveList , sec_moved_ele);
								}
							}
							if(found == 0)
							{
								struct WorkingSetElement* moved_ele = LIST_LAST(&curenv->ActiveList);
								struct WorkingSetElement* moved_ele1 = LIST_LAST(&curenv->ActiveList);

								LIST_REMOVE(&curenv->ActiveList, moved_ele1);
								pt_set_page_permissions(curenv->env_page_directory, moved_ele->virtual_address , 0, PERM_PRESENT);
								LIST_INSERT_HEAD(&curenv->SecondList , moved_ele);
								uint32 *ptr_page_table=NULL;
								struct FrameInfo* frame1 = get_frame_info(ptr_page_directory, (uint32)fault_va, &ptr_page_table);
								if(frame1 != 0)
									unmap_frame(ptr_page_table , fault_va);

								struct FrameInfo* frame ;
								allocate_frame(&frame);
								frame->va = fault_va;
								map_frame(curenv->env_page_directory,frame, fault_va,(PERM_PRESENT | PERM_WRITEABLE | PERM_USER | PERM_AVAILABLE));
								int ret = pf_read_env_page(curenv, (void*)fault_va);
							if((ret == E_PAGE_NOT_EXIST_IN_PF) && !((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)))
							{
								unmap_frame(curenv->env_page_directory,fault_va);
								sched_kill_env(curenv->env_id);
							}
							else
							{
								struct WorkingSetElement* WSElement = env_page_ws_list_create_element(curenv , fault_va);
								pt_set_page_permissions(curenv->env_page_directory, fault_va ,PERM_PRESENT, 0);
								LIST_INSERT_HEAD(&curenv->ActiveList, WSElement);
							}
							}
						}
				}
				else //REPLACEMENT
				{
						//Case1 : the reguired page in the secound list
						bool found = 0;
						struct WorkingSetElement* element;
						LIST_FOREACH(element , &curenv->SecondList)
						{
							if(ROUNDDOWN(element->virtual_address, PAGE_SIZE) == ROUNDDOWN(fault_va , PAGE_SIZE))
							{
								found = 1;
								struct WorkingSetElement* act_moved_ele = LIST_LAST(&curenv->ActiveList);
								struct WorkingSetElement* act_moved_ele1 = LIST_LAST(&curenv->ActiveList);
								struct WorkingSetElement* sec_moved_ele = element;
								LIST_REMOVE(&curenv->SecondList, element);
								pt_set_page_permissions(curenv->env_page_directory, sec_moved_ele->virtual_address , PERM_PRESENT , 0);
								pt_set_page_permissions(curenv->env_page_directory, LIST_LAST(&curenv->ActiveList)->virtual_address ,0 ,PERM_PRESENT);
								LIST_REMOVE(&curenv->ActiveList, act_moved_ele1);
								LIST_INSERT_HEAD(&curenv->SecondList , act_moved_ele);
								LIST_INSERT_HEAD(&curenv->ActiveList , sec_moved_ele);
							}
						}
						//Case2 : the required page is on the disk
						if(found == 0)
						{
							int perm = pt_get_page_permissions(curenv->env_page_directory , curenv->SecondList.lh_last->virtual_address);
							if(perm && PERM_MODIFIED == PERM_MODIFIED)
							{
								uint32* ptr_page_table = NULL;
								struct FrameInfo * firstFrameInfo = get_frame_info(curenv->env_page_directory,curenv->SecondList.lh_last->virtual_address , &ptr_page_table);
								pf_update_env_page(curenv , curenv->SecondList.lh_last->virtual_address , firstFrameInfo);
							}
							env_page_ws_invalidate(curenv , curenv->SecondList.lh_last->virtual_address);
							struct WorkingSetElement* act_moved_ele = LIST_LAST(&curenv->ActiveList);
							struct WorkingSetElement* act_moved_ele1 = LIST_LAST(&curenv->ActiveList);
							pt_set_page_permissions(curenv->env_page_directory, act_moved_ele->virtual_address ,0 ,PERM_PRESENT);
							LIST_REMOVE(&curenv->ActiveList, act_moved_ele1);
							LIST_INSERT_HEAD(&curenv->SecondList , act_moved_ele);

							struct FrameInfo* frame ;
							allocate_frame(&frame);
							map_frame(curenv->env_page_directory,frame, fault_va,(PERM_PRESENT | PERM_WRITEABLE | PERM_USER | PERM_AVAILABLE));
							int ret = pf_read_env_page(curenv, (void*)fault_va);
						if((ret == E_PAGE_NOT_EXIST_IN_PF) && !((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)))
						{
							unmap_frame(curenv->env_page_directory,fault_va);
							sched_kill_env(curenv->env_id);
						}
						else
						{
							struct WorkingSetElement* WSElement = env_page_ws_list_create_element(curenv , fault_va);
							pt_set_page_permissions(curenv->env_page_directory, fault_va ,PERM_PRESENT, 0);
							LIST_INSERT_HEAD(&curenv->ActiveList, WSElement);
						}
					}
				}
				//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
			}
}
void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}



