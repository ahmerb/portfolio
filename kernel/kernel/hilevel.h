#ifndef __HILEVEL_H
#define __HILEVEL_H

// Include functionality relating to newlib (the standard C library).

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <string.h>

#include <limits.h>

// Include functionality relating to the platform.

#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"

// Include functionality relating to the   kernel.

#include "lolevel.h"
#include     "int.h"

/* The kernel source code is made simpler via three type definitions:
 *
 * - an enum type that captures the status of a process in the pcb block.
 *   The possible statuses are defined by the lecture slides.
 * - a type that captures a Process IDentifier (PID), which is really
 *   just an integer,
 * - a type that captures each component of an execution context (i.e.,
 *   processor state) in a compatible order wrt. the low-level handler
 *   preservation and restoration prologue and epilogue, and
 * - a type that captures a process PCB.
 */

typedef enum {
  empty, // pcb created but process should not be executed
  ready,
  waiting,
  executing
} status_t;

typedef int pid_t;

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef struct {
  pid_t       pid;
  status_t    status;
  uint32_t    priority;
  uint32_t    stack_top;
  ctx_t       ctx;
} pcb_t;

typedef enum {
  closed,
  ready_to_read,
  ready_to_write,
  open
} p_status_t;

typedef struct {
  int         buffer;
  pid_t       parent;
  pid_t       read_end;
  pid_t       write_end;
  bool        has_read_end;
  bool        has_write_end;
  p_status_t  status;
} pipe_t;

#endif
