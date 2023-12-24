#include "sched.h"

#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>
#include <inc/fixed_point.h>


uint32 isSchedMethodRR(){if(scheduler_method == SCH_RR) return 1; return 0;}
uint32 isSchedMethodMLFQ(){if(scheduler_method == SCH_MLFQ) return 1; return 0;}
uint32 isSchedMethodBSD(){if(scheduler_method == SCH_BSD) return 1; return 0;}

//===================================================================================//
//============================ SCHEDULER FUNCTIONS ==================================//
//===================================================================================//

//===================================
// [1] Default Scheduler Initializer:
//===================================
void sched_init()
{
	old_pf_counter = 0;

	sched_init_RR(INIT_QUANTUM_IN_MS);

	init_queue(&env_new_queue);
	init_queue(&env_exit_queue);
	scheduler_status = SCH_STOPPED;

}

//=========================
// [2] Main FOS Scheduler:
//=========================
void
fos_scheduler(void)
{
	//	cprintf("inside scheduler\n");

	chk1();
	scheduler_status = SCH_STARTED;

	//This variable should be set to the next environment to be run (if any)
	struct Env* next_env = NULL;

	if (scheduler_method == SCH_RR)
	{
		// Implement simple round-robin scheduling.
		// Pick next environment from the ready queue,
		// and switch to such environment if found.
		// It's OK to choose the previously running env if no other env
		// is runnable.

		//If the curenv is still exist, then insert it again in the ready queue
		if (curenv != NULL)
		{
			enqueue(&(env_ready_queues[0]), curenv);
		}

		//Pick the next environment from the ready queue
		next_env = dequeue(&(env_ready_queues[0]));

		//Reset the quantum
		//2017: Reset the value of CNT0 for the next clock interval
		kclock_set_quantum(quantums[0]);
		//uint16 cnt0 = kclock_read_cnt0_latch() ;
		//cprintf("CLOCK INTERRUPT AFTER RESET: Counter0 Value = %d\n", cnt0 );

	}
	else if (scheduler_method == SCH_MLFQ)
	{
		next_env = fos_scheduler_MLFQ();
	}
	else if (scheduler_method == SCH_BSD)
	{
		next_env = fos_scheduler_BSD();
	}
	//temporarily set the curenv by the next env JUST for checking the scheduler
	//Then: reset it again
	struct Env* old_curenv = curenv;
	curenv = next_env ;
	chk2(next_env) ;
	curenv = old_curenv;

	//sched_print_all();

	if(next_env != NULL)
	{
		//		cprintf("\nScheduler select program '%s' [%d]... counter = %d\n", next_env->prog_name, next_env->env_id, kclock_read_cnt0());
		//		cprintf("Q0 = %d, Q1 = %d, Q2 = %d, Q3 = %d\n", queue_size(&(env_ready_queues[0])), queue_size(&(env_ready_queues[1])), queue_size(&(env_ready_queues[2])), queue_size(&(env_ready_queues[3])));
		env_run(next_env);
	}
	else
	{
		/*2015*///No more envs... curenv doesn't exist any more! return back to command prompt
		curenv = NULL;
		//lcr3(K_PHYSICAL_ADDRESS(ptr_page_directory));
		lcr3(phys_page_directory);

		//cprintf("SP = %x\n", read_esp());

		scheduler_status = SCH_STOPPED;
		//cprintf("[sched] no envs - nothing more to do!\n");
		while (1)
			run_command_prompt(NULL);

	}
}

//=============================
// [3] Initialize RR Scheduler:
//=============================
void sched_init_RR(uint8 quantum)
{

	// Create 1 ready queue for the RR
	num_of_ready_queues = 1;
#if USE_KHEAP
	sched_delete_ready_queues();
	env_ready_queues = kmalloc(sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8)) ;
#endif
	quantums[0] = quantum;
	kclock_set_quantum(quantums[0]);
	init_queue(&(env_ready_queues[0]));

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_RR;
	//=========================================
	//=========================================
}

