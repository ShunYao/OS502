/************************************************************************

        This code forms the base of the operating system you will
        build.  It has only the barest rudiments of what you will
        eventually construct; yet it contains the interfaces that
        allow test.c and z502.c to be successfully built together.

        Revision History:
        1.0 August 1990
        1.1 December 1990: Portability attempted.
        1.3 July     1992: More Portability enhancements.
                           Add call to sample_code.
        1.4 December 1992: Limit (temporarily) printout in
                           interrupt handler.  More portability.
        2.0 January  2000: A number of small changes.
        2.1 May      2001: Bug fixes and clear STAT_VECTOR
        2.2 July     2002: Make code appropriate for undergrads.
                           Default program start is in test0.
        3.0 August   2004: Modified to support memory mapped IO
        3.1 August   2004: hardware interrupt runs on separate thread
        3.11 August  2004: Support for OS level locking
	4.0  July    2013: Major portions rewritten to support multiple threads
************************************************************************/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             "z502.h"
#include             "syao_global.h"



// These loacations are global and define information about the page table
extern UINT16        *Z502_PAGE_TBL_ADDR;
extern INT16         Z502_PAGE_TBL_LENGTH;

extern void          *TO_VECTOR [];

char                 *call_names[] = { "mem_read ", "mem_write",
                            "read_mod ", "get_time ", "sleep    ",
                            "get_pid  ", "create   ", "term_proc",
                            "suspend  ", "resume   ", "ch_prior ",
                            "send     ", "receive  ", "disk_read",
                            "disk_wrt ", "def_sh_ar" };
INT32                PCBSTRARTID=0;
INT32                totalTimerPid=0;
INT32				 totalReadyPid=0;
INT32				 totalMessage=0;
INT32                TimerStatusTime=-1;
S_PCB          *Test;



/************************************************************************
    INTERRUPT_HANDLER
        When the Z502 gets a hardware interrupt, it transfers control to
        this routine in the OS.
************************************************************************/
void    interrupt_handler( void ) {
    INT32              device_id;
    INT32              status;
	INT32			   Time;
    INT32              Index = 0;
    static BOOL        remove_this_in_your_code = TRUE;   /** TEMP **/
    static INT32       how_many_interrupt_entries = 0;    /** TEMP **/
    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );
	
	switch(device_id)
	{
			case TIMER_ITR:
			CALL(MEM_READ(Z502ClockStatus, &Time));
			while(timerHead!=NULL)
			{
				if(Time>(timerHead->p_time))	
				{	
					printf("Interrupt_handler: Found device ID %d\n",timerHead->p_id);
					lockTimer();
					//overTimeModify();
					makeTimerToReady(timerHead);
					addToReadyQ(makeTimer);
					unlockTimer();
				}
			
			    else
				break;
			}
			break;
	}
    MEM_WRITE(Z502InterruptClear, &Index );
}                                       /* End of interrupt_handler */
/************************************************************************
    FAULT_HANDLER
        The beginning of the OS502.  Used to receive hardware faults.
************************************************************************/

void    fault_handler( void )
    {
    INT32       device_id;
    INT32       status;
    INT32       Index = 0;

    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );

    printf( "Fault_handler: Found vector type %d with value %d\n",
                        device_id, status );
    // Clear out this device - we're done with it
    MEM_WRITE(Z502InterruptClear, &Index );
}                                       /* End of fault_handler */

INT32   os_create_process(char *name, void *test_name, INT32 initial_prriority,INT32 *pid_ptr,INT32 *p_error_prt, INT32 status)
{   

	S_PCB *PCB_PTR = NULL;

	if(initial_prriority == ILLEGAL)
	{
		*p_error_prt = ERR_BAD_PARAM;
		return -1;
	}

	if(readyHead != NULL)
	{
		S_PCB *temp =readyHead;
		while(temp != NULL)
		{
			if(strcmp(temp->p_name, name) == 0)
		{
			printf("same name error");
			(*p_error_prt) = ERR_BAD_PARAM;
			return -1;
			
		}
		else
		{
			temp = temp->next;
		}
		
		}
	}

	if(timerHead != NULL)
	{
		S_PCB *temp =timerHead;
		while(temp != NULL)
		{
			if(strcmp(temp->p_name, name) == 0)
		{
			printf("same name error");
			(*p_error_prt) = ERR_BAD_PARAM;
			return -1;
			
		}
		else
		{
			temp = temp->next;
		}
		
		}}
	//check total pid
	(*p_error_prt) = ERR_BAD_PARAM;
	PCB_PTR = (S_PCB*)(malloc(sizeof(S_PCB)));
	PCB_PTR-> p_time = 0;
	PCB_PTR->p_status = status;
	PCB_PTR->p_position = CREATE;
	PCB_PTR->p_id = PCBSTRARTID;
	(*pid_ptr) = PCBSTRARTID;
	PCBSTRARTID++;
	PCB_PTR->p_time = 0;
	PCB_PTR->p_priority = initial_prriority;
	strcpy(PCB_PTR->p_name, name);
	if(PCBSTRARTID >MAXPROCESS)
	{
		*p_error_prt = TOOMUCHPROCESSES;
		return -1;
	}
	make_context(PCB_PTR, test_name);
	if(status == RUN)
	{
    addToReadyQ(PCB_PTR); 
	Dispatcher();
	}
	else
	{
		addToReadyQ(PCB_PTR);
		*p_error_prt = ERR_SUCCESS;
	}

	return 0;
}

