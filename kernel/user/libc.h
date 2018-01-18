#ifndef __LIBC_H
#define __LIBC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <string.h>


// Define a type that captures a Process IDentifier (PID).

typedef int pid_t;

// Define a type that captures a Pipe Identifier

typedef int pipeid_t;

/* The definitions below capture symbolic constants within these classes:
 *
 * 1. system call identifiers (i.e., the constant used by a system call
 *    to specify which action the kernel should take),
 * 2. signal identifiers (as used by the kill system call),
 * 3. status codes for exit,
 * 4. standard file descriptors (e.g., for read and write system calls),
 * 5. platform-specific constants, which may need calibration (wrt. the
 *    underlying hardware QEMU is executed on).
 *
 * They don't *precisely* match the standard C library, but are intended
 * to act as a limited model of similar concepts.
 */

#define SYS_YIELD      ( 0x00 )
#define SYS_WRITE      ( 0x01 )
#define SYS_READ       ( 0x02 )
#define SYS_FORK       ( 0x03 )
#define SYS_EXIT       ( 0x04 )
#define SYS_EXEC       ( 0x05 )
#define SYS_KILL       ( 0x06 )
#define SYS_PID        ( 0x07 )
#define SYS_EXEC_WARG  ( 0x08 )
#define SYS_PID_PARENT ( 0x09 )

#define SYS_PIPE_OPEN        ( 0x10 )
#define SYS_PIPE_WRITE       ( 0x11 )
#define SYS_PIPE_READ        ( 0x12 )
#define SYS_PIPE_CLOSE       ( 0x13 )
#define SYS_PIPE_CANREAD     ( 0x14 )
#define SYS_PIPE_CANWRITE    ( 0x15 )
#define SYS_PIPE_ASSIGNREAD  ( 0x16 )
#define SYS_PIPE_ASSIGNWRITE ( 0x17 )

#define SIG_TERM       ( 0x00 )
#define SIG_QUIT       ( 0x01 )

#define EXIT_SUCCESS   ( 0 )
#define EXIT_FAILURE   ( 1 )

#define  STDIN_FILENO  ( 0 )
#define STDOUT_FILENO  ( 1 )
#define STDERR_FILENO  ( 2 )

// definitions for nice param to fork, i.e. to choose priority of process
#define NICE    ( 0 )
#define NOTNICE ( 1 )

// convert ASCII string x into integer r
extern int  atoi( char* x        );

// convert integer x into ASCII string r
extern void itoa( char* r, int x );

// cooperatively yield control of processor, i.e., invoke the scheduler
extern void yield();

// write n bytes from x to   the file descriptor fd; return bytes written
extern int write( int fd, const void* x, size_t n );

// read  n bytes into x from the file descriptor fd; return bytes read
extern int  read( int fd,       void* x, size_t n );

// perform fork, returning 0 iff. child or > 0 iff. parent process
extern int  fork( bool nice );

// perform exit, i.e., terminate process with status x
extern void exit(       int   x );

// perform exec, i.e., start executing program at address x
extern void exec( const void* x );

// perform exec, but also pass through a pipeid_t for child process to use
extern void exec_warg( const void* x, pipeid_t pipeid );

// kill process identified by pid. return 0 on success, else negative error code.
extern int kill( pid_t pid );

// return pid of calling process
extern pid_t get_pid();

// return pid of parent process
extern pid_t get_parent_pid();


// ipc

extern pipeid_t pipe_open();

extern bool pipe_can_write( pipeid_t id );

extern pipeid_t pipe_write( pipeid_t id, uint32_t data );

extern bool pipe_can_read( pipeid_t id );

extern int pipe_read( pipeid_t id, uint32_t buffer );

extern pipeid_t pipe_close( pipeid_t id );

extern int pipe_assign_read( pipeid_t id, pid_t pid );

extern int pipe_assign_write( pipeid_t id, pid_t pid );

extern void wait_till_can_write( pipeid_t id );

extern void wait_till_can_read( pipeid_t id );

// io

extern int writeout( char* x );

#endif