//===============================
// [4] Initialize MLFQ Scheduler:
//===============================
void sched_init_MLFQ(uint8 numOfLevels, uint8 *quantumOfEachLevel)
{
#if USE_KHEAP
	//=========================================
	//DON'T CHANGE THESE LINES=================
	sched_delete_ready_queues();
	//=========================================
	//=========================================

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_MLFQ;
	//=========================================
	//=========================================
#endif
}

//===============================
// [5] Initialize BSD Scheduler:
//===============================
void sched_init_BSD(uint8 numOfLevels, uint8 quantum)
{
#if USE_KHEAP

	sched_delete_ready_queues();
	env_ready_queues = kmalloc(sizeof(struct Env_Queue)* numOfLevels);
	num_of_ready_queues = numOfLevels;
	quantums = kmalloc(sizeof(uint8)) ;
	for (int i=0; i<numOfLevels; i++)
	{
		init_queue(&(env_ready_queues[i]));
	}
	quantums[0] = quantum;
	kclock_set_quantum(quantum);
	load =  fix_int(0);

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_BSD;
	//=========================================
	//=========================================
#endif

}



//=========================
// [6] MLFQ Scheduler:
//=========================
struct Env* fos_scheduler_MLFQ()
{
	panic("not implemented");
	return NULL;
}

//=========================
// [7] BSD Scheduler:
//=========================
struct Env* fos_scheduler_BSD()
{

		if(curenv!= NULL)
		    {
			enqueue(&env_ready_queues[curenv->priority],curenv);
			}
		int from_highest=num_of_ready_queues-1;
		while(from_highest>=0)
			{

				 if (!LIST_EMPTY(&(env_ready_queues[from_highest])))
				 {

					kclock_set_quantum(quantums[0]);
					return dequeue(&(env_ready_queues[from_highest]));

				 }
				 from_highest--;
			}

