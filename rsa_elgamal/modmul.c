/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "modmul.h"

/* Perform stage 1:
 *
 * - read each 3-tuple of N, e and m from stdin,
 * - compute the RSA encryption c, then
 * - write the ciphertext c to stdout.
 */

void stage1() {

  // read each 3-tuple of N, e and m from stdin
  // 1024-bit line + terminator
  char * N_str = NULL;
  char * e_str = NULL;
  char * m_str = NULL;
  size_t line_size;

  // init mpz_t's for N, e, m
  mpz_t N, e, m;
  mpz_init(N);
  mpz_init(e);
  mpz_init(m);

  // read challenges from stdin forever
  int i = 0;
  while (1) {
    int run = _stage1_read_challenge( &N_str, &e_str, &m_str, &line_size );
    if (!run) return; // no more challenges

    // assign N, e, m, interpreting input as hex integer literals
    if ( mpz_set_str( N, N_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i     );
      return;
    }
    if ( mpz_set_str( e, e_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i + 1 );
      return;
    }
    if ( mpz_set_str( m, m_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i + 2 );
      return;
    }

    // init mpz_t's for ciphertext c
    mpz_t c;
    mpz_init( c );

    // calculate c using RSA
    sliding_window_expm( c, m, e, N );

    // print c to stdout
    gmp_printf( "%ZX\n", c );

    // incr line counter (we parsed 3 lines from stdin this loop)
    i += 3;
  }

  mpz_clear(N);
  mpz_clear(e);
  mpz_clear(m);

}

int _stage1_read_challenge( char ** N_str, char ** e_str, char ** m_str, size_t * line_size ) {
  int run = 1;
  if ( getline(N_str, line_size, stdin) == -1 ||
       getline(e_str, line_size, stdin) == -1 ||
       getline(m_str, line_size, stdin) == -1
     ) {
    run = 0;
  }
  return run;
}


/* Perform stage 2:
 *
 * - read each 9-tuple of N, d, p, q, d_p, d_q, i_p, i_q and c from stdin,
 * - compute the RSA decryption m, then
 * - write the plaintext m to stdout.
 */

void stage2() {

  // 1024-bit line + terminator
  char * N_str   = NULL;
  char * d_str   = NULL;
  char * p_str   = NULL;
  char * q_str   = NULL;
  char * d_p_str = NULL;
  char * d_q_str = NULL;
  char * i_p_str = NULL;
  char * i_q_str = NULL;
  char * c_str   = NULL;
  size_t line_size;

  // init mpz_t's
  mpz_t N, d, p, q, d_p, d_q, i_p, i_q, c, c_p, c_q, m;
  mpz_init(N);
  mpz_init(d);
  mpz_init(p);
  mpz_init(q);
  mpz_init(d_p);
  mpz_init(d_q);
  mpz_init(i_p);
  mpz_init(i_q);
  mpz_init(c);
  mpz_init(c_p);
  mpz_init(c_q);
  mpz_init(m);

  // read challenges from stdin forever
  int i = 0;
  while (1) {
    int run = _stage2_read_challenge( &N_str, &d_str, &p_str, &q_str, &d_p_str, &d_q_str,
                                      &i_p_str, &i_q_str, &c_str, &line_size );
    if (!run) return; // no more challenges

    // assign N, d, c, interpreting input as hex integer literals
    if ( mpz_set_str( N, N_str, 16 ) == -1 )
      fprintf( stderr, "failed to parse line %d\n", i     );
    if ( mpz_set_str( d, d_str, 16 ) == -1 )
      fprintf( stderr, "failed to parse line %d\n", i + 1 );
    if ( mpz_set_str( p, p_str, 16 ) == -1 )
      fprintf( stderr, "failed to parse line %d\n", i + 2 );
    if ( mpz_set_str( q, q_str, 16 ) == -1 )
      fprintf( stderr, "failed to parse line %d\n", i + 3 );
    if ( mpz_set_str( d_p, d_p_str, 16 ) == -1 )
      fprintf( stderr, "failed to parse line %d\n", i + 4 );
    if ( mpz_set_str( d_q, d_q_str, 16 ) == -1 )
      fprintf( stderr, "failed to parse line %d\n", i + 5 );
    if ( mpz_set_str( i_p, i_p_str, 16 ) == -1 )
      fprintf( stderr, "failed to parse line %d\n", i + 6 );
    if ( mpz_set_str( i_q, i_q_str, 16 ) == -1 )
      fprintf( stderr, "failed to parse line %d\n", i + 7 );
    if ( mpz_set_str( c, c_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i + 8 );
      return;
    }

    // calculate m using RSA decryption with CRT

    mpz_mod( c_p, c, p ); // c_p <- c mod p
    mpz_mod( c_q, c, q ); // c_q <- c mod q

    sliding_window_expm( c_p, c_p, d_p, p ); // c_p' <- c_p^d_p mod p
    sliding_window_expm( c_q, c_q, d_q, q ); // c_q' <- c_q^d_q mod q

    mpz_mul( c_p, c_p, q );
    mpz_mul( c_p, c_p, i_q ); // c_p'' <- c_p' * q * i_q
    mpz_mul( c_q, c_q, p );
    mpz_mul( c_q, c_q, i_p ); // c_q'' <- c_q' * p * i_q

    mpz_add( m, c_p, c_q );
    mpz_mod( m, m, N );     // m <- c_p'' * c_q'' mod N TODO: faster to reduce at each stage above

    // print c to stdout
    gmp_printf( "%ZX\n", m );

    // incr line counter (we parsed 9 lines from stdin this loop)
    i += 9;
  }

  mpz_clear(N);
  mpz_clear(d);
  mpz_clear(p);
  mpz_clear(q);
  mpz_clear(d_p);
  mpz_clear(d_q);
  mpz_clear(i_p);
  mpz_clear(i_q);
  mpz_clear(c);
  mpz_clear(c_p);
  mpz_clear(c_q);
  mpz_clear(m);

}