INT32 os_get_process_id(char *name, INT32 *pid_ptr, INT32 *p_error_prt)
{
	S_PCB *temp;
	if(*name == NULL) 
	{
		*pid_ptr = current_PCB_PTR->p_id;
	    *p_error_prt = ERR_SUCCESS;
		return 0;
	}
	
	temp = timerHead;
	while(temp!= NULL)
	{
		if(strcmp(temp->p_name,name) ==0)
		{
			*pid_ptr = temp->p_id;
			*p_error_prt = ERR_SUCCESS;
			return 0;
		}
		else
		{
			temp = temp->next;
		}
	}
	temp =readyHead;
	while(temp != NULL)
	{
		if(strcmp(temp->p_name,name) ==0)
		{
			*pid_ptr = temp->p_id;
			*p_error_prt =ERR_SUCCESS;
			return 0;
		}
		else
		{
			temp = temp->next;
		}
	}
	temp =waitingMegPCBHead;
	while(temp != NULL)
	{
		if(strcmp(temp->p_name,name) ==0)
		{
			*pid_ptr = temp->p_id;
			*p_error_prt =ERR_SUCCESS;
			return 0;
		}
		else
		{
			temp = temp->next;
		}
	}
	temp =suspendHead;
	while(temp != NULL)
	{
		if(strcmp(temp->p_name,name) ==0)
		{
			*pid_ptr = temp->p_id;
			*p_error_prt =ERR_SUCCESS;
			return 0;
		}
		else
		{
			temp = temp->next;
		}
	}

	
	*p_error_prt = -10;
	return -1;
}

void make_context (S_PCB *PCB_PRT, void *procPTR )
{

	CALL(Z502MakeContext(&PCB_PRT->context, procPTR, USER_MODE ));
}

void switch_context (S_PCB *PCB_PTR )
{
	CALL(Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &PCB_PTR->context ));
} 

void switch_context_kill (S_PCB *PCB_PTR )
{
	CALL(Z502SwitchContext( SWITCH_CONTEXT_KILL_MODE, &PCB_PTR->context ));
}
INT32 addToTimerQ(S_PCB * enter) 
{
   S_PCB *temp=NULL;
   S_PCB *prev=NULL;

   enter->p_position=SLEEP;                              //update state to sleep
	
   //First one into queue
	if (timerHead  == NULL)
    {
         timerHead = enter;
	     timerTail = enter;
		 enter->next =NULL;
         totalTimerPid++;
	}

	
	else
	{   
		temp=timerHead;
		    
			while(temp!=NULL)
			{ 
			  if(enter->p_time<temp->p_time)        
				{
				 if(temp==timerHead)
					{
					  timerHead=enter;
					   enter->next=temp;
					   totalTimerPid++;
					   break;
					 }
				  else
					 {
					   prev->next=enter;
					   enter->next=temp;
					   totalTimerPid++;
					   break;
					 }
				  }
			   else
				  {
				  prev=temp;
				  temp=temp->next;
				  }
			}
		if(temp == NULL)
		{
			timerTail->next = enter;
			timerTail = enter;
			enter->next = NULL;
			totalTimerPid++;

		}
	}
	return 1;
}

INT32 addToReadyQ(S_PCB * enter) 
{
	S_PCB *temp=NULL;
	S_PCB *prev=NULL;

	if(readyHead == NULL)
	{
			readyHead = readyTail = enter;
			enter->next =NULL;
		    totalReadyPid++;
	}
	else
	{
		temp=readyHead;
		    
			while(temp!=NULL)
			{ 
			  if(enter->p_priority<temp->p_priority)        
				{
				 if(temp==readyHead)
					{
					   readyHead=enter;
					   enter->next=temp;
					   totalReadyPid++;
					   break;
					 }
				  else
					 {
					   prev->next=enter;
					   enter->next=temp;
					   totalReadyPid++;
					   break;
					 }
				  }
			   else
				  {
				  prev=temp;
				  temp=temp->next;
				  }
			}
		if(temp == NULL)
		{
			prev->next = enter;
			readyTail = enter;
			enter->next = NULL;
			totalReadyPid++;

		}
	}
	return 1;
}

void  Dispatcher(){

		while(readyHead == NULL && timerHead!= NULL)
	    {    
		   CALL(Z502Idle());
		}
		if(readyHead!=NULL)
		{

			    schedular_printer(READY);
				lockReady();
				CALL(makeReadyToRun(readyHead));
				current_PCB_PTR=makeReady;
				current_PCB_PTR->p_status = RUNNING;
				unlockReady();
				CALL(switch_context(current_PCB_PTR));
		}
 return;
}
//this remove the head of the readyQ from readyQ and set makeReady as it
void makeReadyToRun(S_PCB *enter_ptr)
{
	if(enter_ptr==NULL)
	{
		makeReady=NULL;    
		printf("Nothing can be dropped to CPU");
	}
	else if(readyHead->next==NULL)  
	{

		makeReady = enter_ptr;
		makeReady->next=NULL;
		readyHead=NULL;
		readyTail=NULL;
		totalReadyPid--;           
	}
	else 
	{
		makeReady = readyHead;
		readyHead = readyHead->next;
		makeReady->next=NULL;
		totalReadyPid--;           
	}
}

