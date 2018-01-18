#include "hilevel.h"

/***********
 * Control logging verbosity and debugging behaviour
 */

int verbose        = false;  // prints chars to indicate beginning of certain procedures
int debug          = false;  // prints more lines for debugging and enables certain behaviour
int fatal_asserts  = false;  // causes ASSERTFAIL() calls to trigger reset interrupt
int timer_on       = true;  // turn timer interrupts on or off


/***********
 * PCB table
 *
 * Since we *know* there will be 4 processes, stemming from the 4 user
 * programs, we can
 *
 * - allocate a fixed-size process table (of PCBs), and use a pointer
 *   to keep track of which entry is currently executing, and
 * - employ a fixed-case of round-robin scheduling: no more processes
 *   can be created, and neither is able to complete.
 */

// max number of user processes we can have == max number of stacks we can allocate
//   (defined by length of tos_usr, which is defined in loader script)
// pcb[   0   ] -> console
// pcb[ 1..19 ] -> other user processes
#define MAX_PROCESSES 20
pcb_t pcb[ MAX_PROCESSES ];

pcb_t *current     = NULL;
int    current_num =    0;

// stores the highest pid that has been allocated.
// start at 1, which is the console process' pid.
pid_t pidCtr = 1;

// To generate PIDs, begin at one and rise to a maximum number before
// resetting to zero again.
// Once this limit is reached, allocation restarts at zero and again increases.
// However, for this and subsequent passes any PIDs still assigned to processes
// are skipped.
int next_pid( void ) {
  // assign next pid as pidCtr + 1 mod INT_MAX
  pidCtr = (pidCtr + 1) % INT_MAX;

  // check not already assigned, else reassign
  for ( int i = 0; i < MAX_PROCESSES; i++ ) {
    if ( pcb[ i ].pid == pidCtr && !( pcb[ i ].status == empty ) ) {
      pidCtr = next_pid();
      break;
    }
  }

  return pidCtr;
}

// gets the index of a process in the pcb table, given it's pid.
// if not found, return -1
int get_ix( pid_t pid ) {
  int ix = -1;
  for ( int i = 0; i < MAX_PROCESSES; i++ ) {
    if ( pcb[i].pid == pid ) {
      ix = i;
      break;
    }
  }
  return ix;
}

// pretty print a status code
char pretty_status( status_t status ) {
  char pretty = 'e';
  switch ( status ) {
    case empty:
      pretty = 'e'; break;
    case ready:
      pretty = 'r'; break;
    case waiting:
      pretty = 'w'; break;
    case executing:
      pretty = 'x'; break;
    default:
      pretty = 'u'; break;
  }
  return pretty;
}


/*******
 * Error throwing functions
 */

// Triggers reset interrupt
void FAIL(void) {
  if (verbose) PL011_putc( UART0, 'F', true );
  lolevel_handler_rst();
}

// Triggers reset interrupt if `assert == true`
void ASSERTFAIL(void) {
  if (fatal_asserts) {
    if (verbose) PL011_putc( UART0, 'A', true );
    lolevel_handler_rst();
  }
}


/*********
 * User processes
 */

// user process to spawn on reset
extern void main_console();

// usr mode stack pointer
extern uint32_t tos_usr;

// the size of a stack given to a user process
#define STACK_SIZE 0x00001000 // 0x00010000 / 0xF

// number of user processes == number of stacks currently executing/ready/waiting (not empty)
int usr_process_ctr = 0;


/**********
 * Pipes
 *
 * pipe id is its index in pipes array.
 * return values are pipe_id's (except on pipe_read, it is the data read)
 * , or -1 for input pipeid error
 * , or -2 for permissions error,
 * , or -3 if pipe status blocks requested read/write.
 * , or -4 if attempt to assign read/write end failed because read/write end already assigned.
 */

#define MAX_PIPES 20
pipe_t pipes[ MAX_PIPES ];

// returns ix for a closed pipe in pipes, or -1 if not found
int __find_closed_pipe( void ) {
  for ( int i = 0; i < MAX_PIPES; i++ ) {
    if ( pipes[ i ].status == closed ) {
      return i;
    }
  }
  return -1;
}

