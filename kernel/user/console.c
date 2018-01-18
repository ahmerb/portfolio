#include "console.h"

/* The following functions are special-case versions of a) writing,
 * and b) reading a string from the UART (the latter case returning
 * once a carriage return character has been read, or an overall
 * limit reached).
 */

void puts( char* x, int n ) {
  for( int i = 0; i < n; i++ ) {
    PL011_putc( UART1, x[ i ], true );
  }
}

void gets( char* x, int n ) {
  for( int i = 0; i < n; i++ ) {
    x[ i ] = PL011_getc( UART1, true );

    if( x[ i ] == '\x0A' ) {
      x[ i ] = '\x00';
      break;
    }
  }
}

/* Since we lack a *real* loader (as a result of lacking a storage
 * medium to store program images), the following approximates one:
 * given a program name, from the set of programs statically linked
 * into the kernel image, it returns a pointer to the entry point.
 */

extern void main_P3();
extern void main_P4();
extern void main_P5();
extern void main_waiter();
extern void main_test();

void* load( char* x ) {
  if     ( 0 == strcmp( x, "P3" ) ) {
    return &main_P3;
  }
  else if( 0 == strcmp( x, "P4" ) ) {
    return &main_P4;
  }
  else if( 0 == strcmp( x, "P5" ) ) {
    return &main_P5;
  }
  else if( 0 == strcmp( x, "phils" ) ) {
    return &main_waiter;
  }
  else if( 0 == strcmp( x, "test" ) ) {
    return &main_test;
  }

  return NULL;
}

/* The behaviour of the console process can be summarised as an
 * (infinite) loop over three main steps, namely
 *
 * 1. write a command prompt then read a command,
 * 2. split the command into space-separated tokens using strtok,
 * 3. execute whatever steps the command dictates.
 */

void main_console() {
  puts( "\n", 1 );

  // x = user command, p = first word in user command, nice = toggle whether to be nicer when assigning priority
  char* p, x[ 1024 ];

  while( 1 ) {
    puts( "shell$ ", 7 );
    gets( x, 1024 );
    p    = strtok( x, " " );

    // command: fork P{3,4,5} [nice?] ( fork, then exec given process if successfully forked )
    if ( 0 == strcmp( p, "fork" ) ) {
      void* addr = load( strtok( NULL, " " ) );
      bool  is_nice = strcmp( strtok( NULL, " " ), "nice" ) ? NICE : NOTNICE;
      pid_t pid;

      // skip this command if we didn't find the user process
      if ( addr == NULL ) {
        puts( "unknown user process\n", 21 );
        continue;
      }

      pid  = fork( is_nice );

      // after fork call, we have to instances of this process executing right here.
      // If child  process, fork returned 0. If parent process, fork returned PID of child.
      // If parent process, we want to just keep running the shell, so continue loop.
      // If child  process, we want to replace context with context of desired process to execute, with exec trap.
      if ( 0 == pid ) {
        exec( addr );
      }
    }

    // command: kill
    else if ( 0 == strcmp( p, "kill" ) ) {
      pid_t pid = atoi( strtok( NULL, " " ) );
      //int   s   = atoi( strtok( NULL, " " ) );

      int error = kill( pid ); //, s );
      if ( error != 0 )
        puts( "no process with given pid\n", 26 );
    }

    // command: unknown
    else {
      puts( "unknown command\n", 16 );
    }
  }

  exit( EXIT_SUCCESS );
}