//this remove the head of the timerQ from timerQ and set makeTimer as it
//this can be used with addToReadyQ to move the dropped timerPCB to readyQ
void makeTimerToReady(S_PCB *enter_ptr)	
{	INT32       Time;

	CALL(MEM_READ( Z502ClockStatus, &Time ));
	if(enter_ptr==NULL)
	{
		makeTimer=NULL; 
		printf("Nothing can be dropped to Ready");
	}
	else if(enter_ptr->next==NULL)  
	{
		makeTimer = enter_ptr;
		makeTimer->next=NULL;
		timerHead=NULL;
		timerTail=NULL;
		totalTimerPid--;           
	}
	else 
	{
		makeTimer = enter_ptr;
		timerHead = timerHead->next;
		makeTimer->next = NULL;
		totalTimerPid--;
		if(Time<timerHead->p_time)
		{
			start_timer(timerHead->p_time-Time);
			TimerStatusTime = timerHead->p_time;	
		}
	}
}
/************************************************************************
    SVC
        The beginning of the OS502.  Used to receive software interrupts.
        All system calls come to this point in the code and are to be
        handled by the student written code here.
        The variable do_print is designed to print out the data for the
        incoming calls, but does so only for the first ten calls.  This
        allows the user to see what's happening, but doesn't overwhelm
        with the amount of data.
************************************************************************/

void    svc( SYSTEM_CALL_DATA *SystemCallData ) {
    short               call_type;
    static short        do_print = 10;
    short               i;
	INT32				Time;
	static long         SleepTimer;
	INT32               atLock = -1;


    call_type = (short)SystemCallData->SystemCallNumber;
    if ( do_print > 0 ) {
        printf( "SVC handler: %s\n", call_names[call_type]);
        for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++ ){
        	 //Value = (long)*SystemCallData->Argument[i];
             printf( "Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
             (unsigned long )SystemCallData->Argument[i],
             (unsigned long )SystemCallData->Argument[i]);
        }
		do_print--;
	}

	switch (call_type){
		//get time service call
		case SYSNUM_GET_TIME_OF_DAY:  //This value is found in syscalls.h
			CALL(MEM_READ( Z502ClockStatus, &Time ));
			*SystemCallData->Argument[0] = Time;

			break;
		//terminate system call
		case SYSNUM_TERMINATE_PROCESS:
			CALL(MEM_READ( Z502ClockStatus, &Time ));
			if(SystemCallData->Argument[0] == -1)
			{ 
				if(current_PCB_PTR->p_id==0)  
				{
					CALL(Z502Halt());  
					break;
				}	

				else
					if(timerHead != NULL)
					{
						if(Time>TimerStatusTime)
							{
								start_timer(5);
								CALL(Dispatcher());
								break;
							}
						else
						CALL(Dispatcher());
						break;
					}
					else{
					CALL(Dispatcher());
					break;}
			

                break;
			}
			else if(SystemCallData->Argument[0] == -2)
			{
			*SystemCallData->Argument[1] = ERR_SUCCESS;
			CALL(Z502Halt());
			break;
			}
			else
			{
				S_PCB *temp = readyHead;
				S_PCB *prev = NULL;
				while(temp != NULL)
				{
					if(temp->p_id ==SystemCallData->Argument[0])
					{
						if(temp->next == NULL)
						{
							*SystemCallData->Argument[1] = ERR_SUCCESS;
							break;
						}
						else
						{
							*SystemCallData->Argument[1] = ERR_SUCCESS;
							prev->next = temp->next;
							break;
						}
					}
					else
					{
						prev = temp;
						temp = temp->next;
						*SystemCallData->Argument[1] = ERR_SUCCESS;
						break;
					}

				}
				if(temp ==  NULL)
				{
					*SystemCallData->Argument[1] = CANNOTTERMINATE ;
				}

			}
			break;
		case SYSNUM_SLEEP:
			//overTimeModify();
			schedular_printer(SLEEP);
		    SleepTimer = SystemCallData->Argument[0];
			CALL(MEM_READ( Z502ClockStatus, &Time ));
			current_PCB_PTR->p_time = Time + SleepTimer;
			lockTimer();
			CALL(addToTimerQ(current_PCB_PTR));
			unlockTimer();
			if(Time<(timerHead->p_time))        //compare the wake time with current time 
                    {
                    CALL(start_timer(SleepTimer));
                    CALL(Dispatcher());
                    }
                else{                  //if wake time is smaller than now, it need wake up immedately
                    CALL(start_timer(5));
                    CALL(Dispatcher());
                    }
			break;
		case SYSNUM_CREATE_PROCESS:
			os_create_process(SystemCallData->Argument[0], SystemCallData->Argument[1],SystemCallData->Argument[2],SystemCallData->Argument[3],SystemCallData->Argument[4],NOTRUN);
			break;
		case SYSNUM_GET_PROCESS_ID:
			os_get_process_id(SystemCallData->Argument[0], SystemCallData->Argument[1],SystemCallData->Argument[2]);
			break;
		case SYSNUM_SUSPEND_PROCESS:
			os_suspend_process(SystemCallData->Argument[0], SystemCallData->Argument[1]);
			break;
		case SYSNUM_RESUME_PROCESS:
			os_resume_process(SystemCallData->Argument[0], SystemCallData->Argument[1]);
			break;
		case SYSNUM_CHANGE_PRIORITY:
			os_change_priority(SystemCallData->Argument[0], SystemCallData->Argument[1],SystemCallData->Argument[2]);
			break;
		case SYSNUM_SEND_MESSAGE:
			os_send_message(SystemCallData->Argument[0], SystemCallData->Argument[1],SystemCallData->Argument[2],SystemCallData->Argument[3]);
			break;
		case SYSNUM_RECEIVE_MESSAGE:
			os_receive_message(SystemCallData->Argument[0], SystemCallData->Argument[1],SystemCallData->Argument[2],SystemCallData->Argument[3],SystemCallData->Argument[4],SystemCallData->Argument[5]);
			break;
		default:
			printf( "ERROR! Call type not reconginzed!\n");
			printf( "Call_type is - %i\n", call_type);
	}
}
void   start_timer(INT32 timer_time)
{	INT32          Time;
    INT32          SleepTime;

	CALL(MEM_READ( Z502ClockStatus, &Time ));
	
	CALL(MEM_WRITE( Z502TimerStart, &timer_time));
	TimerStatusTime = Time+timer_time;

}	