// inits pipe in pipes[ id ] to given processes
int __init_pipe( int id ) {
  // ensure id is valid
  if ( id < 0 || id >= MAX_PIPES ) {
    return -1;
  }

  pipe_t* pipe        = &pipes[ id ];

  pipe->buffer        = 0;
  pipe->parent        = current->pid;
  pipe->status        = open;
  pipe->has_read_end  = false;
  pipe->has_write_end = false;

  return id;
}

// PIPE_OPEN
int __pipe_open() {
  // find available pipe
  int pipe_ix = __find_closed_pipe();

  // initialise the pipe
  pipe_ix = __init_pipe( pipe_ix );

  // return pipe's id ( will be -1 if error )
  return pipe_ix;
}

// PIPE_CANWRITE
bool __pipe_can_write( int id ) {
  if ( id < 0 || id >= MAX_PIPES ) return false;

  return pipes[ id ].status == ready_to_write;
}

// PIPE_WRITE
int __pipe_write( int id, uint32_t data ) {
  // ensure id is valid
  if ( id < 0 || id >= MAX_PIPES) return -1;

  // capture pipe
  pipe_t* pipe = &pipes[ id ];

  // ensure pipe can be written to / exists
  if ( pipe->status != ready_to_write ) return -3;

  // ensure current process is the write_end
  if ( current->pid != pipe->write_end ) return -2;

  // write to pipe
  pipe->buffer = data;

  // update status
  pipe->status = ready_to_read;

  // return id, no errors
  return id;
}

// PIPE_CANREAD
bool __pipe_can_read( int id ) {
  if ( id < 0 || id >= MAX_PIPES ) return false;

  return pipes[ id ].status == ready_to_read;
}

// PIPE_READ
int __pipe_read( int id, uint32_t buffer ) {
  // ensure id is valid
  if ( id < 0 || id >= MAX_PIPES )     return -1;

  // capture pipe
  pipe_t* pipe = &pipes[ id ];

  // ensure pipe can be read from / exists
  if ( pipe->status != ready_to_read )  return -3;

  // ensure current process is the read_end
  if ( current->pid != pipe->read_end ) return -2;

  // read from pipe
  buffer = pipe->buffer;

  // update status
  pipe->status = ready_to_write;

  // return buffer, no errors
  return buffer;
}

// PIPE_CLOSE
int __pipe_close( int id ) {
  // ensure id is valid
  if ( id < 0 || id >= MAX_PIPES ) return -1;

  // capture pipe
  pipe_t* pipe = &pipes[ id ];

  // ensure closer is pipe parent (the instantiator of pipe)
  if ( current->pid != pipe->parent ) return -2;

  // set pipe to closed
  pipe->status = closed;

  // return id
  return id;
}

// PIPE_ASSIGN_READ
int __pipe_assign_read( int id, pid_t pid ) {
  // ensure id is valid
  if ( id < 0 || id >= MAX_PIPES ) return -1;

  // capture pipe
  pipe_t* pipe = &pipes[ id ];

  // do not allow read_end change if already assigned
  if ( pipe->has_read_end ) return -4;

  // set pipe read_end
  pipe->read_end     = pid;
  pipe->has_read_end = true;

  // set to ready to write if writer also assigned
  if ( pipe->has_write_end ) {
    pipe->status = ready_to_write;
  }

  return id;
}

// PIPE_ASSIGN_WRITE
int __pipe_assign_write( int id, pid_t pid ) {
  // ensure id is valid
  if ( id < 0 || id >= MAX_PIPES ) return -1;

  // capture pipe
  pipe_t* pipe = &pipes[ id ];

  // do not allow write_end change if already assigned
  if ( pipe->has_write_end ) return -4;

  // set pipe write_end
  pipe->write_end     = pid;
  pipe->has_write_end = true;

  // set to ready to write if reader also assigned
  if ( pipe->has_read_end ) {
    pipe->status = ready_to_write;
  }

  return id;
}

