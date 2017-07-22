
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	disp_pos = 0;
	for(int i = 0; i < 80*25; i++) {
		disp_str(" ");
	}
	disp_pos = 0;

	waiting = 0;
	CHAIRS = 3;
	customerID = 0;

	// Initialize semaphore
	customers.value = 0;
	customers.len = 0;

	barbers.value = 0;
	barbers.len = 0;

	mutex.value = 1;
	mutex.len = 0;

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
        u8              privilege;
        u8              rpl;
        int             eflags;
	for (i = 0; i < NR_TASKS+NR_PROCS; i++) {
                if (i < NR_TASKS) {     /* 任务 */
                        p_task    = task_table + i;
                        privilege = PRIVILEGE_TASK;
                        rpl       = RPL_TASK;
                        eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
                }
                else {                  /* 用户进程 */
                        p_task    = user_proc_table + (i - NR_TASKS);
                        privilege = PRIVILEGE_USER;
                        rpl       = RPL_USER;
                        eflags    = 0x202; /* IF=1, bit 2 is always 1 */
                }

		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid
		p_proc->is_ready = TRUE;
		p_proc->sleep = 0;

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		p_proc->nr_tty = 0;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].ticks = proc_table[0].priority = 15;
	proc_table[1].ticks = proc_table[1].priority = 15;
	proc_table[2].ticks = proc_table[2].priority = 15;
	proc_table[3].ticks = proc_table[3].priority = 15;
	proc_table[4].ticks = proc_table[4].priority = 15;
	proc_table[5].ticks = proc_table[5].priority = 15;

        proc_table[1].nr_tty = 0;
        proc_table[2].nr_tty = 0;
        proc_table[3].nr_tty = 0;
        proc_table[4].nr_tty = 0;
        proc_table[5].nr_tty = 0;

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

	init_clock();
        init_keyboard();

	restart();

	while(1){}
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	int i = 0;
	while (1) {
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	int i = 0x1000;
	int customer;
	while(1){
		sem_p(&customers);
		sem_p(&mutex);
		customer = wait[0];
		for (int i = 1; i < waiting; ++i) {
			wait[i - 1] = wait[i];
		}
		waiting--;

		sem_v(&barbers);
		sem_v(&mutex);

		printf("Barber cut hair for Customer %x.\n", customer);
		milli_delay(50000);
		printf("Barber finish cutting. Customer %x leave.\n", customer);
	}
}

/*======================================================================*
                               TestC
 *======================================================================*/
void TestC()
{
	int i = 0x2000;
	while(1){
		sem_p(&mutex);
		customerID++;
		printf("Customer %x come ", customerID);
		if (waiting < CHAIRS) {
			wait[waiting++] = customerID;
			printf("and wait. Waiting number: %x.\n", waiting);
			sem_v(&customers);
			sem_v(&mutex);
			sem_p(&barbers);
		} else {
			my_disp_str("and leave.\n");
			sem_v(&mutex);
		}
		milli_delay(30000);
	}
}

/*======================================================================*
                               TestD
 *======================================================================*/
void TestD()
{
	int i = 0x3000;
	while(1){
		sem_p(&mutex);
		customerID++;
		printf("Customer %x come ", customerID);
		if (waiting < CHAIRS) {
			wait[waiting++] = customerID;
			printf("and wait. Waiting number: %x.\n", waiting);
			sem_v(&customers);
			sem_v(&mutex);
			sem_p(&barbers);
			// printf("Customer D cut hair.\n");
		} else {
			my_disp_str("and leave.\n");
			sem_v(&mutex);
		}
		milli_delay(40000);
	}
}

/*======================================================================*
                               TestE
 *======================================================================*/
void TestE()
{
	int i = 0x4000;
	while(1){
		sem_p(&mutex);
		customerID++;
		printf("Customer %x come ", customerID);
		if (waiting < CHAIRS) {
			wait[waiting++] = customerID;
			printf("and wait. Waiting number: %x.\n", waiting);
			sem_v(&customers);
			sem_v(&mutex);
			sem_p(&barbers);
			// printf("Customer E cut hair.\n");
		} else {
			my_disp_str("and leave.\n");
			sem_v(&mutex);
		}
		milli_delay(20000);
	}
}