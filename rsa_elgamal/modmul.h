/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __MODMUL_H
#define __MODMUL_H

#include       <stdio.h>
#include      <stdlib.h>

#include        <math.h>
#include      <limits.h>

#include      <string.h>
#include         <gmp.h>


// helpers for stage1-4
int _stage1_read_challenge( char ** N_str, char ** e_str, char ** m_str, size_t * line_size );

int _stage2_read_challenge( char ** N_str, char ** d_str, char ** p_str, char ** q_str, char ** d_p_str,
                             char ** d_q_str, char ** i_p_str, char ** i_q_str, char ** c_str,
                             size_t * line_size );

int _stage3_read_challenge( char ** p_str, char ** q_str, char ** g_str, char ** h_str, char ** m_str,
                            size_t * line_size );

int _stage4_read_challenge( char ** p_str, char ** q_str, char ** g_str, char ** x_str, char ** c1_str,
                            char ** c2_str, size_t * line_size );


// csprng
void get_random_seed( mpz_t seed );


// sliding window exponentiation
void sliding_window_expm( mpz_t r, mpz_t b, mpz_t e, mpz_t N );

void sliding_window_expm_precompute_T( mpz_t * T, size_t n, mpz_t b, mpz_t N, mp_bitcnt_t k );

// Montgomery multiplication
void mulm( mpz_t r, mpz_t x, mpz_t y, mpz_t N );

void Z_N_montmul( mpz_t r, mpz_t x, mpz_t y, mpz_t N, size_t l_N, mpz_t omega, size_t omega_N );

void shift_limbs_1( const mp_limb_t * out, const mp_limb_t * in, size_t N_in );




#endif