// kernel always has permission to close any pipe
void __force_close_pipe( int id ) {
  // set pipe to closed
  pipes[ id ].status = closed;
}


/**********
 * Scheduler
 */

// the maxmimum priority value ( 0 <= priority(p) <= MAX_PRIORITY )
#define MAX_PRIORITY 10

// the nice value, which a fork caller can request to be added to a forked processes initial priority
#define NICE_VAL 5

// find the next process whose status is waiting and return it's index
int nextProcessIx_RoundRobin( void ) {
  // if left unassigned, no waiting processes, so current is still to execute
  int next_num = current_num;

  // loop through processes, starting at pcb[ current_num ]
  for ( int i = current_num; i < current_num + MAX_PROCESSES; i++ ) {
    //if (debug) PL011_putc( UART0, pretty_status( pcb[ i % MAX_PROCESSES ].status ), true );
    int table_ix = i % MAX_PROCESSES;
    if ( pcb[ table_ix ].status == ready ) { // pcb table length = MAX_PROCESSES
      next_num = table_ix;
      break;
    }
  }

  return next_num;
}

int nextProcessIx_Priority( void ) {
    // find ready/executing process with highest priority
    int max_ix = current_num;

    // loop through pcb table
    for ( int i = current_num + 1; i <= current_num + MAX_PROCESSES; i++ ) {
      int curr_ix = i % MAX_PROCESSES;
      // only consider process if it is ready or executing already
      if ( pcb[ curr_ix ].status == ready || pcb[ curr_ix ].status == executing ) {
        // set ix_max to current process in pcb if it's priority is higher
        if ( pcb[ curr_ix ].priority > pcb[ max_ix ].priority ) {
          max_ix = curr_ix;
        }
      }
    }

  return max_ix;
}

void update_priorities( void ) {
  // loop through pcb and increment priorities of all non exec'ing processes
  for ( int i = 0; i < MAX_PROCESSES; i++ ) {
    if ( pcb[ i ].status != executing ) {
      pcb[ i ].priority += 1;

      // ensure priority isn't set above max
      if ( pcb[ i ].priority > MAX_PRIORITY )
        pcb[ i ].priority = MAX_PRIORITY;
    }
    // lower priority for executing process as its been run recently.
    else {
      // reset to NICE_VAL if currently higher than NICE_VAL, else reset to 1
      pcb[ i ].priority = pcb[ i ].priority > NICE_VAL ? NICE_VAL : 1;
    }
  }
}

void scheduler( ctx_t* ctx ) {
  if (verbose) PL011_putc( UART0, 'N', true );

  // find next process' index in pcb table
  int next_num = nextProcessIx_Priority();
  //int next_num = nextProcessIx_RoundRobin();

  // don't bother switching context if the executing process is not going to change
  if ( current_num != next_num ) {
    if (verbose) PL011_putc( UART0, 'n', true );

    // save current process' context
    memcpy( &(current->ctx), ctx, sizeof( ctx_t ) );

    // switch to the pcb entry next
    memcpy( ctx, &(pcb[ next_num ].ctx), sizeof( ctx_t ) );

    // update statuses: if current is empty, it was deleted, so leave as empty.
    //                  else, it is being paused, so set to ready.
    if ( pcb[ current_num ].status != empty ) { // NOTE: maybe change to status == executing, is more precise case
        current->status = ready;
    }

    // update statuses: set new execiuting process' status to executing
    pcb[ next_num ].status    = executing;

    // update current pcb entry
    current     = &pcb[ next_num ];
    current_num = next_num;

  }

  // update dynamic priorities
  update_priorities();

  // What happens when all processes are terminated???
  //        |-> actually, we can't terminate the console, so never happens.

  if (verbose) PL011_putc( UART0, '\n', true );
  return;
}


/*********
 * High level interrupt handlers
 */