int _stage2_read_challenge( char ** N_str, char ** d_str, char ** p_str, char ** q_str, char ** d_p_str,
                             char ** d_q_str, char ** i_p_str, char ** i_q_str, char ** c_str,
                             size_t * line_size ) {

  int run = 1;

  if ( getline(N_str,   line_size, stdin) == -1 ||
       getline(d_str,   line_size, stdin) == -1 ||
       getline(p_str,   line_size, stdin) == -1 ||
       getline(q_str,   line_size, stdin) == -1 ||
       getline(d_p_str, line_size, stdin) == -1 ||
       getline(d_q_str, line_size, stdin) == -1 ||
       getline(i_p_str, line_size, stdin) == -1 ||
       getline(i_q_str, line_size, stdin) == -1 ||
       getline(c_str,   line_size, stdin) == -1
  ) {
    run = 0;
  }

  return run;
}

/* Perform stage 3:
 *
 * - read each 5-tuple of p, q, g, h and m from stdin,
 * - compute the ElGamal encryption c = (c_1,c_2), then
 * - write the ciphertext c to stdout.
 */

void stage3() {

  // read each 5-tuple of p, q, g, h and m from stdin
  // 1024-bit line + terminator
  char * p_str = NULL;
  char * q_str = NULL;
  char * g_str = NULL;
  char * h_str = NULL;
  char * m_str = NULL;
  size_t line_size;

  // init mpz_t's for N, e, m
  mpz_t p, q, g, h, m;
  mpz_init(p);
  mpz_init(q);
  mpz_init(g);
  mpz_init(h);
  mpz_init(m);

  // init a rand state
  gmp_randstate_t state;
  gmp_randinit_mt( state ); // use Mersenne Twister as a PRNG

  // seed the rand state
  mpz_t seed;
  mpz_init( seed );
  get_random_seed( seed ); // get a seed using /dev/urandom
  gmp_randseed( state, seed ); // seed the gmp_randstate_t

  // read challenges from stdin forever
  int i = 0;
  while (1) {
    int run = _stage3_read_challenge( &p_str, &q_str, &g_str, &h_str, &m_str, &line_size );
    if (!run) return; // no more challenges

    // assign p, q, g, h, m interpreting input as hex integer literals
    if ( mpz_set_str( p, p_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i     );
      return;
    }
    if ( mpz_set_str( q, q_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i + 1 );
      return;
    }
    if ( mpz_set_str( g, g_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i + 2 );
      return;
    }
    if ( mpz_set_str( h, h_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i + 3 );
      return;
    }
    if ( mpz_set_str( m, m_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i + 4 );
      return;
    }

    // init mpz_t's for ciphertext c
    mpz_t c1, c2;
    mpz_init( c1 );
    mpz_init( c2 );

    // choose ephermal key k = [0..q-1]
    mpz_t k;
    mpz_init( k );
    mpz_urandomm(k, state, q);
    //mpz_set_ui( k, 1 );

    // calculate c1 using ElGamal: c1 = g^k mod p
    sliding_window_expm( c1, g, k, p );

    // calculate c2 using ElGamal: c2 = m*h^k mod p
    sliding_window_expm( h, h, k, p ); // h'  <- h^k mod p
    mpz_mul( c2, m, h );        // c2' <- m*h'
    mpz_mod( c2, c2, p);        // c2  <- c2' mod p

    // print c to stdout
    gmp_printf( "%ZX\n", c1 );
    gmp_printf( "%ZX\n", c2 );

    // clear k
    mpz_clear( k );

    i += 5;
  }

  mpz_clear(p);
  mpz_clear(q);
  mpz_clear(g);
  mpz_clear(h);
  mpz_clear(m);

}