/************************************************************************
    osInit
        This is the first routine called after the simulation begins.  This
        is equivalent to boot code.  All the initial OS components can be
        defined and initialized here.
************************************************************************/

void    osInit( int argc, char *argv[]  ) {
    void                *next_context;
    INT32               i;
	void         		*process_ptr;
	INT32               j;

    /* Demonstrates how calling arguments are passed thru to here       */

    printf( "Program called with %d arguments:", argc );
    for ( i = 0; i < argc; i++ )
        printf( " %s", argv[i] );
    printf( "\n" );
    printf( "Calling with argument 'sample' executes the sample program.\n" );

    /*          Setup so handlers will come to code in base.c           */

    TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR]   = (void *)interrupt_handler;
    TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR] = (void *)fault_handler;
    TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR]  = (void *)svc;

    /*  Determine if the switch was set, and if so go to demo routine.  */

    if (( argc > 1 ) && ( strcmp( argv[1], "sample" ) == 0 ) ) {
        Z502MakeContext( &next_context, (void *)sample_code, KERNEL_MODE );
        Z502SwitchContext( SWITCH_CONTEXT_KILL_MODE, &next_context );
    }                   /* This routine should never return!!           */
	
    /*  This should be done by a "os_make_process" routine, so that
        test0 runs on a process recognized by the operating system.    */
	else if(( argc > 1 ) && ( strcmp( argv[1], "test1a" ) == 0 ) )
		{
			process_ptr = test1a;
			os_create_process("test1a", process_ptr, LEGAL,&j, &j,RUN);
		} 
	
	else if(( argc > 1 ) && ( strcmp( argv[1], "test1b" ) == 0 ) )
		{
			process_ptr = test1b;
			os_create_process("test1b", process_ptr, LEGAL ,&j, &j,RUN);

		}
	else if(( argc > 1 ) && ( strcmp( argv[1], "test1c" ) == 0 ) )
		{
		
			process_ptr = test1c;
			os_create_process("test1c", process_ptr, LEGAL ,&i, &j,RUN);
		}
	else if(( argc > 1 ) && ( strcmp( argv[1], "test1d" ) == 0 ) )
		{
			process_ptr = test1d;
			os_create_process("test1d", process_ptr, LEGAL ,&i, &j,RUN);
		}
	else if(( argc > 1 ) && ( strcmp( argv[1], "test1e" ) == 0 ) )
	{
		process_ptr = test1e;
		os_create_process("test1e", process_ptr, LEGAL ,&i, &j,RUN);
	}
	else if(( argc > 1 ) && ( strcmp( argv[1], "test1f" ) == 0 ) )
	{
		process_ptr = test1f;
		os_create_process("test1f", process_ptr, LEGAL ,&i, &j,RUN);
	}
	else if(( argc > 1 ) && ( strcmp( argv[1], "test1g" ) == 0 ) )
	{
		process_ptr = test1g;
		os_create_process("test1g", process_ptr, LEGAL ,&i, &j,RUN);
	}
	else if(( argc > 1 ) && ( strcmp( argv[1], "test1h" ) == 0 ) )
	{
		process_ptr = test1h;
		os_create_process("test1h", process_ptr, LEGAL ,&i, &j,RUN);
	}
	else if(( argc > 1 ) && ( strcmp( argv[1], "test1i" ) == 0 ) )
	{
		process_ptr = test1i;
		os_create_process("test1i", process_ptr, LEGAL ,&i, &j,RUN);
	}
	else if(( argc > 1 ) && ( strcmp( argv[1], "test1j" ) == 0 ) )
	{
		process_ptr = test1j;
		os_create_process("test1j", process_ptr, LEGAL ,&i, &j,RUN);
	}
	else 
	{
		process_ptr = test1l;
		os_create_process("test1l", process_ptr, LEGAL ,&i, &j,RUN);
	}
}

INT32  schedular_printer(INT32 action)
{
	    INT32            Time;
		INT32            spLock;

		CALL(MEM_READ( Z502ClockStatus, &Time ));
		if(current_PCB_PTR == NULL)
		{
		if(action == SLEEP)
			printf("Time: %d   Target: %d   Next Action: SLEEP \n",Time);
		else if(action == READY)
		printf("Time: %d   Next Action: Make ReadyHead To Run \n",Time);
		}
		else
		{
			if(action == SLEEP)
				printf("Time: %d   Target: %d   Next Action: SLEEP \n",Time, current_PCB_PTR->p_id);
		    if(action == READY)
				printf("Time: %d   Target: %d   Next Action: Make ReadyHead To Run \n",Time,readyHead->p_id);
			if(action == RESUSPEND)
				printf("Time: %d   Target: %d   Next Action: RESUME SUSPEND \n",Time, resumeSuspend->p_id);
			if(action == SUSPEND)
				printf("Time: %d   Target:  Next Action: SUSPEND \n",Time);
			if(action == PRIORITY)
				printf("Time: %d   Target: %d   Next Action: PRIORITY \n",Time,changePriority->p_id);
		}
		lockSP();	
		printf("Ready:");
		if(readyHead != NULL)
		{
			S_PCB *temp;
			temp = readyHead;
			while(temp != NULL)
			{
				printf("%d  %d ",temp->p_id, temp->p_priority);
				temp = temp->next;
			}
			printf("\n");
		}
		else
			printf("\n");

	printf("Timer:");
		if(timerHead != NULL)
		{
			S_PCB *temp;
			temp = timerHead;
			while(temp != NULL)
			{
				printf("%d  %d ",temp->p_id, temp->p_priority);
				temp = temp->next;
			}
			printf("\n \n");
		}
		else
			printf("\n \n");

		unlockSP();

	return 1;
}

