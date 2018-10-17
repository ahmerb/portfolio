/* Ahmer Butt <ab15015@bristol.ac.uk>
 *
 * COMS30901 Applied Security
 * University of Bristol
 *
 * A power attack against AES.
 */

#ifndef __ATTACK_H
#define __ATTACK_H

#include     <stdbool.h>
#include      <stdint.h>
#include       <stdio.h>
#include      <stdlib.h>

#include      <string.h>
#include      <signal.h>
#include      <unistd.h>
#include       <fcntl.h>
#include        <time.h>
#include        <math.h>

// test AES keys
#include <openssl/aes.h>

// scanf a uint8_t
#include    <inttypes.h>

// parallelise
#include         <omp.h>

// an element in AES key or state matrix
typedef uint8_t byte;

#endif