// RST handler
void hilevel_handler_rst( ctx_t* ctx ) {
  PL011_putc( UART0, '\n', true );
  PL011_putc( UART0, 'R' , true );
  PL011_putc( UART0, '\n', true );

  /* 1) Configure the mechanism for interrupt handling by
   *
   * - configuring UART st. an interrupt is raised every time a byte is
   *   subsequently received,
   * - configuring timer st. it raises a (periodic) interrupt for each
   *   timer tick,
   * - configuring GIC st. the selected interrupts are forwarded to the
   *   processor via the IRQ interrupt signal, then
   * - enabling IRQ interrupts.
   */

  UART0->IMSC        |= 0x00000010;   // enable UART      (Rx) interrupt
  UART0->CR           = 0x00000301;   // enable UART   (Tx+Rx)

  GICC0->PMR          = 0x000000F0;   // unmask all            interrupts
  GICD0->ISENABLER1  |= 0x00001000;   // enable UART      (Rx) interrupt
  GICC0->CTLR         = 0x00000001;   // enable GIC interface
  GICD0->CTLR         = 0x00000001;   // enable GIC distributor

  if ( timer_on ) {
    TIMER0->Timer1Load  = 0x00060000; // select period = 2^20 ticks ~= 1 sec
    TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
    TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
    TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
    TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

    GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
  }

  /* 2) Initialise PCBs representing processes stemming from execution of
  * the four user programs.  Note in each case that
  *
  * - the CPSR value of 0x50 means the processor is switched into USR
  *   mode, with IRQ interrupts enabled, and
  * - the PC and SP values match the entry point and top of stack.
  *
  * Note that, only the console program is set s.t. .status = ready.
  * Other processes will thus not be executed.
  */

  // set pcb[ 0 ] to the console process
  memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );
  pcb[ 0 ].pid        = pidCtr;
  pcb[ 0 ].status     = ready;
  pcb[ 0 ].priority   = NICE_VAL; // initial priority for console process
  pcb[ 0 ].ctx.cpsr   = 0x50;
  pcb[ 0 ].ctx.pc     = ( uint32_t )( &main_console );
  pcb[ 0 ].ctx.sp     = ( uint32_t )( &tos_usr      );
  pcb[ 0 ].stack_top  = ( uint32_t )( &tos_usr      );

  // update user process counter
  usr_process_ctr++;

  // init the rest of the pcb table to zeros
  memset( &pcb[ 1 ], 0, sizeof( pcb_t ) * MAX_PROCESSES ); // there are 15 other table rows

  /* 3) Once the PCBs are initialised, we select the console process to be
  * restored (i.e., executed) when the function then returns.
  */

  current         = &pcb[ 0 ];
  current_num     = 0;
  current->status = executing;
  memcpy( ctx, &current->ctx, sizeof( ctx_t ) );

  /* 4) init pipes array
  */
  //memset( &pipes, 0, sizeof( pipe_t ) * MAX_PIPES );
  for ( int i = 0; i < MAX_PIPES; i ++ ) {
    pipes[ i ].status = closed;
  }

  return;
}

// IRQ handler
void hilevel_handler_irq( ctx_t* ctx ) {
  if (verbose) PL011_putc( UART0, 'I', true );

  // Step 2: read  the interrupt identifier so we know the source.

  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.

  // case: interrupt source is timer
  if( id == GIC_SOURCE_TIMER0 ) {
    if (verbose) PL011_putc( UART0, 't', true );
    TIMER0->Timer1IntClr = 0x01;
    scheduler( ctx );
  }

  // case: interrupt source is from UART0 (keyboard)
  else if( id == GIC_SOURCE_UART0 ) {
    // assume UART0 raised an interrupt because there is a byte to be read
    // (make assumption, do no explicit checking to see why interrupt was raised)
    uint8_t x = PL011_getc( UART0, true );

    PL011_putc( UART0, 'k',                      true );
    PL011_putc( UART0, '<',                      true );
    PL011_putc( UART0, itox( ( x >> 4 ) & 0xF ), true );
    PL011_putc( UART0, itox( ( x >> 0 ) & 0xF ), true );
    PL011_putc( UART0, '>',                      true );

    // signal to UART0 (interrupt source) that interrupt has been handled
    // In UART0, this clears the interrupt and so resets the interrupt generations logic
    // in the device, allowing it to raise another interrupt in the future
    UART0->ICR = 0x10;
  }

  // Step 5: write the interrupt identifier to signal we're done.

  GICC0->EOIR = id;

  return;
}