//Timer Locks
void  lockTimer( void ){
	INT32 LockResult;
	READ_MODIFY( MEMORY_INTERLOCK_BASE, DO_LOCK, TRUE, &LockResult );
}
void  unlockTimer ( void ){
	INT32 LockResult;
	READ_MODIFY( MEMORY_INTERLOCK_BASE, DO_UNLOCK, TRUE, &LockResult );
}

//Ready Locks
void  lockReady(void){
	INT32 LockResult;
	READ_MODIFY( MEMORY_INTERLOCK_BASE + 1, DO_LOCK, TRUE, &LockResult );
}
void  unlockReady(void){
	INT32 LockResult;
	READ_MODIFY( MEMORY_INTERLOCK_BASE + 1, DO_UNLOCK, TRUE, &LockResult );
}

void  lockSP(void){
	INT32 LockResult;
	READ_MODIFY( MEMORY_INTERLOCK_BASE + 2, DO_LOCK, TRUE, &LockResult );
}

void  unlockSP(void){
	INT32 LockResult;
	READ_MODIFY( MEMORY_INTERLOCK_BASE + 2, DO_UNLOCK, TRUE, &LockResult );
}

void  lockSuspend(void){
	INT32 LockResult;
	READ_MODIFY( MEMORY_INTERLOCK_BASE + 3, DO_LOCK, TRUE, &LockResult );
}
void  unlockSuspend(void){
	INT32 LockResult;
	READ_MODIFY( MEMORY_INTERLOCK_BASE + 3, DO_UNLOCK, TRUE, &LockResult );
}
void  lockpriority(void){
	INT32 LockResult;
	READ_MODIFY( MEMORY_INTERLOCK_BASE + 4, DO_LOCK, TRUE, &LockResult );
}
void  unlockpriority(void){
	INT32 LockResult;
	READ_MODIFY( MEMORY_INTERLOCK_BASE + 4, DO_UNLOCK, TRUE, &LockResult );
}




void  print_queues (void){
	printf("Ready:");
	if(readyHead != NULL)
		{
			S_PCB *temp;
			temp = readyHead;
			while(temp != NULL)
			{
				printf("%d  P: %d",temp->p_id, temp->p_priority);
				temp = temp->next;
			}
			printf("\n");
		}
		else
			printf("\n");

	printf("Timer:");
		if(timerHead != NULL)
		{
			S_PCB *temp;
			temp = timerHead;
			while(temp != NULL)
			{
				printf("%d  %d",temp->p_id, temp->p_time);
				temp = temp->next;
			}
			printf("\n");
		}
		else
			printf("\n");

	}
void  addToSuspendQ(S_PCB  *enter)
{
	if (suspendHead == NULL)
		{
			 suspendHead = enter;
			 suspendTail = enter;
			 enter->next =NULL;
		}
	else
		{
			suspendTail->next = enter;
			enter->next = NULL;
		}
}

