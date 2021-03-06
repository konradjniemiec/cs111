#ifndef WEENSYOS_SCHEDOS_H
#define WEENSYOS_SCHEDOS_H
#include "types.h"

/*****************************************************************************
 * schedos.h
 *
 *   Constants and variables shared by SchedOS's kernel and applications.
 *
 *****************************************************************************/

// System call numbers.
// An application calls a system call by causing the specified interrupt.

#define INT_SYS_YIELD		48
#define INT_SYS_EXIT		49
#define INT_SYS_LOCK		50
#define INT_SYS_UNLOCK		51
#define INT_SYS_PRIORITY        52
#define INT_SYS_PRINT_CHAR      53

// The current screen cursor position (stored at memory location 0x198000).

extern uint16_t * volatile cursorpos;

#endif
