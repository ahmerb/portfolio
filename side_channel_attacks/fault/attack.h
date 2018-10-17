/* Ahmer Butt <ab15015@bristol.ac.uk>
 *
 * COMS30901 Applied Security
 * University of Bristol
 *
 * A fault attack against AES.
 */

#ifndef __ATTACK_H
#define __ATTACK_H

#include <stdbool.h>
#include  <stdint.h>
#include   <stdio.h>
#include  <stdlib.h>

#include  <string.h>
#include  <signal.h>
#include  <unistd.h>
#include   <fcntl.h>
#include    <time.h>

// for testing key hypotheses
#include <openssl/aes.h>
#include         <omp.h>

// an element in AES key or state matrix
typedef uint8_t byte;

// 2^(sizeof(byte))
#define BYTE_N 256


#endif