int _stage3_read_challenge( char ** p_str, char ** q_str, char ** g_str, char ** h_str, char ** m_str,
                            size_t * line_size ) {

  int run = 1;

  if ( getline(p_str,   line_size, stdin) == -1 ||
       getline(q_str,   line_size, stdin) == -1 ||
       getline(g_str,   line_size, stdin) == -1 ||
       getline(h_str,   line_size, stdin) == -1 ||
       getline(m_str,   line_size, stdin) == -1
  ) {
    run = 0;
  }

  return run;
}


/* Perform stage 4:
 *
 * - read each 5-tuple of p, q, g, x and c = (c_1,c_2) from stdin,
 * - compute the ElGamal decryption m, then
 * - write the plaintext m to stdout.
 */

void stage4() {

  // read each 5-tuple of p, q, g, h and m from stdin
  // 1024-bit line + terminator
  char * p_str  = NULL;
  char * q_str  = NULL;
  char * g_str  = NULL;
  char * x_str  = NULL;
  char * c1_str = NULL;
  char * c2_str = NULL;
  size_t line_size;

  // init mpz_t's for p, q, g, x, c1, c2
  mpz_t p, q, g, x, c1, c2;
  mpz_init(p);
  mpz_init(q);
  mpz_init(g);
  mpz_init(x);
  mpz_init(c1);
  mpz_init(c2);

  // read challenges from stdin forever
  int i = 0;
  while (1) {
    int run = _stage4_read_challenge( &p_str, &q_str, &g_str, &x_str, &c1_str, &c2_str, &line_size );
    if (!run) return; // no more challenges

    // assign p, q, g, x, c1, c2, interpreting input as hex integer literals
    if ( mpz_set_str( p, p_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i     );
      return;
    }
    if ( mpz_set_str( q, q_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i + 1 );
      return;
    }
    if ( mpz_set_str( g, g_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i + 2 );
      return;
    }
    if ( mpz_set_str( x, x_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i + 3 );
      return;
    }
    if ( mpz_set_str( c1, c1_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i + 4 );
      return;
    }
    if ( mpz_set_str( c2, c2_str, 16 ) == -1 ) {
      fprintf( stderr, "failed to parse line %d\n", i + 5 );
      return;
    }

    // init mpz_t of decrypted plaintext m
    mpz_t m;
    mpz_init( m );

    // calculate m using ElGamal: m = c2 * c1^-x
    // 1. c1 <- c1^-x mod p
    sliding_window_expm( c1, c1, x, p );
    mpz_invert( c1, c1, p );

    // 2. m <- c1^-x * c2 mod p
    mpz_mul( m, c2, c1 );
    mpz_mod( m,  m, p  );
    //mulm( m, c2, c1, p ); NOTE this implementation of montgomery multiplication is broken

    // print m to stdout
    gmp_printf( "%ZX\n", m );

    i += 6;
  }

  mpz_clear(p);
  mpz_clear(q);
  mpz_clear(g);
  mpz_clear(x);
  mpz_clear(c1);
  mpz_clear(c2);

}

