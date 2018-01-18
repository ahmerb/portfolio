#include "philosopher.h"

bool loud = true;

#define THINK   0
#define EAT     1

/* table_fork */
typedef enum {
  on_table,
  picked_up
} table_fork;

table_fork pick_up( table_fork fork ) {
  if ( fork == on_table ) fork = picked_up;
  return fork;
}

table_fork put_down( table_fork fork ) {
  if ( fork == picked_up ) fork = on_table;
  return fork;
}

/* philosopher */

void main_philosopher( pipeid_t pipeid ) {
  if (loud) {
    writeout( "main_philosopher\n" );
    writeout( "pipeid = " );
    char buffer[ 20 ];      // string to pretty print the read value into
    itoa( buffer, pipeid ); // convert read value into string
    writeout( buffer );     // write out read value
    writeout( "\n" );
  }

  // capture pid
  pid_t pid = get_pid();

  // assign oneself as the read_end of the pipe
  int error = pipe_assign_read( pipeid, pid );
  if ( error < 0 ) { while (1) writeout( "error phils:41" ); }

  // block until we can read from the pipe
  while ( !pipe_can_read( pipeid ) ) { continue; }

  // forever, read instr from pipe and do as told
  while ( 1 ) {
    int buffer = EAT;

    // read
    buffer = pipe_read( pipeid, buffer );
    if ( buffer == EAT ) {
      writeout( " | EAT  " );
    }
    else {
      writeout( " | THINK" );
    }

    // block until we can read again ( waiter has written again )
    while ( !pipe_can_read( pipeid ) ) { continue; }
  }


  if (loud) writeout( "philosoher exit\n" );
  exit( EXIT_SUCCESS );
}
