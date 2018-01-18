// only gives permission to one philosopher at a time
// move to next philosopher when one philosopher has two forks

#include "waiter.h"

// make the waiter be verbose
bool talk = true;

// print errors
bool printerrors = true;

// NUMPHILOSOPHERS == NUMFORKS
#define NUMPHILOSOPHERS 16

// this will be correct if the console spawns the waiter first,
// we know this only because we wrote the kernel's algorithm to assign pids.
// its okay to do this as the point of this user process is just to showcase IPC.
// NOTE: we don't use this anymore, we have get_pid syscall now.
#define WAITER_PID 2

#define THINK   0
#define EAT     1

// philosopher process address
extern void main_philosopher();

// array to keep pids of each philosoher process
pid_t philosophers[ NUMPHILOSOPHERS ];

// array to keep pipeid's of pipe to each philosoher proccess
pipeid_t channels[ NUMPHILOSOPHERS ];


/* waiter */
void main_waiter( void ) {
  if (talk) writeout( "main_waiter\n" );

  // capture the waiter (parents) pid
  pid_t waiter_pid = get_pid();

  // 1) spawn philosoher processes with a pipe to each
  for ( int i = 0; i < NUMPHILOSOPHERS; i++ ) {

    if (talk) writeout( "fork phil\n" );

    // open a pipe
    channels[ i ] = pipe_open();

    // fork to create child
    philosophers[ i ] = fork( NOTNICE );

    // CHILD: (philosopher) is exec'd, with pipeid
    if ( philosophers[ i ] == 0 ) {
      if (talk) writeout( "exec phil\n" );
      exec_warg( &main_philosopher, channels[ i ] ); // EXEC
    }

    // PARENT: (waiter) assigns itself as writer
    int error = pipe_assign_write( channels[ i ], waiter_pid );
    if ( error < 0 && printerrors ) writeout( "error waiter:59" );

  }

  // 2) Forever, send instructions to philosophers.
  //    Loop through all philo's, only allowing one to eat at one time.
  writeout( "waiter process will now send instructions\n" );
  while ( 1 ) {
    // give permission to one philosopher at a time to eat
    for ( int i = 0; i < NUMPHILOSOPHERS; i++ ) {
      for ( int j = 0; j < NUMPHILOSOPHERS; j++ ) {
        int data  = THINK;
        int error = -3;

        // choose whether to tell philo j to eat or think
        data = ( i == j ) ? EAT : THINK;

        // block until we can write
        wait_till_can_write( philosophers[ j ] );

        // write to pipe
        error = pipe_write( channels[ j ], data );
        if ( error < 0 && printerrors ) writeout( "error: waiter.c: 79\n" );

        // block again until philo reads it
        wait_till_can_write( philosophers[ j ] );
      }
    }
  }

  writeout( "waiter exit\n" );
  exit( EXIT_SUCCESS );

}