void  makeReadyToSuspend(INT32 process_id,INT32   *error_ptr)
{
	S_PCB          *temp=NULL;
	S_PCB          *prev=NULL;
	
	temp = readyHead;
	*error_ptr = ERROR;
	while(temp!=NULL)
	{
		if(temp->p_id == process_id)
		{
			if(temp==readyHead && temp->next != NULL)
			{
				readyHead = temp->next;
				temp->next = NULL;
				makeSuspend = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
			else if((temp==readyHead && temp->next == NULL))
			{
				readyHead = readyTail = NULL;
				temp->next = NULL;
				makeSuspend = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
			else
			{
				prev->next = temp->next;
				temp->next = NULL;
				makeSuspend = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
		}
		prev = temp;
		temp = temp->next;
	}

}  

void  makeWaitngMegPCBToSuspend(INT32 process_id,INT32 *error_ptr)
{
	S_PCB          *temp=NULL;
	S_PCB          *prev=NULL;
	
	temp = waitingMegPCBHead;
	*error_ptr = ERROR;
	while(temp!=NULL)
	{
		if(temp->p_id == process_id)
		{
			if(temp==waitingMegPCBHead && temp->next != NULL)
			{
				waitingMegPCBHead = temp->next;
				temp->next = NULL;
				makeWaitngMegPCBToS = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
			else if((temp==waitingMegPCBHead && temp->next == NULL))
			{
				waitingMegPCBHead = waitingMegPCBTail = NULL;
				temp->next = NULL;
				makeWaitngMegPCBToS = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
			else if((temp->next == NULL))
			{
				waitingMegPCBTail = prev;
				waitingMegPCBTail->next = NULL;
				makeWaitngMegPCBToS = temp;
				*error_ptr = ERR_SUCCESS;
				break;

			}
			else
			{
				prev->next = temp->next;
				temp->next = NULL;
				makeWaitngMegPCBToS = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
		}
		prev = temp;
		temp = temp->next;
	}

} 

void  resumeSuspendToReady(INT32 process_id,INT32   *error_ptr)
{
	S_PCB          *temp=NULL;
	S_PCB          *prev=NULL;
	
	temp = suspendHead;
	*error_ptr = ERROR;

	while(temp!=NULL)
	{
		if(temp->p_id == process_id)
		{
			if(temp==suspendHead && temp->next != NULL)
			{
				suspendHead = temp->next;
				temp->next = NULL;
				resumeSuspend = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
			else if((temp==suspendHead && temp->next == NULL))
			{
				suspendHead = suspendTail = NULL;
				temp->next = NULL;
				resumeSuspend = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
			else
			{
				prev->next = temp->next;
				temp->next = NULL;
				resumeSuspend = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
		}
		prev = temp;
		temp = temp->next;
	}

}         
void  os_suspend_process(INT32 process_id, INT32   *error_ptr)
{	
	
	if(process_id == -1)
	{
		if((timerHead == NULL && readyHead == NULL))
		{
			*error_ptr = ERROR;
			printf("Suspend the Only Process.\n");
		}		
		else
		{
			makeSuspend = current_PCB_PTR;
			lockSuspend();
			addToSuspendQ(makeSuspend);
			unlockSuspend();
			*error_ptr = ERR_SUCCESS;
			Dispatcher();
		}
	}
	else if(process_id != -1)
	{
		makeReadyToSuspend(process_id,error_ptr);
		if(*error_ptr != ERROR)
		{
			schedular_printer(SUSPEND);
			addToSuspendQ(makeSuspend);
			*error_ptr = ERR_SUCCESS;
		}
	}

	{
		makeWaitngMegPCBToSuspend(process_id,error_ptr);
		if(*error_ptr != ERROR)
		{
			schedular_printer(SUSPEND);
			makeWaitngMegPCBToS->p_meg_status = CHANGE;
			addToSuspendQ(makeWaitngMegPCBToS);
			change_waiting_meg_status(makeWaitngMegPCBToS->p_id);
			*error_ptr = ERR_SUCCESS;
		}
	}
}

INT32  os_resume_process(INT32 process_id, INT32   *error_ptr)
{
	S_MEG              *temp;
	S_MEG              *temp2;
	S_MEG              *prev;
	S_MEG              *prev2;

	lockSuspend();
	resumeSuspendToReady(process_id,error_ptr);
	if(*error_ptr != ERROR && resumeSuspend->p_meg_status != CHANGE)
	{
		addToReadyQ(resumeSuspend);
		schedular_printer(RESUSPEND);
		*error_ptr = ERR_SUCCESS;
	}
	else
	{
		resumeSuspend->p_meg_status = NULL;
		change_waiting_meg_status(resumeSuspend->p_id);
		addToWaitingMegPCBQ(resumeSuspend);
		schedular_printer(RESUSPEND);
		*error_ptr = ERR_SUCCESS;
		
	}
	unlockSuspend();
	return 0;
}


INT32  os_change_priority(INT32 process_id, INT32 new_priority, INT32   *error_ptr)
{
	S_PCB          *temp=NULL;
	S_PCB          *prev=NULL;
	lockpriority();
	if(new_priority>888)
	{
		error_ptr = ERROR;
		return -1;
	}
	if(process_id == -1)
	{
		current_PCB_PTR->p_priority = new_priority;
	}
	else
	{
		*error_ptr = ERROR;

		temp = timerHead;
		while(temp!=NULL)
		{
			if(temp->p_id == process_id)
			{temp->p_priority = new_priority;
			*error_ptr = ERR_SUCCESS;
			break;}
			else
				temp=temp->next;
		}
		
		temp = readyHead;
		

		if(*error_ptr!=ERR_SUCCESS)
		{
			while(temp!=NULL)
			{
				if(temp->p_id == process_id)
				{
					if(temp==readyHead && temp->next != NULL)
					{
						readyHead = temp->next;
						temp->next = NULL;
						temp->p_priority = new_priority;
						changePriority = temp;
						*error_ptr = ERR_SUCCESS;
						break;
					}
					else if((temp==readyHead && temp->next == NULL))
					{
						readyHead = readyTail = NULL;
						temp->next = NULL;
						temp->p_priority = new_priority;
						changePriority = temp;
						*error_ptr = ERR_SUCCESS;
						break;
					}
					else
					{
						prev->next = temp->next;
						temp->next = NULL;
						temp->p_priority = new_priority;
						changePriority = temp;
						*error_ptr = ERR_SUCCESS;
						break;
					}
				}
				prev = temp;
				temp = temp->next;
			}

	
			if(*error_ptr != ERROR)
			{
				addToReadyQ(changePriority);
				schedular_printer(PRIORITY);
				*error_ptr = ERR_SUCCESS;
			}
		}
	}
	unlockSuspend();
}
INT32  addToMessageQ(S_MEG *enter)
{
	if(megHead == NULL)
	{
		megHead = megTail = enter;
		enter->next = NULL;
		return 0;
	}
	else
	{
		megTail->next = enter;
		enter->next = NULL;
		megTail = enter;
		return 0;
	}
	return -1;
}

INT32  removeFromMessageQ(S_MEG *enter, S_MEG *penter)
{
	if(enter == megHead && megHead->next ==NULL)
	{
		megHead = megTail =NULL;
		return 0;
	}
	else if(enter == megHead &&megHead->next !=NULL)
	{
		megHead = enter->next;
		return 0;
	}
	else if(enter->next = NULL)
	{
		penter->next = NULL;
		megTail = penter;
		return 0;
	}
	else
	{
		penter->next = enter->next;
		return 0;
	}
	return -1;
}

INT32  os_send_message(INT32 target_pid, char *message_buffer_ptr, INT32 message_send_length, INT32 *error_ptr)
{
	S_MEG        *MEG_PTR = NULL;
	S_MEG        *temp = NULL;
	S_MEG        *prev = NULL;
	if(target_pid>99)
	{
		*error_ptr = ERROR;
		return -1;
	}
	else if(totalMessage>MAXTOTALMESSAGES)
	{
		*error_ptr = ERROR;
		return -1;
	}
	else if(message_send_length<strlen(message_buffer_ptr))
	{
		*error_ptr = ERROR;
		return-1;
	}
	else if(message_send_length>MAXMESSAGELENGTH)
	{
		*error_ptr = ERROR;
		return -1;
	}
	else
	{
		temp=waitingMegHead;
		while(temp!=NULL)
		{
			if((temp->target_id == target_pid && temp->sender_id == current_PCB_PTR->p_id||temp->target_id == target_pid && temp->sender_id == -1||target_pid == -1 && temp->sender_id == current_PCB_PTR->p_id||target_pid == temp->sender_id && target_pid == -1 && current_PCB_PTR != temp->target_id )&& temp->status != NOTRUN)
			{
				removeFromWaitingMegQ(temp,prev);
				makeWaitingMegPCBToReady(temp->target_id,error_ptr);
				addToReadyQ(makeWaitngMegPCBToR);
				*error_ptr = ERR_SUCCESS;
				break;}
			prev = temp;
			temp = temp->next;
		}


		MEG_PTR = (S_MEG*)malloc(sizeof(S_MEG));
		MEG_PTR->target_id = target_pid;
		MEG_PTR->sender_id = current_PCB_PTR->p_id;
		strcpy(MEG_PTR->message_buffer,message_buffer_ptr);
		MEG_PTR->message_send_length = message_send_length; 
		addToMessageQ(MEG_PTR);
		totalMessage++;
		*error_ptr = ERR_SUCCESS;
	}

	return 0;
}
INT32 os_receive_message(INT32 source_pid, char *message_buffer_ptr,INT32 message_receive_length,INT32 *message_send_length_ptr,INT32 *meesage_sender_pid_ptr,INT32 *error_ptr)
{
	S_MEG        *temp=NULL;
	S_MEG        *prev=NULL;
	S_MEG        *S_MEG_PTR=NULL;
	temp = megHead;
	if(message_receive_length>MAXMESSAGELENGTH)
	{
		*error_ptr =ERROR;
		return -1;
	}
	else if(message_receive_length<strlen(message_buffer_ptr))
	{
		*error_ptr = ERROR;
		return-1;
	}
	else if(source_pid>99)
	{
		*error_ptr = ERROR;
		return -1;
	}
	else if(source_pid == -1)
	{
		while(temp!=NULL)
		{
			if(temp->target_id == -1&& current_PCB_PTR->p_id !=temp->sender_id|| temp->target_id ==current_PCB_PTR->p_id)
			{
				if((strlen(temp->message_buffer))>message_receive_length)
				{
					*error_ptr = ERROR;
					return -1;
				}
				else
				{
					strcpy(message_buffer_ptr,temp->message_buffer);
					*message_send_length_ptr = temp->message_send_length;
					*meesage_sender_pid_ptr = temp->sender_id;
					removeFromMessageQ(temp, prev);
					*error_ptr = ERR_SUCCESS;
					totalMessage--;
					return 0;
				}
			}
			prev = temp;
			temp = temp->next;
		}
	}
	else
	{	
		while(temp!=NULL)
		{
			if((temp->target_id ==current_PCB_PTR->p_id|| temp->target_id == -1)&& temp->sender_id == source_pid)
			{
				if((strlen(temp->message_buffer))>message_receive_length)
				{
					*error_ptr = ERROR;
					return -1;
				}
				else
				{
					strcpy(message_buffer_ptr,temp->message_buffer);
					*message_send_length_ptr = temp->message_send_length;
					*meesage_sender_pid_ptr = temp->sender_id;
					removeFromMessageQ(temp, prev);
					totalMessage--;
					return 0;
				}
			}
		prev = temp;
		temp = temp->next;
		}
	}

	if(readyHead == NULL)
	{
	test_existing_meg();
	}
	S_MEG_PTR = (S_MEG*)(malloc(sizeof(S_MEG)));
	S_MEG_PTR->sender_id=source_pid;
	S_MEG_PTR->target_id=current_PCB_PTR->p_id;
	S_MEG_PTR->status = RUN;

	addToWaitingMegQ(S_MEG_PTR);
	
	

	makeCurrentToWaitingMegPCB(-1,error_ptr);

	temp = megHead;

	if(source_pid == -1)
	{
		while(temp!=NULL)
		{
			if(temp->target_id == -1|| temp->target_id ==current_PCB_PTR->p_id)
			{
				if((strlen(temp->message_buffer))>message_receive_length)
				{
					*error_ptr = ERROR;
					return -1;
				}
				else
				{
					strcpy(message_buffer_ptr,temp->message_buffer);
					*message_send_length_ptr = temp->message_send_length;
					*meesage_sender_pid_ptr = temp->sender_id;
					removeFromMessageQ(temp, prev);
					totalMessage--;
					return 0;
				}
			}
			prev = temp;
			temp = temp->next;
		}
	}
	else
	{	
		while(temp!=NULL)
		{
			if(temp->target_id ==current_PCB_PTR->p_id && temp->sender_id == source_pid)
			{
				if((strlen(temp->message_buffer))>message_receive_length)
				{
					*error_ptr = ERROR;
					return -1;
				}
				else
				{
					strcpy(message_buffer_ptr,temp->message_buffer);
					*message_send_length_ptr = temp->message_send_length;
					*meesage_sender_pid_ptr = temp->sender_id;
					removeFromMessageQ(temp, prev);
					totalMessage--;
					return 0;
				}
			}
		prev = temp;
		temp = temp->next;
		}
	}
}
void  addToWaitingMegQ(S_MEG *enter)
{
	if (waitingMegHead == NULL)
		{
			 waitingMegHead = enter;
			 waitingMegTail = enter;
			 enter->next =NULL;
		}
	else
		{
			waitingMegTail->next = enter;
			waitingMegTail = enter;
			enter->next = NULL;
		}
}
INT32  removeFromWaitingMegQ(S_MEG *enter, S_MEG *penter)
{
	if(enter == waitingMegHead && waitingMegHead->next ==NULL)
	{
		waitingMegHead = waitingMegTail =NULL;
		return 0;
	}
	else if(enter == waitingMegHead &&waitingMegHead->next !=NULL)
	{
		waitingMegHead = enter->next;
		return 0;
	}
	else if(enter->next == NULL)
	{
		waitingMegTail = penter;
		penter->next = NULL;
		return 0;
	}
	else
	{
		penter->next = enter->next;
		return 0;
	}
	return -1;
}
void  makeCurrentToWaitingMegPCB(INT32 process_id, INT32 *error_ptr)
{	
	if(process_id == -1)
	{
		{
			makeWaitngMegPCBToS = current_PCB_PTR;
			addToWaitingMegPCBQ(makeWaitngMegPCBToS);
			*error_ptr = ERR_SUCCESS;
			Dispatcher();
		}
	}
}
void  addToWaitingMegPCBQ(S_PCB  *enter)
{
	if (waitingMegPCBHead == NULL)
		{
			 waitingMegPCBTail = waitingMegPCBHead = enter;
			 enter->next =NULL;
		}
	else
		{
			waitingMegPCBTail->next = enter;
			waitingMegPCBTail = enter;
			enter->next = NULL;
		}
}
void  makeWaitingMegPCBToReady(INT32 process_id,INT32   *error_ptr)
{
	S_PCB          *temp=NULL;
	S_PCB          *prev=NULL;
	
	temp = waitingMegPCBHead;
	*error_ptr = ERROR;

	while(temp!=NULL)
	{
		if(temp->p_id == process_id)
		{
			if(temp==waitingMegPCBHead && temp->next != NULL)
			{
				waitingMegPCBHead = temp->next;
				temp->next = NULL;
				makeWaitngMegPCBToR = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
			else if((temp==waitingMegPCBHead && temp->next == NULL))
			{
				waitingMegPCBHead = waitingMegPCBTail = NULL;
				temp->next = NULL;
				makeWaitngMegPCBToR = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
			else if(temp->next == NULL)
			{
				waitingMegPCBTail = prev;
				waitingMegPCBTail->next = NULL;
				makeWaitngMegPCBToR = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
			else
			{
				prev->next = temp->next;
				temp->next = NULL;
				makeWaitngMegPCBToR = temp;
				*error_ptr = ERR_SUCCESS;
				break;
			}
		}
		prev = temp;
		temp = temp->next;
	}

}
void  change_waiting_meg_status(INT32 process_id)
{
	S_MEG*      temp = NULL;
	temp =waitingMegHead;

	{
		while(temp!= NULL)
		{
			if(temp->target_id == process_id && temp->status == NOTRUN)
			{temp->status = RUN;break;}
			if(temp->target_id == process_id && temp->status == RUN)
			{temp->status = NOTRUN;break;}
			temp=temp->next;
		}
	}
}

void  test_existing_meg(void)
{
	INT32        error = 0;
	S_MEG* temp1 = NULL;
	S_MEG* temp2 = NULL;
	S_MEG* prev1=NULL;
	S_MEG* prev2=NULL;
	S_PCB* temp3 =NULL;
	S_PCB* prev3=NULL;
	temp1 = megHead;
	temp2 = waitingMegHead;
	temp3 =waitingMegPCBHead;
		while(temp1 != NULL)
		{
			while(temp2 != NULL)
			{
				if(temp1->sender_id == temp2->sender_id&&temp1->target_id==temp2->target_id || temp2->sender_id==-1 &&temp1->target_id==temp2->target_id ||temp1->sender_id == temp2->sender_id&&temp1->target_id==-1 ||temp2->sender_id==-1 &&temp1->target_id==-1 && temp1->sender_id != temp2->target_id)
				{
					while(temp3 != NULL)
					{
						if(temp2->target_id == temp3->p_id&&temp2->status!= NOTRUN)
						{
							removeFromWaitingMegQ(temp2,prev2);
							makeWaitingMegPCBToReady(temp3->p_id, &error);
							addToReadyQ(makeWaitngMegPCBToR);
							break;
						}

					prev3 = temp3;
					temp3 = temp3->next;
					}
				}
				prev2 = temp2;
				temp2 = temp2->next;
				
			}
		
			prev1 = temp1;
			temp1 = temp1->next;
		}
}