// SVC handler
int findEmptyEntryInPcbTable( void ) {
  // loop through pcb and find the first entry with empty status
  int ix = -1;
  for ( int i = 0; i < MAX_PROCESSES; i++ ) {
    if ( pcb[i].status == empty ) {
      ix = i;
      break;
    }
  }
  return ix;
}

void __yield( ctx_t* ctx ) {
  scheduler( ctx );
}

void __write( ctx_t* ctx ) {
  // get args to write call
  int   fd = ( int   )( ctx->gpr[ 0 ] ); // file descriptor to write to
  char*  x = ( char* )( ctx->gpr[ 1 ] ); // string to write
  int    n = ( int   )( ctx->gpr[ 2 ] ); // bytes of string to write

  // print first n bytes from x to fd
  for( int i = 0; i < n; i++ ) {
    PL011_putc( UART0, *x++, true );
  }

  // set return value to n (bytes written)
  ctx->gpr[ 0 ] = n;
}

void __fork( ctx_t* ctx ) {
  // create new child process with unique pid and copy of parent ctx

  // get args to fork call
  bool nice = ( bool )( ctx->gpr[0] ); // decide whether to add NICE_VAL to init priority

  // 1) find pcb entry for new process
  int child_num = findEmptyEntryInPcbTable();

  // if no space left in pcb table or other error: return -1 in parent. Else, continue
  if (child_num == -1) {
    ctx->gpr[ 0 ] = -1;
    ASSERTFAIL();
  }
  else {
    // 2) create copy of parent process (the caller) in new PCB table, but with new pid

    //  2a) create exact copy (*current has outdated context,
    //        so need to overwrite with new context first)
    pcb_t *child = &pcb[ child_num ];
    memcpy( &current->ctx, ctx, sizeof( ctx_t ) );
    memcpy( child, current, sizeof( pcb_t ) );

    //  2b) give child unique pid
    child->pid = next_pid();

    //  2c) give child its own stack, but filled with parents stack contents.
    //      maintain user stack i is for process i in pcb table.

    //   2ci) find the address for child's stack.
    child->stack_top = ( uint32_t ) &tos_usr - get_ix( child->pid ) * STACK_SIZE;

    //   2ci) calculate address of stack pointer for child (sp and contents same as parent)
    child->ctx.sp = child->stack_top - ( current->stack_top - current->ctx.sp );

    //   2ci) calculate the difference between the parent's stack top and it's sp
    uint32_t size_stack_used = current->stack_top - ctx->sp;

    //   2cii) copy the parents stack to the childs stack
    memcpy( ( void * ) child->ctx.sp, ( void * ) ctx->sp, size_stack_used );

    //  2d) ...all done, set child's status to ready
    child->status = ready;

    // 3) maintain usr_process_ctr
    usr_process_ctr++;

    // 4) set priority
    if ( nice ) {
      child->priority = NICE_VAL;
      if (debug) PL011_putc( UART0, '@', true );
    }
    else {
      if (debug) PL011_putc( UART0, '#', true );
      child->priority = 1;
    }

    // 4) set return value in parent process to pid of child process
    ctx->gpr[ 0 ] = child->pid;

    // 5) set return value in child process to zero
    child->ctx.gpr[ 0 ] = 0;
  }
}

void __exit( ctx_t* ctx ) {
  // get args to exit call
  int x = ( int )( ctx->gpr[ 0 ] ); // exit status code

  // 1) perform termination, by setting the pcb's status
  current->status = empty;

  // 2) pass exit status code to parent

  // 3) maintain usr_process_ctr
  usr_process_ctr--;

  // 4) close open pipes
  pid_t pid = current->pid;
  for ( int i = 0; i < MAX_PIPES; i++ ) {
    if ( pipes[ i ].parent == pid || pipes[ i ].read_end == pid || pipes[ i ].write_end == pid ) {
      __force_close_pipe( i );
    }
  }

  // 5) invoke scheduler to invoke another user process
  scheduler( ctx );
}