	load=fix_int(0);
	return NULL;
}
//========================================
// [8] Clock Interrupt Handler
//	  (Automatically Called Every Quantum)
//========================================
void clock_interrupt_handler()
{

    if(curenv!=NULL)
    	curenv->recent = fix_add(curenv->recent,fix_int(1));

	     if((ticks*quantums[0]) % 1000 == 0)
	     {
	    	 //======================load================================
	    	 //===========(59/60)*load+(1/60)*recent_cpu=================
	    	 fixed_point_t l1=fix_unscale(fix_int(59),60);//fraction 59/60
	    	 fixed_point_t l2=fix_unscale(fix_int(1),60);//fraction 1/60
	    	 fixed_point_t mul_1=fix_mul(l1,load);// load*(59/60)
	    	 int ready_process=0;
	    	 for(int i = num_of_ready_queues - 1 ; i >= 0; i--)
	    	 {
				 ready_process+=env_ready_queues[i].size;
	    	 }
	    	 if(curenv != NULL)
	    		 ++ready_process;
	    	 fixed_point_t ready_process_f=fix_int(ready_process);
	    	 fixed_point_t mul_2=fix_mul(l2,ready_process_f);//(1/60)*ready processes
	    	 load=fix_add(mul_1,mul_2);// adding the 2 terms
	    	 //==================recent_cpu calculation======================
	    	 //===============(((2*load)/(2*load+1))*recent_cpu)+nice========
	    	 fixed_point_t m1=fix_scale(load,2);
	    	 fixed_point_t add=fix_add(m1,fix_int(1));
	    	 fixed_point_t div=fix_div(m1,add);
	    	 for(int i =0 ; i <num_of_ready_queues; i++)
	    	 {
	    	     if (!LIST_EMPTY(&(env_ready_queues[i])))
	    	     {
	    	    	 struct Env* ptr=NULL;
	    	    	 LIST_FOREACH(ptr,&(env_ready_queues[i]))
	    	    	 {
	    	    		 fixed_point_t nice_fix=fix_int(ptr->nice_value);
	    	    		 ptr->recent = fix_add(fix_mul(div,ptr->recent),nice_fix);
	    	    	 }
	    	     }
	    	  }
    		 fixed_point_t nice_fix=fix_int(curenv->nice_value);
    		 curenv->recent = fix_add(fix_mul(div,curenv->recent),nice_fix);
	     }
	//==========================Priority======================================
	     if((ticks) % 4 == 0)
	     {
	    	 for(int i =0; i <num_of_ready_queues; i++)
	    	 {
	    		 if (!LIST_EMPTY(&(env_ready_queues[i])))
	    		 {
	    			 struct Env* ptr=NULL;
	    			 LIST_FOREACH(ptr,&(env_ready_queues[i]))
	    			 {
		    				int temp =(num_of_ready_queues-1)-(fix_trunc(fix_unscale(ptr->recent,4)))-(ptr->nice_value*2);
		    				if(temp > num_of_ready_queues - 1)
		    				{
		    					temp = num_of_ready_queues - 1;
		    				}
		    				if(temp<PRI_MIN)
		    				{
		    					temp=PRI_MIN;
		    				}
	                        if(ptr->priority!=temp)
	                        {
	                        	ptr->priority=temp;
	                            remove_from_queue(&(env_ready_queues[i]),ptr);
	                            enqueue(&(env_ready_queues[ptr->priority]),ptr);
	                        }

	    			 }
	    		 }

	    	 }
	    	 if(curenv!=NULL)
	    	 {
					int new_priority=(num_of_ready_queues-1)-(fix_trunc(fix_unscale(curenv->recent,4)))-(curenv->nice_value*2);
					if(new_priority>num_of_ready_queues-1)
						curenv->priority=num_of_ready_queues-1;

					if(new_priority<PRI_MIN)
						curenv->priority=PRI_MIN;
	    	 }

	     }
	/**DON'T CHANGE THIS LINE*/
	ticks++ ;
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX))
	{
		update_WS_time_stamps();
	}
	//cprintf("Clock Handler\n") ;
	fos_scheduler();
	/*****/
}
//===================================================================
// [9] Update LRU Timestamp of WS Elements
//	  (Automatically Called Every Quantum in case of LRU Time Approx)
//===================================================================
void update_WS_time_stamps()
{
	struct Env *curr_env_ptr = curenv;

	if(curr_env_ptr != NULL)
	{
		struct WorkingSetElement* wse ;
		{
			int i ;
#if USE_KHEAP
			LIST_FOREACH(wse, &(curr_env_ptr->page_WS_list))
			{
#else
			for (i = 0 ; i < (curr_env_ptr->page_WS_max_size); i++)
			{
				wse = &(curr_env_ptr->ptr_pageWorkingSet[i]);
				if( wse->empty == 1)
					continue;
#endif
				//update the time if the page was referenced
				uint32 page_va = wse->virtual_address ;
				uint32 perm = pt_get_page_permissions(curr_env_ptr->env_page_directory, page_va) ;
				uint32 oldTimeStamp = wse->time_stamp;

				if (perm & PERM_USED)
				{
					wse->time_stamp = (oldTimeStamp>>2) | 0x80000000;
					pt_set_page_permissions(curr_env_ptr->env_page_directory, page_va, 0 , PERM_USED) ;
				}
				else
				{
					wse->time_stamp = (oldTimeStamp>>2);
				}
			}
		}

		{
			int t ;
			for (t = 0 ; t < __TWS_MAX_SIZE; t++)
			{
				if( curr_env_ptr->__ptr_tws[t].empty != 1)
				{
					//update the time if the page was referenced
					uint32 table_va = curr_env_ptr->__ptr_tws[t].virtual_address;
					uint32 oldTimeStamp = curr_env_ptr->__ptr_tws[t].time_stamp;

					if (pd_is_table_used(curr_env_ptr->env_page_directory, table_va))
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2) | 0x80000000;
						pd_set_table_unused(curr_env_ptr->env_page_directory, table_va);
					}
					else
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2);
					}
				}
			}
		}
	}
}
