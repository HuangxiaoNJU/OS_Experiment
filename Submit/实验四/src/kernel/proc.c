
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	for (int i = 0; i < NR_TASKS + NR_PROCS; ++i) {
		p_proc_ready++;
		if (p_proc_ready >= proc_table + NR_TASKS + NR_PROCS) {
			p_proc_ready = proc_table;
		}
		if (p_proc_ready -> is_ready && p_proc_ready -> sleep == 0) {
			break;
		}
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks() {
	return ticks;
}

PUBLIC void sys_process_sleep(int mill_seconds) {
	p_proc_ready->sleep = mill_seconds* HZ / 1000 + 1;
	schedule();
}

PUBLIC void sys_disp_str(char* str) {
	printf(str);
}

PUBLIC void sys_sem_p(semaphore* sem) {
	sem->value--;
	if (sem->value < 0) {
		sem->list[sem->len++] = p_proc_ready;
		p_proc_ready->is_ready = FALSE;
		// if barber process go sleep.
		if (p_proc_ready -> pid == 2) {
			my_disp_str("There is no customer, barber go sleeping.\n");
		}
		schedule();
	}
}

PUBLIC void sys_sem_v(semaphore* sem) {
	sem->value++;
	if (sem->value <= 0) {
		sem->list[0]->is_ready = TRUE;
		for (int i = 1; i < sem->len; ++i) {
			sem->list[i-1] = sem->list[i];
		}
		sem->len--;
	}
}