void __exec( ctx_t* ctx ) {
  // replace current process image with new process image

  // 1) get args to exec call
  uint32_t x = ( uint32_t )( ctx->gpr[ 0 ] ); // pointer to user process

  // 2) create new process image.
  // cpsr set to USR mode with IRQ interrupts enabled
  // pc set to x
  // sp: stack contents are cleared. sp moved back to top and clear the contents.
  // gpr's are set to 0s
  // lr is left unchanged (new process will overwrite it)
  ctx->cpsr = 0x50;
  ctx->pc   = ( uint32_t ) x;
  ctx->sp   = current->stack_top;
  memset( (void *) current->stack_top - STACK_SIZE, 0, STACK_SIZE ); // clear stack
  memset( &ctx->gpr, 0, sizeof( uint32_t ) * 13 ); // clear the 13 gpr's
}

void __kill( ctx_t* ctx ) {
  // 1) get args to kill call
  pid_t pid = ( pid_t )( ctx->gpr[ 0 ] ); // pid of process to kill

  // 2) get ix in pcb table, given pid
  int ix = get_ix( pid );

  // 3) ensure pid is valid, else set return value to -1
  if ( ix == -1 ) {
    ctx->gpr[ 0 ] = -1; // err: pid out of range or no process with given pid
  }
  else {
    // success, perform termination
    pcb[ ix ].status = empty;

    // maintain process counter
    usr_process_ctr--;

    // close open pipes
    pid_t pid = current->pid;
    for ( int i = 0; i < MAX_PIPES; i++ ) {
      if ( pipes[ i ].parent == pid || pipes[ i ].read_end == pid || pipes[ i ].write_end == pid ) {
        __force_close_pipe( i );
      }
    }

    // set return value to success
    ctx->gpr[ 0 ] = 0;
  }
}

void __exec_warg( ctx_t* ctx ) {
  // get args to exec call
  uint32_t x = ( uint32_t ) ( ctx->gpr[ 0 ] ); // pointer to user process
  int    arg = ( int      ) ( ctx->gpr[ 1 ] ); // command line argument

  // create new process image
  ctx->cpsr = 0x50;
  ctx->pc   = ( uint32_t ) x;
  ctx->sp   = current->stack_top;
  memset( (void *) current->stack_top - STACK_SIZE, 0, STACK_SIZE ); // clear stack
  memset( &ctx->gpr, 0, sizeof( uint32_t ) * 13 ); // clear the 13 gpr's

  // set command line argument in new process image
  ctx->gpr[ 0 ] = ( int ) arg;
}

