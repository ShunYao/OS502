#include        "stdio.h"
#include	"stdlib.h"

#define			MAX_PIDs		        10
#define         MAX_NAME                32
#define         TIMER_ITR                4
#define         RUN                      1
#define         NOTRUN                   0

#define	        CREATE		           100
#define			READY			       101
#define			RUNNING			       102
#define         SUSPEND                103
#define			SLEEP		           104
#define         RESUSPEND              105
#define         PRIORITY               106
#define         CHANGE                 107

#define         MAXPROCESS              25
#define         MAXMESSAGELENGTH        64
#define         MAXTOTALMESSAGES         8

#define         SUCCESS                  0
#define         SAME_NAME_ERROR         -1
#define         TOOMUCHPROCESSES        -2
#define         CANNOTFINDID            -3
#define         LEGAL                 10
#define         ILLEGAL               -3
#define         CANNOTTERMINATE       -1
#define         ERROR                 -10

#define                 DO_LOCK       1
#define                 DO_UNLOCK     0

//PCB structure

typedef         struct {
	char					p_name[MAX_NAME+1];      //name
	INT32					p_id;                    //process id
    INT32					p_time;                  //wake up time
    INT32                   p_status;                 //state
    INT32                   p_position;
	INT32                   p_priority;
	INT32                   p_error;
	INT32                   p_meg_status;
    void					*context; 
	void				    *next;                    //link to next node

}S_PCB;

typedef      struct S_MEG{
	INT32				   sender_id;
	INT32				   target_id;
	char                   message_buffer[100];
	INT32                  message_send_length;
	INT32                  status;                 
	struct S_MEG           *next;
	
}S_MEG;

S_PCB* timerHead = NULL;
S_PCB* timerTail = NULL;
S_PCB* readyHead = NULL;
S_PCB* readyTail = NULL;
S_PCB* suspendHead = NULL;
S_PCB* suspendTail = NULL;
S_PCB* makeTimer = NULL;
S_PCB* makeReady = NULL;
S_PCB* makeSuspend = NULL;
S_PCB* resumeSuspend = NULL;
S_PCB* current_PCB_PTR = NULL;
S_PCB* changePriority = NULL;
S_MEG* megHead = NULL;
S_MEG* megTail = NULL;
S_MEG* waitingMegHead=NULL;
S_MEG* waitingMegTail=NULL;
S_PCB* makeWaitngMegPCBToS=NULL;
S_PCB* waitingMegPCBHead=NULL;
S_PCB* waitingMegPCBTail=NULL;
S_PCB* makeWaitngMegPCBToR=NULL;


INT32   os_create_process(char *name, void *test_name, INT32 initial_prriority,INT32 *pid_ptr,INT32 *p_error_prt, INT32 status);
void make_context (S_PCB *PCB_PRT, void *procPTR );
void switch_context (S_PCB *PCB_PTR );
INT32 addToTimerQ(S_PCB * enter);
INT32 addToReadyQ(S_PCB * enter);
void Dispatcher();
void makeReadyToRun(S_PCB *enter_ptr);
void makeTimerToReady(S_PCB *enter_ptr);
void   start_timer(INT32 timer_time);
INT32 os_get_process_id(char *name, INT32 *pid_ptr, INT32 *p_error_prt);
INT32 schedular_printer(INT32 action);
void  switch_context_kill (S_PCB *PCB_PTR );
void  print_queues ();
void  lockTimer( void );
void  unlockTimer ( void );
void  lockReady ( void );
void  unlockReady ( void );
void  lockSP( void);
void  unlockSP( void);
void  os_suspend_process(INT32 process_id, INT32   *error_ptr);
void  makeReadyToSuspend(INT32 process_id,INT32   *error_ptr);
void  addToSuspendQ(S_PCB  *enter);
INT32   os_resume_process(INT32 process_id, INT32   *error_ptr);
void  resumeSuspendToReady(INT32 process_id,INT32   *error_ptr);
INT32   os_change_priority(INT32 process_id, INT32 new_priority, INT32   *error_ptr);
void  lockSuspend(void);
void  unlockSuspend(void);
void  lockpriority(void);
void  unlockpriority(void);
INT32  addToMessageQ(S_MEG *enter);
INT32  os_send_message(INT32 target_pid, char *message_buffer_ptr, INT32 message_send_length, INT32 *error_ptr);
INT32 os_receive_message(INT32 source_pid, char *message_buffer_ptr,INT32 message_receive_length,INT32 *message_send_length_ptr,INT32 *meesage_sender_pid_ptr,INT32 *error_ptr);
INT32  removeFromMessageQ(S_MEG *enter, S_MEG *penter);
void addToWaitingMegQ(S_MEG *enter);
INT32  removeFromWaitingMegQ(S_MEG *enter, S_MEG *penter);
void  makeCurrentToWaitingMegPCB(INT32 process_id, INT32 *error_ptr);
void  makeWaitingMegPCBToReady(INT32 process_id,INT32   *error_ptr);
void  makeWaitngMegPCBToSuspend(INT32 process_id,INT32   *error_ptr);
void  addToWaitingMegPCBQ(S_PCB  *enter);
void  change_waiting_meg_status(INT32 process_id);
void  test_existing_meg(void);
