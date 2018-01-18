#include "libc.h"

int  atoi( char* x        ) {
  char* p = x; bool s = false; int r = 0;

  if     ( *p == '-' ) {
    s =  true; p++;
  }
  else if( *p == '+' ) {
    s = false; p++;
  }

  for( int i = 0; *p != '\x00'; i++, p++ ) {
    r = s ? ( r * 10 ) - ( *p - '0' ) :
            ( r * 10 ) + ( *p - '0' ) ;
  }

  return r;
}

void itoa( char* r, int x ) {
  char* p = r; int t, n;

  if( x < 0 ) {
    p++; t = -x; n = 1;
  }
  else {
         t = +x; n = 1;
  }

  while( t >= n ) {
    p++; n *= 10;
  }

  *p-- = '\x00';

  do {
    *p-- = '0' + ( t % 10 ); t /= 10;
  } while( t );

  if( x < 0 ) {
    *p-- = '-';
  }

  return;
}

void yield() {
  asm volatile( "svc %0     \n" // make system call SYS_YIELD
              :
              : "I" (SYS_YIELD)
              : );

  return;
}

int write( int fd, const void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_WRITE
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_WRITE), "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int  read( int fd,       void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_READ
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_READ),  "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int  fork( bool nice ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = nice
                "svc %1     \n" // make system call SYS_FORK
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_FORK),  "r" (nice)
              : "r0" );

  return r;
}

void exit( int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  x
                "svc %0     \n" // make system call SYS_EXIT
              :
              : "I" (SYS_EXIT), "r" (x)
              : "r0" );

  return;
}

void exec( const void* x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 = x
                "svc %0     \n" // make system call SYS_EXEC
              :
              : "I" (SYS_EXEC), "r" (x)
              : "r0" );

  return;
}

void exec_warg( const void* x, int arg ) {
  asm volatile( "mov r0, %1 \n" // assign r0 = x
                "mov r1, %2 \n" // assign r1 = pipeid
                "svc %0     \n" // make system call SYS_EXEC
              :
              : "I" (SYS_EXEC_WARG), "r" (x), "r" (arg)
              : "r0", "r1");

  return;
}

int kill( pid_t pid ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 =  pid
                "svc %1     \n" // make system call SYS_KILL
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r)
              : "I"  (SYS_KILL), "r" (pid)
              : "r0" );

  return r;
}

pid_t get_pid() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_PID
                "mov %0, r0 \n" // assign r0 = r
              : "=r" (r)
              : "I"  (SYS_PID)
              : "r0" );


  return ( pid_t ) r;
}

pid_t get_parent_pid() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_PID_PARENT
                "mov %0, r0 \n" // assign r0 = r
              : "=r" (r)
              : "I"  (SYS_PID_PARENT)
              : "r0" );

  return ( pid_t ) r;
}


// ipc

pipeid_t pipe_open() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_PIPE_OPEN
                "mov %0, r0 \n" // assign r = r0
              : "=r" (r)
              : "I"  (SYS_PIPE_OPEN)
              : "r0" );

  return r;
}

bool pipe_can_write( pipeid_t id ) {
  bool r;

  asm volatile( "mov r0, %2 \n" // assign r0 = id
                "svc %1     \n" // make system call SYS_PIPE_CANWRITE
                "mov %0, r0 \n" // assign r = r0
              : "=r" (r)
              : "I"  (SYS_PIPE_CANWRITE), "r" (id)
              : "r0" );

  return r;
}

pipeid_t pipe_write( pipeid_t id, uint32_t data ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = id
                "mov r1, %3 \n" // assign r1 = data
                "svc %1     \n" // make system call SYS_PIPE_WRITE
                "mov %0, r0 \n" // assign r = r0
              : "=r" (r)
              : "I"  (SYS_PIPE_WRITE), "r" (id), "r" (data)
              : "r0", "r1" );

  return r;
}

bool pipe_can_read( pipeid_t id ) {
  bool r;

  asm volatile( "mov r0, %2 \n" // assign r0 = id
                "svc %1     \n" // make system call SYS_PIPE_CANREAD
                "mov %0, r0 \n" // assign r = r0
              : "=r" (r)
              : "I"  (SYS_PIPE_CANREAD), "r" (id)
              : "r0" );

  return r;
}

int pipe_read( pipeid_t id, uint32_t buffer ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = id
                "mov r1, %3 \n" // assign r1 = buffer
                "svc %1     \n" // make system call SYS_PIPE_READ
                "mov %0, r0 \n" // assign r = r0
              : "=r" (r)
              : "I"  (SYS_PIPE_READ), "r" (id), "r" (buffer)
              : "r0", "r1" );

  return r;
}

pipeid_t pipe_close( int id ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = id
                "svc %1     \n" // make system call SYS_PIPE_CLOSE
                "mov %0, r0 \n" // assign r = r0
              : "=r" (r)
              : "I"  (SYS_PIPE_CLOSE), "r" (id)
              : "r0" );

  return r;
}

int pipe_assign_read( pipeid_t id, pid_t pid ) {
  int r;

  asm volatile( "mov r0, %2 \n"
                "mov r1, %3 \n"
                "svc %1     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "I" (SYS_PIPE_ASSIGNREAD), "r" (id), "r" (pid)
              : "r0", "r1" );

  return r;
}

int pipe_assign_write( pipeid_t id, pid_t pid ) {
  int r;

  asm volatile( "mov r0, %2 \n"
                "mov r1, %3 \n"
                "svc %1     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "I" (SYS_PIPE_ASSIGNWRITE), "r" (id), "r" (pid)
              : "r0", "r1" );

  return r;
}

void wait_till_can_write( pipeid_t id ) {
  while ( !pipe_can_write( id ) ) { continue; }
}

void wait_till_can_read( pipeid_t id ) {
  while ( !pipe_can_read( id ) ) { continue; }
}

// io

int writeout( char* x ) {
  size_t n = strlen( x );
  return write( STDOUT_FILENO, ( const void* ) x, n );
}