int _stage4_read_challenge( char ** p_str, char ** q_str, char ** g_str, char ** x_str, char ** c1_str,
                            char ** c2_str, size_t * line_size ) {

  int run = 1;

  if ( getline(p_str ,   line_size, stdin) == -1 ||
       getline(q_str ,   line_size, stdin) == -1 ||
       getline(g_str ,   line_size, stdin) == -1 ||
       getline(x_str ,   line_size, stdin) == -1 ||
       getline(c1_str,   line_size, stdin) == -1 ||
       getline(c2_str,   line_size, stdin) == -1
  ) {
    run = 0;
  }

  return run;
}


/*********************************************************************************************************************/

/* Perform stages and optimisations on fixed inputs for testing.
 * This will be piped to a file to compare to Python implementation
 */
void tests( char * test ) {
  if ( strcmp( test, "stage1" ) == 0 )
    stage1();
  else if ( strcmp( test, "stage2" ) == 0 )
    stage2();
  else if ( strcmp( test, "stage3" ) == 0 )
    stage3();
  else if ( strcmp( test, "stage4" ) == 0 )
    stage4();
  else if ( strcmp( test, "sw" ) == 0 )
    ;//test_sliding_window();
  else
    fprintf( stderr, "unrecognised test\n" );
 }


/* The main function acts as a driver for the assignment by simply invoking the
 * correct function for the requested stage.
 */

int main( int argc, char* argv[] ) {
  if( 2 != argc ) {
    abort();
  }

  if     ( !strcmp( argv[ 1 ], "stage1" ) ) {
    stage1();
  }
  else if( !strcmp( argv[ 1 ], "stage2" ) ) {
    stage2();
  }
  else if( !strcmp( argv[ 1 ], "stage3" ) ) {
    stage3();
  }
  else if( !strcmp( argv[ 1 ], "stage4" ) ) {
    stage4();
  }
  else if( !strcmp( argv[ 1 ], "test"   ) ) {
    tests( "stage1" );
  }
  else {
    abort();
  }

  return 0;
}




//**********************************************************************************************************************
// Cryptographically Secure Random Number Generation                                                                  **
//**********************************************************************************************************************
void get_random_seed( mpz_t seed ) {
  // ~bytes of entropy we're trying to achieve (1024-bit, same as p)
  int byte_count = 1024 / 8;

  // allocate a buffer of this size
  char data[ byte_count ];

  // read this many bytes from urandom into the buffer
  FILE *fp;
  fp = fopen("/dev/urandom", "rb");
  fread( &data, 1, byte_count, fp );
  fclose(fp);

  fprintf( stderr, "rand seed:\n%s\n", data );

  // parse the buffer and use this to assign our mpz_t variable
  //          rop   count       order  size  endian  nails  op
  mpz_import( seed, byte_count,     1,    1,      0,     0, data );
}


//**********************************************************************************************************************
// Sliding Window Exponentiation                                                                                      **
//**********************************************************************************************************************
const mp_bitcnt_t k = 4; // k <- MAX_SLIDING_WINDOW_SIZE
const int sw_debug = 0;  // turn on to print verbose info for debugging in sliding window exp functions

