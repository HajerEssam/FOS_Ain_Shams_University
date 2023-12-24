OS project 3rd year 1st semester

milestone 1 functions : 

  - str2lower
  - process_command
  - 3 system calls
  - params validation
    
  Dynamic Allocator :

     - initialize block allocator
     - alloc_block_FF
     - free_block
     - realloc_block_FF
     - alloc_block_FF
     - alloc_block_BF
        
milestone 2 functions :

  -Kernel Heap :
	
    - initialization
    - sbrk
    - kmalloc (First Fit)
    - kfree
    - kheap_virtual_address
    - kheap_physical_address
    - krealloc (First Fit)
  -Fault Handler :
	
    - env_page_ws_list_create_element
    - Check invalid pointers
    - Page_fault_handler
		
  -User Heap :
	
    - initialization
    - System call to get Limit
    - System call to get page permissions
    -  sys_sbrk
    -  malloc (First Fit)
    -  allocate_user_mem
    -  free
    -  free_user_mem
		
milestone 3 functions :

  -Fault Handler 2 :
	
    - FIFO Replacement
    - Approx LRU Placement
    - Approx LRU Replacement
		
  -BSD Scheduler :
	
    - System call to set nice
    - Getters and Setter
    - sched_init_BSD
    - fos_scheduler_BSD
    - clock_interrupt_handler

 
