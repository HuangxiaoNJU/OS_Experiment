#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)


PRIVATE unsigned int cursorRecord[2048];
PRIVATE int location;
PRIVATE char findContent[64];
PRIVATE int length;
PRIVATE int findFinish;


PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY* p_tty, u32 key);
PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);

//init_screen
PUBLIC void init_screen(TTY* p_tty)
{
	// init variables
	location = 0;
	color = DEFAULT_CHAR_COLOR;
	findMode = FALSE;
	memset(findContent, '\0', 64);
	length = 0;
	findFinish = FALSE;
	//

	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;
	p_tty->p_console->cursor = disp_pos / 2;
	disp_pos = 0;

	set_cursor(p_tty->p_console->cursor);
}

//out_char
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	if (findMode && findFinish && ch != '\x1B') {
		return;
	}

	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);

	switch(ch) {
	case '\n':
		if (findMode) {
			findFinish = TRUE;
			// find content
			for (int i = 0; i < location - length + 1; ++i) {
				int find = TRUE;
				for (int j = 0; j < length; j++) {
					p_vmem = (u8*)(V_MEM_BASE + cursorRecord[i + j] * 2);
					if (*p_vmem != findContent[j]) {
						find = FALSE;
						break;
					}
				}
				if (find) {
					for (int j = 0; j < length; ++j) {
						p_vmem = (u8*)(V_MEM_BASE + cursorRecord[i + j] * 2 + 1);
						*p_vmem = color;
					}
				}
			}
		} else {
			if (p_con->cursor < p_con->original_addr +
			    p_con->v_mem_limit - SCREEN_WIDTH) {
				// save cursor location
	            cursorRecord[location++] = p_con->cursor;

				p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
					((p_con->cursor - p_con->original_addr) /
					 SCREEN_WIDTH + 1);
			}
		}
		break;
    case '\t':
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 4) {
				int space = 4 - p_con->cursor % 4;
				// save cursor location
                cursorRecord[location++] = p_con->cursor;
			    for (int i = 0; i < space; ++i) {
			        *p_vmem++ = ' ';
					*p_vmem++ = color;
			    }
				p_con->cursor = p_con->cursor + space;
		}
		break;
	case '\b':
		// Set: backspace can't be used in find mode
		if (findMode) {
			break;
		}
		if (p_con->cursor > p_con->original_addr) {
			// cursor location change to last cursor location
            location--;
            int lastLocation = cursorRecord[location];
            p_con->cursor = lastLocation;
                        
            p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
			*(p_vmem ) = ' ';
			*(p_vmem + 1) = color;
		}
		break;
	case '\x1B':
		if (!findMode) {
			disable_irq(CLOCK_IRQ);
			findMode = TRUE;
			color = BLUE;
			for (int i = 0; i < location; ++i) {
				p_vmem = (u8*)(V_MEM_BASE + cursorRecord[i] * 2 + 1);
				*p_vmem = (*p_vmem & 0x0F) + 0x80;
			}
		} else {
			enable_irq(CLOCK_IRQ);
			findMode = FALSE;
			findFinish = FALSE;
			color = DEFAULT_CHAR_COLOR;
			for (int i = 0; i < location; ++i) {
				p_vmem = (u8*)(V_MEM_BASE + cursorRecord[i] * 2 + 1);
				*p_vmem = DEFAULT_CHAR_COLOR;
			}
			// clear find content on screen
			for (; p_con->cursor > cursorRecord[location - 1]; p_con->cursor--) {
				p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
				*(p_vmem ) = ' ';
				*(p_vmem + 1) = color;
			}
			p_con->cursor++;

			// clear findContent and length
			memset(findContent, '\0', 64);
			length = 0;
		}
		break;
	default:
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 1) {
				if (!findMode) {
					// save cursor location
                	cursorRecord[location++] = p_con->cursor;
            	} else {
            		findContent[length++] = ch;
            	}
				*p_vmem++ = ch;
				p_con->cursor++;
				*p_vmem++ = color;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

//flush
PRIVATE void flush(CONSOLE* p_con)
{
        set_cursor(p_con->cursor);
        set_video_start_addr(p_con->current_start_addr);
}

//set_cursor
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

//set_video_start
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}
/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

//task_tty
PUBLIC void task_tty()
{
	TTY* p_tty;

	init_keyboard();

	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
		init_tty(p_tty);
	}
	while (1) {
		for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
			keyboard_read(p_tty);
			tty_do_write(p_tty);
		}
	}
}

//init_tty
PRIVATE void init_tty(TTY* p_tty)
{
	location = 0;
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;
	
	init_screen(p_tty);
}

//in procecss
PUBLIC void in_process(TTY* p_tty, u32 key)/**change**/
{
        char output[2] = {'\0', '\0'};

        if (!(key & FLAG_EXT)) {
			put_key(p_tty, key);
        }
        else {
                int raw_code = key & MASK_RAW;
                switch(raw_code) {
                case ENTER:
						put_key(p_tty, '\n');
						break;
                case BACKSPACE:
						put_key(p_tty, '\b');
						break;
				// add TAB
				case TAB:
                        put_key(p_tty, '\t');
						break;
				// add ESC
				case ESC:
						put_key(p_tty, '\x1B');
						break;
                case UP:
                        if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
							scroll_screen(p_tty->p_console, SCR_DN);
                        }
						break;
				case DOWN:
						if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
							scroll_screen(p_tty->p_console, SCR_UP);
						}
						break;
        		default:
            			break;
                }
        }
}

//put_key
PRIVATE void put_key(TTY* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}

//write
PRIVATE void tty_do_write(TTY* p_tty)
{
	if (p_tty->inbuf_count) {
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(p_tty->p_console,ch);
	}
}