void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {
  if (verbose) PL011_putc( UART0, 'S', true );

  /* Based on the identifier encoded as an immediate operand in the
   * instruction,
   *
   * - read  the arguments from preserved usr mode registers,
   * - perform whatever is appropriate for this system call,
   * - write any return value back to preserved usr mode registers.
   */

  switch( id ) {
    // 0x00 => yield()
    case 0x00 : {
      if (verbose) PL011_putc( UART0, 'y', true );
      __yield( ctx );
      break;
    }

    // 0x01 => int write( fd, x, n )
    case 0x01 : {
      if (verbose) PL011_putc( UART0, 'w', true );
      __write( ctx );
      break;
    }

    // 0x03 => fork( bool nice )
    case 0x03 : {
      if (verbose) PL011_putc( UART0, 'f', true );
      __fork( ctx );
      break;
    }

    // 0x04 => void exit( x )
    case 0x04 : {
      if (verbose) PL011_putc( UART0, 'x', true );
      __exit( ctx );
      break;
    }

    // 0x05 => exec( const void *x )
    case 0x05 : {
      if (verbose) PL011_putc( UART0, 'e', true );
      __exec( ctx );
      break;
    }

    // 0x06 => int kill( pid_t pid )
    case 0x06 : {
      if (verbose) PL011_putc( UART0, 'k', true );
      __kill( ctx );
      break;
    }

    // 0x07 => get_pid()
    case 0x07 : {
      // set return value to pid of current process
      ctx->gpr[ 0 ] = current->pid;
      break;
    }

    // 0x08 => exec_warg( const void *x, int arg )
    case 0x08 : {
      if (verbose) PL011_putc( UART0, 'e', true );
      __exec_warg( ctx );
      break;
    }

    // 0x10 => int pipe_open( int id, pid_t read_end, pid_t, write_end )
    case 0x10 : {
      if (verbose) PL011_putc( UART0, 'P', true );
      if (verbose) PL011_putc( UART0, 'o', true );

      // call open pipe procedure and set return value in user process
      ctx->gpr[ 0 ] = __pipe_open();

      break;
    }

    // 0x11 => int pipe_write( int id, uint32_t data )
    case 0x11 : {
      if (verbose) PL011_putc( UART0, 'P', true );
      if (verbose) PL011_putc( UART0, 'w', true );

      // get args
      int        id = ( int      ) ctx->gpr[ 0 ];
      uint32_t data = ( uint32_t ) ctx->gpr[ 1 ];

      // call write to pipe proc and set return value in user process
      ctx->gpr[ 0 ] = __pipe_write( id, data );

      break;
    }

    // 0x12 => int pipe_read( int id, uint32_t buffer )
    case 0x12 : {
      if (verbose) PL011_putc( UART0, 'P', true );
      if (verbose) PL011_putc( UART0, 'r', true );

      // get args
      int          id = ( int ) ctx->gpr[ 0 ];
      int      buffer = ( int ) ctx->gpr[ 1 ];

      // call read from pipe proc and set return value in user process
      ctx->gpr[ 0 ] = __pipe_read( id, buffer );

      break;
    }

    // 0x13 => int pipe_close( int id )
    case 0x13 : {
      if (verbose) PL011_putc( UART0, 'P', true );
      if (verbose) PL011_putc( UART0, 'c', true );

      // get args
      int id = ( int ) ctx->gpr[ 0 ];

      // call pipe close procedure and set return value in user process
      ctx->gpr[ 0 ] = __pipe_close( id );

      break;
    }

    // 0x14 => bool pipe_can_read( int id )
    case 0x14 : {
      if (verbose) PL011_putc( UART0, 'P', true );
      if (verbose) PL011_putc( UART0, 'c', true );

      // get args
      int id = ( int ) ctx->gpr[ 0 ];

      // call the can read check proc and set return value in user process
      ctx->gpr[ 0 ] = __pipe_can_read( id );

      break;
    }

    // 0x15 => bool pipe_can_write( int id )
    case 0x15 : {
      if (verbose) PL011_putc( UART0, 'P', true );
      if (verbose) PL011_putc( UART0, 'c', true );

      // get args
      int id = ( int ) ctx->gpr[ 0 ];

      // call the can write check proc and set return value in user process
      ctx->gpr[ 0 ] = __pipe_can_write( id );

      break;
    }

    // 0x16 => int pipe_assign_read( int id, pid_t pid )
    case 0x16 : {
      if (verbose) PL011_putc( UART0, 'P', true );
      if (verbose) PL011_putc( UART0, 'a', true );

      // get args
      int id = ( int ) ctx->gpr[ 0 ];
      pid_t pid = ( pid_t ) ctx->gpr[ 1 ];

      // call the proc and set return value in user process
      ctx->gpr[ 1 ] = __pipe_assign_read( id, pid );

      break;
    }

    // 0x17 => int pipe_assign_write( int id, pid_t pid )
    case 0x17 : {
      if (verbose) PL011_putc( UART0, 'P', true );
      if (verbose) PL011_putc( UART0, 'a', true );

      // get args
      int id = ( int ) ctx->gpr[ 0 ];
      pid_t pid = ( pid_t ) ctx->gpr[ 1 ];

      // call the proc and set return value in user process
      ctx->gpr[ 1 ] = __pipe_assign_write( id, pid );

      break;
    }

    // 0x?? => unknown/unsupported
    default : {
      if (verbose) PL011_putc( UART0, 'u', true );
      ASSERTFAIL();
      break;
    }
  }

  return;
}