// Performs modular exponentiation in the group Z_N.
// @param r the result an elem of Z_N. r = b^e mod N
// @param b the base an elem of Z_N
// @param e the exponenent, an integer
// @param N the modulus
void sliding_window_expm( mpz_t r, mpz_t b, mpz_t e, mpz_t N ) {
  // precompute T = [ b^[j] mod N | j=1,3,..., 2^k - 1 ]
  size_t table_n = pow( 2, k-1 ); // k-1 because we only need odd elems
  mpz_t T[table_n];
  sliding_window_expm_precompute_T( T, table_n, b, N, k );

  // init return value to group identity
  mpz_set_ui( r, 1 );

  // init loop vars we will use
  int         i, // current ix of exponent e we are at
              l, // ix of last elem in the window
              u, // the value of the sliding window between indices i and l, i.e. e[i..l]
              w; // the size of the window

  // start of most significant bit of exponent e (when e represented in base-2)
  size_t tmp = mpz_sizeinbase( e, 2 ) - 1;
  if (tmp > INT_MAX) fprintf(stderr, "ERROR: sliding_window_expm: mpz_sizeinbase(exponent,2) > INT_MAX\n");
  i = (int) tmp;

  // we start at i = most signigicant bit of e
  // l is always a less than or equal sig bit of e
  // in gmp, e[0] is the least significant bit

  // keep finding windows until we've gone through all bits of e
  while (i >= 0) {

    int sliding = 0;

    // if e[i] is 0, then use a window of size 1 with value 0
    if ( mpz_tstbit( e, i ) == 0 ) {
      l = i;
      u = 0;
    }

    // else if e[i] is 1, then we want to find a sliding window of size >= 1.
    else {
      sliding = 1;

      if (sw_debug) fprintf(stderr, "NEW ITER with window_size>1\ni %d\n", i);

      // first, set the index of the end of window to the max window size
      l = ( i > k - 1 ) ? i - (k - 1) : 0;  // max( i-k+1, 0 )

      // next, iterate towards i until we find a 1.
      // this index will become the end of the window
      mp_bitcnt_t e_l;
      while ( l <= i ) {
        e_l = mpz_tstbit( e, l );
        if ( e_l == 1 )
          break;
        l++;
      }
      if (sw_debug) fprintf(stderr, "l %d\n", l);

      // set u <- e[i..l]
      u = 0;
      int j = 0;
      if (sw_debug) fprintf(stderr, "e[i..l] ");

      for ( int ix = l; ix <= i; ix++ ) {
        u += (1 << j) * mpz_tstbit( e, (mp_bitcnt_t)ix ); // u_j = 2^j * e[ix]
        j++;

        if (sw_debug) fprintf(stderr, "%d", mpz_tstbit(e,(mp_bitcnt_t)ix));
      }
      if (sw_debug) fprintf(stderr, "\n");
      if (sw_debug) fprintf(stderr, "u       %d\n", u);
    }

    w = (i-l+1);

    // now, multiply return value r by 2^(window_size)
    // r <- r^( 2^(window_size) )
    for ( int j = 0; j < w; j++ ) {
      mpz_mul( r, r, r );
      mpz_mod( r, r, N );
    }

    // if the value of the window u is nonzero, then add b^(u) to r
    if ( u != 0 ) {
      // get the b^(u) using the precomputed table T
      int ix = ( u - 1 ) / 2;
      if (sw_debug && sliding == 1) fprintf(stderr, "ix %d\n", ix);

      // r <- r * b^(u) mod N
      mpz_mul( r, r, T[ix] );
      mpz_mod( r, r, N );
    }

    // finally, set i to start of next window for the next iter
    i = l - 1;
  }
}


// precomputes T = [ b^[j] mod N | j=1,3,...,2^k-1 ]
void sliding_window_expm_precompute_T( mpz_t * T, size_t n, mpz_t b, mpz_t N, mp_bitcnt_t k ) {
  // init and assign the first elem to b^1=b
  mpz_init( T[0] );
  mpz_set( T[0], b );
  mpz_mod( T[0], T[0], N );

  if (sw_debug) gmp_fprintf(stderr, "T[0]=%Zd\n", T[0]);

  // for a T[i-1]=b^j, then T[i]=b^(j+2)
  for ( int i = 1; i <= n; i++ ) {
    // T[i] <- T[i-1] * b^2 mod N
    mpz_init( T[i] );
    mpz_mul( T[i], b, b );
    mpz_mul( T[i], T[i], T[i-1] );
    mpz_mod( T[i], T[i], N );

    if (sw_debug) gmp_fprintf(stderr, "T[%d]=%Zd\n", i, T[i]);
  }
}


