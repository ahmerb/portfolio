/* Ahmer Butt <ab15015@bristol.ac.uk>
 *
 * COMS30901 Applied Security
 * University of Bristol
 *
 * A fault attack against AES using a single fault.
 */

#include "attack.h"


/*
*** Helpers
*/

void print_bytes(byte arr[], int start, int end) {
  for (int i = start; i < end; i++) {
    printf("%02x", arr[i]);
  }
  printf("\n");
}




/*
 *** AES-Rijndael
*/

// SBox (from wiki)
const byte SBox[256] = {
   0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
   0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
   0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
   0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
   0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
   0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
   0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
   0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
   0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
   0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
   0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
   0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
   0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
   0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
   0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
   0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

// Inverted SBox (from wiki)
const byte SBoxInv[256] = {
  0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
  0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
  0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
  0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
  0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
  0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
  0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
  0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
  0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
  0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
  0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
  0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
  0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
  0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
  0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
  0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

// Rijndael Key Schedule (from wiki)
const byte Rcon[256] = {
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a,
    0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39,
    0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a,
    0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8,
    0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef,
    0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc,
    0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b,
    0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3,
    0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94,
    0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20,
    0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35,
    0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f,
    0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04,
    0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63,
    0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd,
    0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d
};


void inv_round_key_one_round_in_place(byte k[17], int r) {
  k[16] ^=  k[12];
  k[15] ^=  k[11];
  k[14] ^=  k[10];
  k[13] ^=  k[9];
  k[12] ^=  k[8];
  k[11] ^=  k[7];
  k[10] ^=  k[6];
  k[9]  ^=  k[5];
  k[8]  ^=  k[4];
  k[7]  ^=  k[3];
  k[6]  ^=  k[2];
  k[5]  ^=  k[1];
  k[4]  ^=  SBox[k[13]];
  k[3]  ^=  SBox[k[16]];
  k[2]  ^=  SBox[k[15]];
  k[1]  ^=  SBox[k[14]] ^ Rcon[r];
}

void inv_tenth_round_key_in_place(byte key[17]) {
  for (int i = 10; i > 0; i--) {
    inv_round_key_one_round_in_place(key, i);
  }
}

bool test_key(byte k_test[16], byte m[16], byte c_true[16]) {
  // set the AES_KEY to the test key
  AES_KEY k;
  AES_set_encrypt_key(k_test, 128, &k);

  // encrypt m under test key and store in c
  byte c[16];
  AES_encrypt(m, c, &k);

  // compare c to c_true
  if (0 == memcmp(c, c_true, 16 * sizeof(byte)))
    return true;
  else
    return false;
}




/*
 *** Group operations in GF(2^8)
*/

byte gf28_add( byte a, byte b ) {
  return a ^ b;
}

byte gf28_sub( byte a, byte b ) {
  return a ^ b;
}

byte gf28_mult( byte a, byte b ) {
  byte r = 0;
  byte carryBit;
  for ( int i = 0; i < 8; i ++ ) {

    if ( b & 1 )
      r ^= a;

    carryBit = a & 0x80;

    a <<= 1;

    if ( carryBit == 0x80 )
      a ^= 0x1b;

    b >>= 1;

  }
  return r & 0xFF;
}


byte MulTab[256][256]; // multiplication table

void precompute_gf28_mult_table(void) {
  printf("\nPrecomputing multiplication table\n");
  for (int i = 0; i < 256; i++) {
    for (int j = 0; j < 256; j++) {
      MulTab[i][j] = gf28_mult(i, j);
    }
  }
  printf("...Done\n\n");
}




/*
 *** Communicate with attack target
*/

#define BUFFER_SIZE ( 80 )

pid_t pid        = 0;    // process ID (of either parent or child) from fork

int   target_raw[ 2 ];   // unbuffered communication: attacker -> attack target
int   attack_raw[ 2 ];   // unbuffered communication: attack target -> attacker

FILE* target_out = NULL; // buffered attack target input  stream
FILE* target_in  = NULL; // buffered attack target output stream


void interact(byte m[17], byte c[18], char fault[10]) {
  // send fault specification to the attack target
  fprintf(target_in, "%s\n", fault);
  fflush(target_in);

  // send 1-block AES plaintext, represented as an
  // octet string. send newline instead of null term.
  for (int i = 0; i < 16; i++) {
    fprintf(target_in, "%02X", m[i]);
  }
  fprintf(target_in, "\n");
  fflush(target_in);

  // receive the 1-block AES ciphertext
  c[0] = 0;
  for (int i = 1; i < 17; i++) {
    int e = fscanf(target_out, "%2hhX", &c[i]);
    if (1 != e) {
      printf("e = %d\n", e);
      abort();
    }
  }
  // append null term
  c[17] = '\0';
}




/*
 *** Cleanup on complete
*/

void cleanup( int s ) {
  // close the buffered communication handles
  fclose( target_in  );
  fclose( target_out );

  // close the unbuffered communication handles
  close( target_raw[ 0 ] );
  close( target_raw[ 1 ] );
  close( attack_raw[ 0 ] );
  close( attack_raw[ 1 ] );

  // forcibly terminate the attack target process
  if( pid > 0 ) {
    kill( pid, SIGKILL );
  }

  // forcibly terminate the attacker process.
  exit( 1 );
}




/*
 *** KeyHypotheses objects and associated methods.
 ***  - these structs is used to store all the possible key hypotheses that
 ***    we find during Stage 1 of the attack.
*/

typedef struct {
  int set1; int set2; int set3; int set4;
} KeyHypothesesNs;

typedef struct {
  byte k1;
  byte k8;
  byte k11;
  byte k14;
} Set1Hypothesis;

Set1Hypothesis * init_set1_hypothesis(byte k1, byte k8, byte k11, byte k14) {
  Set1Hypothesis * x = malloc(sizeof(Set1Hypothesis));
  x->k1 = k1; x->k8 = k8; x->k11 = k11; x->k14 = k14;
  return x;
}

void print_set1(Set1Hypothesis * x) {
  printf("<%02x,%02x,%02x,%02x>", x->k1, x->k8, x->k11, x->k14);
}

typedef struct {
  byte k2;
  byte k5;
  byte k12;
  byte k15;
} Set2Hypothesis;

Set2Hypothesis * init_set2_hypothesis(byte k2, byte k5, byte k12, byte k15) {
  Set2Hypothesis * x = malloc(sizeof(Set2Hypothesis));
  x->k2 = k2; x->k5 = k5; x->k12 = k12; x->k15 = k15;
  return x;
}

void print_set2(Set2Hypothesis * x) {
  printf("<%02x,%02x,%02x,%02x>", x->k2, x->k5, x->k12, x->k15);
}

typedef struct {
  byte k3;
  byte k6;
  byte k9;
  byte k16;
} Set3Hypothesis;

Set3Hypothesis * init_set3_hypothesis(byte k3, byte k6, byte k9, byte k16) {
  Set3Hypothesis * x = malloc(sizeof(Set3Hypothesis));
  x->k3 = k3; x->k6 = k6; x->k9 = k9; x->k16 = k16;
  return x;
}

void print_set3(Set3Hypothesis * x) {
  printf("<%02x,%02x,%02x,%02x>", x->k3, x->k6, x->k9, x->k16);
}

typedef struct {
  byte k4;
  byte k7;
  byte k10;
  byte k13;
} Set4Hypothesis;

Set4Hypothesis * init_set4_hypothesis(byte k4, byte k7, byte k10, byte k13) {
  Set4Hypothesis * x = malloc(sizeof(Set4Hypothesis));
  x->k4 = k4; x->k7 = k7; x->k10 = k10; x->k13 = k13;
  return x;
}

void print_set4(Set4Hypothesis * x) {
  printf("<%02x,%02x,%02x,%02x>", x->k4, x->k7, x->k10, x->k13);
}

typedef struct {
 Set1Hypothesis ** set1;
 Set2Hypothesis ** set2;
 Set3Hypothesis ** set3;
 Set4Hypothesis ** set4;
 KeyHypothesesNs ns;
} KeyHypotheses;

KeyHypotheses * init_key_hypotheses(void) {
  KeyHypotheses * x = malloc(sizeof(KeyHypotheses));
  x->set1 = malloc(512 * sizeof(Set1Hypothesis *)); // NOTE, still possibly more than 512
  x->set2 = malloc(512 * sizeof(Set2Hypothesis *));
  x->set3 = malloc(512 * sizeof(Set3Hypothesis *));
  x->set4 = malloc(512 * sizeof(Set4Hypothesis *));
  memset(&(x->ns), 0, sizeof(KeyHypothesesNs));
  return x;
}

void free_key_hypotheses(KeyHypotheses * x) {
  KeyHypothesesNs ns = x->ns;
  for (int i = 0; i < ns.set1; i++) {
    free(x->set1[i]);
  }
  for (int i = 0; i < ns.set2; i++) {
    free(x->set2[i]);
  }
  for (int i = 0; i < ns.set3; i++) {
    free(x->set3[i]);
  }
  for (int i = 0; i < ns.set4; i++) {
    free(x->set4[i]);
  }
 free(x->set1);
 free(x->set2);
 free(x->set3);
 free(x->set4);
 free(x);
}

void print_key_hypotheses(KeyHypotheses * x) {
  printf("set1<k1,k8,k11,k14> (n=%03d): ", x->ns.set1);
  for (int i = 0; i < x->ns.set1; i++) {
   print_set1(x->set1[i]); printf(" ");
  }
  printf("\n\n");
  printf("set2<k2,k5,k12,k15> (n=%03d):", x->ns.set2);
  for (int i = 0; i < x->ns.set2; i++) {
   print_set2(x->set2[i]); printf(" ");
  }
  printf("\n\n");
  printf("set3<k3,k6,k9,k16> (n=%03d): ", x->ns.set3);
  for (int i = 0; i < x->ns.set3; i++) {
   print_set3(x->set3[i]); printf(" ");
  }
  printf("\n\n");
  printf("set4<k4,k7,k10,k13> (n=%03d): ", x->ns.set4);
  for (int i = 0; i < x->ns.set4; i++) {
   print_set4(x->set4[i]); printf(" ");
  }
  printf("\n\n");
}


/*
 *** Attack Stage 1
*/

byte equation(byte c, byte c_faulty, byte k) {
  return gf28_add(SBoxInv[gf28_add(c, k)], SBoxInv[gf28_add(c_faulty, k)]);
}


void stage1_equations1(byte c[18], byte c_faulty[18], KeyHypotheses * keys) {
  // select a value of 0<delta1<256 and determine which values of k1, k8, k11, k14
  // satisfy the system of equations.
  // If delta1 = 0, then the fault has not been injected, so attack cannot continue.

  for (int delta1 = 1; delta1 < BYTE_N; delta1++) {
    // the max size of each is 4 key hypotheses
    byte *  k1s = malloc(4 * sizeof(byte));
    byte *  k8s = malloc(4 * sizeof(byte));
    byte * k11s = malloc(4 * sizeof(byte));
    byte * k14s = malloc(4 * sizeof(byte));

    // equation1: exhaustive search for values of k1 for this delta
    int i_k1s = 0;
    for (int k1 = 0; k1 < BYTE_N; k1++) {
      if (MulTab[delta1][2] == equation(c[1], c_faulty[1], k1)) {
        k1s[i_k1s] = k1;
        i_k1s++;
      }
    }

    // if any of the eqs cannot be satisfied for this delta, then this
    // delta is an impossible differential, so skip
    if (i_k1s == 0) {
      continue;
    }

    // equation2: exhaustive search for values of k8
    int i_k8s = 0;
    for (int k8 = 0; k8 < BYTE_N; k8++) {
      if (MulTab[delta1][3] == equation(c[8], c_faulty[8], k8)) {
        k8s[i_k8s] = k8;
        i_k8s++;
      }
    }

    // impossible differential
    if (i_k8s == 0) {
      continue;
    }

    // equation3: exhaustive search for values of k11
    int i_k11s = 0;
    for (int k11 = 0; k11 < BYTE_N; k11++) {
      if (delta1 == equation(c[11], c_faulty[11], k11)) {
        k11s[i_k11s] = k11;
        i_k11s++;
      }
    }

    // impossible differential
    if (i_k11s == 0) {
      continue;
    }

    // equation4: exhaustive search for values of k14
    int i_k14s = 0;
    for (int k14 = 0; k14 < BYTE_N; k14++) {
      if (delta1 == equation(c[14], c_faulty[14], k14)) {
        k14s[i_k14s] = k14;
        i_k14s++;
      }
    }

    // impossible differential
    if (i_k14s == 0) {
      continue;
    }

    // we now have possible values for k1,k8,k11,k14 for this delta
    // add all possible key combinations to the list of possible solns
    for (int i = 0; i < i_k1s; i++) {
      for (int j = 0; j < i_k8s; j++) {
        for (int k = 0; k < i_k11s; k++) {
          for (int l = 0; l < i_k14s; l++) {
            keys->set1[keys->ns.set1] = init_set1_hypothesis(k1s[i], k8s[j], k11s[k], k14s[l]);
            keys->ns.set1 = keys->ns.set1 + 1;
            //if (keys->ns.set1 >= 255) printf("****BIG FAIL****\n");
          }
        }
      }
    }

    // free the arrays we made for this loop iter
    free(k1s); free(k8s); free(k11s); free(k14s);
  }
}


void stage1_equations2(byte c[18], byte c_faulty[18], KeyHypotheses * keys) {
  // select a value of 0<delta2<256 and determine which values of k2, k5, k12, k15
  // satisfy the system of equations.
  // If delta2 = 0, then the fault has not been injected, so attack cannot continue.

  for (int delta2 = 1; delta2 < BYTE_N; delta2++) {
    // the max size of each is 4 key hypotheses
    byte *  k2s = malloc(4 * sizeof(byte));
    byte *  k5s = malloc(4 * sizeof(byte));
    byte * k12s = malloc(4 * sizeof(byte));
    byte * k15s = malloc(4 * sizeof(byte));

    // equation1: exhaustive search for values of k2 for this delta
    int i_k2s = 0;
    for (int k2 = 0; k2 < BYTE_N; k2++) {
      if (delta2 == equation(c[2], c_faulty[2], k2)) {
        k2s[i_k2s] = k2;
        i_k2s++;
      }
    }

    // if any of the eqs cannot be satisfied for this delta, then this
    // delta is an impossible differential, so skip
    if (i_k2s == 0) {
      continue;
    }

    // equation2: exhaustive search for values of k5
    int i_k5s = 0;
    for (int k5 = 0; k5 < BYTE_N; k5++) {
      if (delta2 == equation(c[5], c_faulty[5], k5)) {
        k5s[i_k5s] = k5;
        i_k5s++;
      }
    }

    // impossible differential
    if (i_k5s == 0) {
      continue;
    }

    // equation3: exhaustive search for values of k12
    int i_k12s = 0;
    for (int k12 = 0; k12 < BYTE_N; k12++) {
      if (MulTab[delta2][2] == equation(c[12], c_faulty[12], k12)) {
        k12s[i_k12s] = k12;
        i_k12s++;
      }
    }

    // impossible differential
    if (i_k12s == 0) {
      continue;
    }

    // equation4: exhaustive search for values of k15
    int i_k15s = 0;
    for (int k15 = 0; k15 < BYTE_N; k15++) {
      if (MulTab[delta2][3] == equation(c[15], c_faulty[15], k15)) {
        k15s[i_k15s] = k15;
        i_k15s++;
      }
    }

    // impossible differential
    if (i_k15s == 0) {
      continue;
    }

    // we now have possible values for k2,k5,k12,k15 for this delta
    // add all possible key combinations to the list of possible solns
    for (int i = 0; i < i_k2s; i++) {
      for (int j = 0; j < i_k5s; j++) {
        for (int k = 0; k < i_k12s; k++) {
          for (int l = 0; l < i_k15s; l++) {
            keys->set2[keys->ns.set2] = init_set2_hypothesis(k2s[i], k5s[j], k12s[k], k15s[l]);
            keys->ns.set2 = keys->ns.set2 + 1;
            //if (keys->ns.set2 >= 255) printf("****BIG FAIL****\n");
          }
        }
      }
    }

    // free loop iter arrays
    free(k2s); free(k5s); free(k12s); free(k15s);
  }
}


void stage1_equations3(byte c[18], byte c_faulty[18], KeyHypotheses * keys) {
  // select a value of 0<delta2<256 and determine which values of k3, k6, k9, k16
  // satisfy the system of equations.
  // If delta3 = 0, then the fault has not been injected, so attack cannot continue.

  for (int delta3 = 1; delta3 < BYTE_N; delta3++) {
    // the max size of each is 4 key hypotheses
    byte *  k3s = malloc(4 * sizeof(byte));
    byte *  k6s = malloc(4 * sizeof(byte));
    byte *  k9s = malloc(4 * sizeof(byte));
    byte * k16s = malloc(4 * sizeof(byte));

    // equation1: exhaustive search for values of k3 for this delta
    int i_k3s = 0;
    for (int k3 = 0; k3 < BYTE_N; k3++) {
      if (MulTab[delta3][2] == equation(c[3], c_faulty[3], k3)) {
        k3s[i_k3s] = k3;
        i_k3s++;
      }
    }

    // if any of the eqs cannot be satisfied for this delta, then this
    // delta is an impossible differential, so skip
    if (i_k3s == 0) {
      continue;
    }

    // equation2: exhaustive search for values of k6
    int i_k6s = 0;
    for (int k6 = 0; k6 < BYTE_N; k6++) {
      if (gf28_mult(delta3, 3) == equation(c[6], c_faulty[6], k6)) {
        k6s[i_k6s] = k6;
        i_k6s++;
      }
    }

    // impossible differential
    if (i_k6s == 0) {
      continue;
    }

    // equation3: exhaustive search for values of k9
    int i_k9s = 0;
    for (int k9 = 0; k9 < BYTE_N; k9++) {
      if (delta3 == equation(c[9], c_faulty[9], k9)) {
        k9s[i_k9s] = k9;
        i_k9s++;
      }
    }

    // impossible differential
    if (i_k9s == 0) {
      continue;
    }

    // equation4: exhaustive search for values of k16
    int i_k16s = 0;
    for (int k16 = 0; k16 < BYTE_N; k16++) {
      if (delta3 == equation(c[16], c_faulty[16], k16)) {
        k16s[i_k16s] = k16;
        i_k16s++;
      }
    }

    // impossible differential
    if (i_k16s == 0) {
      continue;
    }

    // we now have possible values for k3,k6,k9,k16 for this delta
    // add all possible key combinations to the list of possible solns
    for (int i = 0; i < i_k3s; i++) {
      for (int j = 0; j < i_k6s; j++) {
        for (int k = 0; k < i_k9s; k++) {
          for (int l = 0; l < i_k16s; l++) {
            keys->set3[keys->ns.set3] = init_set3_hypothesis(k3s[i], k6s[j], k9s[k], k16s[l]);
            keys->ns.set3 = keys->ns.set3 + 1;
            //if (keys->ns.set3 >= 255) printf("****BIG FAIL****\n");
          }
        }
      }
    }

    // free loop iter arrays
    free(k3s); free(k6s); free(k9s); free(k16s);
  }
}


void stage1_equations4(byte c[18], byte c_faulty[18], KeyHypotheses * keys) {
  // select a value of 0<delta2<256 and determine which values of k4, k7, k10, k13
  // satisfy the system of equations.
  // If delta4 = 0, then the fault has not been injected, so attack cannot continue.

  for (int delta4 = 1; delta4 < BYTE_N; delta4++) {
    // the max size of each is 4 key hypotheses
    byte *  k4s = malloc(4 * sizeof(byte));
    byte *  k7s = malloc(4 * sizeof(byte));
    byte * k10s = malloc(4 * sizeof(byte));
    byte * k13s = malloc(4 * sizeof(byte));

    // equation1: exhaustive search for values of k4 for this delta
    int i_k4s = 0;
    for (int k4 = 0; k4 < BYTE_N; k4++) {
      if (delta4 == equation(c[4], c_faulty[4], k4)) {
        k4s[i_k4s] = k4;
        i_k4s++;
      }
    }

    // if any of the eqs cannot be satisfied for this delta, then this
    // delta is an impossible differential, so skip
    if (i_k4s == 0) {
      continue;
    }

    // equation2: exhaustive search for values of k7
    int i_k7s = 0;
    for (int k7 = 0; k7 < BYTE_N; k7++) {
      if (delta4 == equation(c[7], c_faulty[7], k7)) {
        k7s[i_k7s] = k7;
        i_k7s++;
      }
    }

    // impossible differential
    if (i_k7s == 0) {
      continue;
    }

    // equation3: exhaustive search for values of k10
    int i_k10s = 0;
    for (int k10 = 0; k10 < BYTE_N; k10++) {
      if (MulTab[delta4][2] == equation(c[10], c_faulty[10], k10)) {
        k10s[i_k10s] = k10;
        i_k10s++;
      }
    }

    // impossible differential
    if (i_k10s == 0) {
      continue;
    }

    // equation4: exhaustive search for values of k13
    int i_k13s = 0;
    for (int k13 = 0; k13 < BYTE_N; k13++) {
      if (MulTab[delta4][3] == equation(c[13], c_faulty[13], k13)) {
        k13s[i_k13s] = k13;
        i_k13s++;
      }
    }

    // impossible differential
    if (i_k13s == 0) {
      continue;
    }

    // we now have possible values for k4,k7,k10,k13 for this delta
    // add all possible key combinations to the list of possible solns
    for (int i = 0; i < i_k4s; i++) {
      for (int j = 0; j < i_k7s; j++) {
        for (int k = 0; k < i_k10s; k++) {
          for (int l = 0; l < i_k13s; l++) {
            keys->set4[keys->ns.set4] = init_set4_hypothesis(k4s[i], k7s[j], k10s[k], k13s[l]);
            keys->ns.set4 = keys->ns.set4 + 1;
            //if (keys->ns.set4 >= 255) printf("****BIG FAIL****\n");
          }
        }
      }
    }

    // free loop iter arrays
    free(k4s); free(k7s); free(k10s); free(k13s);
  }
}


void stage1(byte c[18], byte c_faulty[18], KeyHypotheses * key_hypotheses) {

  // find constrains of key values using the system of eqs from paper
  stage1_equations1(c, c_faulty, key_hypotheses);
  stage1_equations2(c, c_faulty, key_hypotheses);
  stage1_equations3(c, c_faulty, key_hypotheses);
  stage1_equations4(c, c_faulty, key_hypotheses);

  // // remove any duplicate key hypotheses
  // remove_duplicate_keys(key_hypotheses);

  printf("\nKey hypotheses after Stage 1:\n");
  print_key_hypotheses(key_hypotheses);
}




/*
 *** Attack Stage 2
*/


bool stage2_eqs(byte c[18], byte c_faulty[18], byte k[18]) {
  byte a = SBoxInv[MulTab[14][SBoxInv[c[1] ^ k[1]] ^ k[1] ^ SBox[k[14] ^ k[10]] ^ Rcon[10]] ^ MulTab[11][SBoxInv[c[14] ^ k[14]] ^ k[2] ^ SBox[k[15] ^ k[11]]] ^ MulTab[13][SBoxInv[c[11] ^ k[11]] ^ k[3] ^ SBox[k[16] ^ k[12]]] ^ MulTab[9][SBoxInv[c[8] ^ k[8]] ^ k[4] ^ SBox[k[13] ^ k[9]]]] ^ SBoxInv[MulTab[14][SBoxInv[c_faulty[1] ^ k[1]] ^ k[1] ^ SBox[k[14] ^ k[10]] ^ Rcon[10]] ^ MulTab[11][SBoxInv[c_faulty[14] ^ k[14]] ^ k[2] ^ SBox[k[15] ^ k[11]]] ^ MulTab[13][SBoxInv[c_faulty[11] ^ k[11]] ^ k[3] ^ SBox[k[16] ^ k[12]]] ^ MulTab[9][SBoxInv[c_faulty[8] ^ k[8]] ^ k[4] ^ SBox[k[13] ^ k[9]]]];

  byte f1 = MulTab[a][3];

  byte b = SBoxInv[MulTab[9][SBoxInv[c[13] ^ k[13]] ^ k[13] ^ k[9]] ^ MulTab[14][SBoxInv[c[10] ^ k[10]] ^ k[10] ^ k[14]] ^ MulTab[11][SBoxInv[c[7] ^ k[7]] ^ k[15] ^ k[11]] ^ MulTab[13][SBoxInv[c[4] ^ k[4]] ^ k[16] ^ k[12]]] ^ SBoxInv[MulTab[9][SBoxInv[c_faulty[13] ^ k[13]] ^ k[13] ^ k[9]] ^ MulTab[14][SBoxInv[c_faulty[10] ^ k[10]] ^ k[10] ^ k[14]] ^ MulTab[11][SBoxInv[c_faulty[7] ^ k[7]] ^ k[15] ^ k[11]] ^ MulTab[13][SBoxInv[c_faulty[4] ^ k[4]] ^ k[16] ^ k[12]]];

  byte f2 = MulTab[b][6];

  if (f1 != f2) {
      return false;
  }

  byte d = SBoxInv[MulTab[13][SBoxInv[c[9] ^ k[9]] ^ k[9] ^ k[5]] ^ MulTab[9][SBoxInv[c[6] ^ k[6]] ^ k[10] ^ k[6]] ^ MulTab[14][SBoxInv[c[3] ^ k[3]] ^ k[11] ^ k[7]] ^ MulTab[11][SBoxInv[c[16] ^ k[16]] ^ k[12] ^ k[8]]] ^ SBoxInv[MulTab[13][SBoxInv[c_faulty[9] ^ k[9]] ^ k[9] ^ k[5]] ^ MulTab[9][SBoxInv[c_faulty[6] ^ k[6]] ^ k[10] ^ k[6]] ^ MulTab[14][SBoxInv[c_faulty[3] ^ k[3]] ^ k[11] ^ k[7]] ^ MulTab[11][SBoxInv[c_faulty[16] ^ k[16]] ^ k[12] ^ k[8]]];

  byte f3 = MulTab[d][6];

  if (f2 != f3) {
      return false;
  }

  byte e = SBoxInv[MulTab[11][SBoxInv[c[5] ^ k[5]] ^ k[5] ^ k[1]] ^ MulTab[13][SBoxInv[c[2] ^ k[2]] ^ k[6] ^ k[2]] ^ MulTab[9][SBoxInv[c[15] ^ k[15]] ^ k[7] ^ k[3]] ^ MulTab[14][SBoxInv[c[12] ^ k[12]] ^ k[8] ^ k[4]]] ^ SBoxInv[MulTab[11][SBoxInv[c_faulty[5] ^ k[5]] ^ k[5] ^ k[1]] ^ MulTab[13][SBoxInv[c_faulty[2] ^ k[2]] ^ k[6] ^ k[2]] ^ MulTab[9][SBoxInv[c_faulty[15] ^ k[15]] ^ k[7] ^ k[3]] ^ MulTab[14][SBoxInv[c_faulty[12] ^ k[12]] ^ k[8] ^ k[4]]];

  byte f4 = MulTab[e][2];

  if (f3 != f4) {
      return false;
  }

  return true;
}


void stage2(byte msg[17], byte c[18], byte c_faulty[18], KeyHypotheses * key_hypotheses) {
  // loop through every single possible key combination
    // for this combination, check if the equations hold
    // if so,
      // invert this round key to get actual key
      // test this key encrypts m to get c
      // if test passes,
      //  we have found the key and we are done.

  bool found_key = false;
  KeyHypothesesNs ns = key_hypotheses->ns;

  printf("Stage 2\n");

  volatile bool done_flag = false; // tell all threads to skip all computation when done
  #pragma omp parallel for num_threads(omp_get_max_threads()) schedule(static) shared(done_flag)
  for (int i1 = 0; i1 < ns.set1; i1++) {
    if (done_flag) continue;
    for (int i2 = 0; i2 < ns.set2; i2++) {
      if (done_flag) continue;
      for (int i3 = 0; i3 < ns.set3; i3++) {
        if (done_flag) continue;
        for (int i4 = 0; i4 < ns.set4; i4++) {
          if (done_flag) continue;

          // set the key bytes for this combination
          byte k_guess[17];
          k_guess[0] = 0;
          k_guess[1] = key_hypotheses->set1[i1]->k1;
          k_guess[2] = key_hypotheses->set2[i2]->k2;
          k_guess[3] = key_hypotheses->set3[i3]->k3;
          k_guess[4] = key_hypotheses->set4[i4]->k4;
          k_guess[5] = key_hypotheses->set2[i2]->k5;
          k_guess[6] = key_hypotheses->set3[i3]->k6;
          k_guess[7] = key_hypotheses->set4[i4]->k7;
          k_guess[8] = key_hypotheses->set1[i1]->k8;
          k_guess[9] = key_hypotheses->set3[i3]->k9;
          k_guess[10] = key_hypotheses->set4[i4]->k10;
          k_guess[11] = key_hypotheses->set1[i1]->k11;
          k_guess[12] = key_hypotheses->set2[i2]->k12;
          k_guess[13] = key_hypotheses->set4[i4]->k13;
          k_guess[14] = key_hypotheses->set1[i1]->k14;
          k_guess[15] = key_hypotheses->set2[i2]->k15;
          k_guess[16] = key_hypotheses->set3[i3]->k16;

          // see if it satisfies the equations
          bool pass = stage2_eqs(c, c_faulty, k_guess);

          // if so, then check if this key is correct
          if (pass) {
            //printf("A key passed Stage 2\n");

            // get AES key from tenth round key
            inv_tenth_round_key_in_place(k_guess);

            // test the key works
            found_key = test_key(k_guess+1, msg, c+1); // +1's to remove the padding

            // if it does then we are done!
            if (found_key) {
              printf("\n\nFound the key. \nk = ");
              print_bytes(k_guess, 1, 17);
              printf("\n");
              done_flag = true;
            }
          }
        }
      }
    }
  }

  if (!found_key) {
    printf("FAILED TO FIND KEY.");
  }
}




/*
 *** Launch Attack
*/

void attack() {
  byte m[17] = { 0 }; // 16 bytes +1 for null terminator
  byte c[18] = { 0 }; // 16 bytes +1 for null terminator +1 for zero padding at start
  byte c_faulty[18] = { 0 }; // the zero padding at start making indexing for consistent with keys

  // precompute multiplication table for GF(2^8)
  precompute_gf28_mult_table();

  // generate a random message (C's rand function is adequate here)
  srand(time(NULL));
  for ( int i = 0; i < 16; i++ ) {
    m[i] = rand() % 254 + 1;
  }
  m[16] = '\0';

  // set the fault spec:
  //  8th round,
  //  in SubBytes round fn,
  //  before the round fn,
  //  at row i=0,
  //  at column j=0,
  char fault_spec[10] = "8,1,0,0,0";
  char empty_spec[]   = "";

  // receive the faulty and correct ciphertexts from D
  // if c==c_faulty i.e. fault is zero, then try again
  int interactions = 0;
  while (0 == memcmp(c, c_faulty, 17 * sizeof(byte))) {
    interact(m, c_faulty, fault_spec);
    interact(m,        c, empty_spec);
    interactions += 2;
  }

  // print all of the inputs and outputs.
  printf("\nStarting attack with following params\n");
  printf("m        = "); print_bytes(m, 0, 16);
  printf("c        = "); print_bytes(c, 0, 18);
  printf("c_faulty = "); print_bytes(c_faulty, 0, 18);
  printf("\n");

  // init a KeyHypotheses object
  KeyHypotheses * key_hypotheses = init_key_hypotheses();

  // stage 1 of attack
  stage1(c, c_faulty, key_hypotheses);

  // stage 2 of attack
  stage2(m, c, c_faulty, key_hypotheses);

  printf("total interactions: %d\n", interactions);

  // done. now, free KeyHypotheses object
  printf("\nFreeing resources...\n");
  free_key_hypotheses(key_hypotheses);
  printf("...done\n");
}




/*
 *** Main
*/

int main( int argc, char* argv[] ) {
  // ensure we clean-up correctly if Control-C (or similar) is signalled.
  signal( SIGINT, &cleanup );

  // create pipes to/from attack target device
  if( pipe( target_raw ) == -1 ) {
    abort();
  }
  if( pipe( attack_raw ) == -1 ) {
    abort();
  }

  // fork process so we can exec the attack target
  pid = fork();

  if ( pid >  0 ) { // parent
    // construct handles to attack target standard input and output.
    if( ( target_out = fdopen( attack_raw[ 0 ], "r" ) ) == NULL ) {
      abort();
    }
    if( ( target_in  = fdopen( target_raw[ 1 ], "w" ) ) == NULL ) {
      abort();
    }

    // execute the attack
    printf("Starting attack...\n");
    attack();
  }

  else if ( pid == 0 ) { // child
    // (re)connect standard input and output to pipes.
    close( STDOUT_FILENO );
    if( dup2( attack_raw[ 1 ], STDOUT_FILENO ) == -1 ) {
      abort();
    }
    close(  STDIN_FILENO );
    if( dup2( target_raw[ 0 ],  STDIN_FILENO ) == -1 ) {
      abort();
    }

    // produce a sub-process representing the attack target.
    execl( argv[ 1 ], argv[ 0 ], NULL );
  }

  else if ( pid <  0 ) { // error
    // the fork failed; reason is stored in errno, but we'll just abort.
    abort();
  }

  // clean up any resources we've hung on to.
  printf("\nFinished. Cleaning up...\n");
  cleanup( SIGINT );

  return 0;
}