//**********************************************************************************************************************
// Montgomery Multiplication                                                                                          **
//**********************************************************************************************************************
void mulm( mpz_t r, mpz_t x, mpz_t y, mpz_t N ) {
  // 1. precompute the Montgomery params omega and rho squared
  mpz_t omega, rho_sqrd;
  mpz_init( omega );
  mpz_init( rho_sqrd );

  mpz_t b;  // the word size i.e. base we are working in
  mpz_init_set_ui( b, mp_bits_per_limb );

  // omega
  mpz_powm_ui( omega, N, (1 << (mp_bits_per_limb-1)) - 1, b ); // NOTE can optimise? mod b is equiv to taking the least significant limb
  mpz_neg( omega, omega );
  mpz_add( omega, omega, b ); // omega <- -( N^( 2^( w-1 ) - 1 ) ) mod b

  // rho^2
  size_t l_N = mpz_size( N );
  mpz_t exponent, two;
  mpz_init( exponent );
  mpz_init_set_ui( two, 2 );
  mpz_mul_ui( exponent, omega, 2*l_N );
  mpz_powm( rho_sqrd, two, exponent, N ); // rho^2 <- 2^( 2*l_N*omega ) mod N

  // 2. calculate x_hat and y_hat  (x and y in Montgomery representation) using ZN-MontMul({x,y}_hat, rho^2 mod N)
  mpz_t x_hat, y_hat;
  mpz_init( x_hat ); mpz_init( y_hat );
  size_t omega_N = mpz_size( omega );
  Z_N_montmul( x_hat, x, rho_sqrd, N, l_N, omega, omega_N );
  Z_N_montmul( y_hat, y, rho_sqrd, N, l_N, omega, omega_N );

  // 3. calculate r_hat=ZN-MontMul(x_hat,y_hat) (r=x*y in Montgomery representation)
  mpz_t r_hat;
  mpz_init( r_hat );
  Z_N_montmul( r_hat, x_hat, y_hat, N, l_N, omega, omega_N );

  // 4. calculate r=ZN-MontMul(r_hat,1) (r_hat in standard base-b representation)
  mpz_t unit;
  mpz_init_set_ui( unit, 1 );
  Z_N_montmul( r, r_hat, unit, N, l_N, omega, omega_N );
}

void Z_N_montmul( mpz_t r, mpz_t x, mpz_t y, mpz_t N, size_t l_N, mpz_t omega, size_t omega_N ) {
  // set r to zero
  mpz_set_ui( r, 0 );

  //int b = mp_bits_per_limb; // the base we are working in

  // get the limb arrays of x, N, r and omega
  const mp_limb_t * x_limbs = mpz_limbs_modify( x, l_N );
  const mp_limb_t * N_limbs = mpz_limbs_modify( N, l_N );
  const mp_limb_t * r_limbs     = mpz_limbs_modify( r, l_N ); // resize to l_N
  const mp_limb_t * omega_limbs = mpz_limbs_modify( omega, omega_N );

  // limbs that we need for the calculations in loop body
  mp_limb_t u_i;
  mp_limb_t r_0, x_0, y_i;

  // limb arrays that we need for calculations in loop body
  mp_limb_t tmp0[ 2 + omega_N ];
  mp_limb_t tmp1[ l_N + 1 ];
  mp_limb_t tmp2[ l_N + 1 ];
  mp_limb_t tmp3[ l_N + 1 ];
  mp_limb_t tmp4[ l_N + 1 ];

  for ( size_t i = 0; i < l_N; i++ ) {
    r_0 = mpz_getlimbn( r, 0 );
    x_0 = mpz_getlimbn( x, 0 );
    y_i = mpz_getlimbn( y, i );

    // calculate the 'magic' u_i
    mpn_mul_n( tmp0, &y_i, &x_0, 1 );               //                y_i * x_0
    mpn_add_n( tmp0, &r_0, tmp0, 2 );               //         r_0 + (y_i * x_0)
    mpn_mul( tmp0, tmp0, 2, omega_limbs, omega_N ); //        (r_0 + (y_i * x_0)) * omega
    u_i = tmp0[ 0 ];                                // u_i <- (r_0 + (y_i * x_0)) * omega (mod b), where b is the base

    // calculate r at this iter
    mpn_mul( tmp1, &y_i, 1, x_limbs, l_N );       //           y_i * x
    mpn_mul( tmp2, &u_i, 1, N_limbs, l_N );       //                       u_i * N
    mpn_add_n( tmp3,    tmp1, tmp2, l_N + 1 );    //          (y_i * x) + (u_i * N)
    mpn_add_n( tmp4, r_limbs, tmp3, l_N + 1 );    //      r + (y_i * x) + (u_i * N)
    shift_limbs_1( r_limbs, tmp4, l_N + 1 );      // r <- r + (y_i * x) + (u_i * N) / b
  }

  // ensure r is in range 0 <= r < N
  if ( mpz_cmp( r, N ) >= 0 ) {
    mpz_sub( r, r, N );
  }
}

void shift_limbs_1( const mp_limb_t * out, const mp_limb_t * in, size_t N_in ) {
  // assert N_out == N_in - 1

  int i = 1; // loop through in
  int j = 0; // loop through out

  while ( j < N_in ) {
    memcpy( (void *) out + j, (void *) in + i, sizeof( mp_limb_t ) );
    i++; j++;
  }
}
