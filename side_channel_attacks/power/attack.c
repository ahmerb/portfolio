/* Ahmer Butt <ab15015@bristol.ac.uk>
 *
 * COMS30901 Applied Security
 * University of Bristol
 *
 * A differential power analysis attack against XTS-AES.blockDec.
 */

#include "attack.h"

/**

TODO's

 ** Restart with more data from target if attack fails
     |-> this doesn't work properly but it doesn't matter because we have a reasoned
         approach to choosing the parameters that should guarentee key recover with
         >0.9999 probability (see ../marksheet.txt).

**/


/* PSEUDO for DPA on AES-Enc

# stage1 is choosing f as SBox(d_i ^ k_j)

# stage2
generate D random plaintexts.
encrypt them (one time each?) using 21452.D and get results + power traces.
  store traces in the matrix T.

for key_byte_ix in 1..16:

  let ks = [ all 256 possible 8 bit bitstrings (i.e. the keybyte) ]

  for every d_i in D:
    for every k_j in ks:
      let V[i][j] = SBox(d_i ^ k_j)        # stage3
      let H[i][j] = HammingWeight(V[i][j]) # stage4

      # stage5
      for each column (ix i) hypo_power_consumption_for_key_k in H:
        for each column (ix j) real_power_consumption_at_time_t in T:
          let r = correlation_coeff(H[][i], T[][j])
          R[i][j] = r

      sum correlations across rows of R
      let the row with hightest sum be i
      then, the key at position key_byte_ix is i.
*/



/*
 *** Hamming Weight Lookup table
*/

// taken from online but I can't find the webpage again :S
const byte hamming_weight[256] = {
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};



/*
*** Helpers
*/

int min(int x, int y) {
  return x < y ? x : y;
}

void print_bytes(byte arr[], int start, int end) {
  for (int i = start; i < end; i++) {
    printf("%02x", arr[i]);
  }
  printf("\n");
}

void print_floats(float arr[], int start, int end) {
  for (int i = start; i < end; i++) {
    printf("%.6f,", arr[i]);
  }
  printf("\n");
}



/*
 *** AES
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

void aes_enc(byte m[16], byte k[16], byte c[16]) {
  AES_KEY enc_key;
  AES_set_encrypt_key(k, 128, &enc_key);
  AES_encrypt(m, c, &enc_key);
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
 *** Cleanup on complete
*/

void cleanup(int s) {
  // close the buffered communication handles
  fclose(target_in);
  fclose(target_out);

  // close the unbuffered communication handles
  close(target_raw[ 0 ]);
  close(target_raw[ 1 ]);
  close(attack_raw[ 0 ]);
  close(attack_raw[ 1 ]);

  // forcibly terminate the attack target process
  if(pid > 0) {
    kill(pid, SIGKILL);
  }

  // forcibly terminate the attacker process.
  exit(1);
}


/*
 *** Attack on k2: Setup
 ***  - Generate sector address i's and interact with device
*/


void generate_is(int n, byte i[n][16]) {
  printf("\nGenerating %d random valid sector addresses...\n", n);

  // set seed for rand
  srand(time(NULL));

  // make n random sector addresses i
  for (int ix = 0; ix < n; ix++) {
    // gen 16 NON-ZERO octets (i will be invalid)
    for (int j = 0; j < 16; j++) {
      int x = rand() % 256;
      while (x == 0) x = rand() % 256;
      i[ix][j] = x;
    }
  }
  printf("...done\n");
}

void get_power_traces(int num_i, int num_power_samples, byte m[num_i][16], uint8_t traces_start[num_power_samples][num_i], uint8_t traces_end[num_power_samples][num_i], byte i[num_i][16]) {
  printf("\nInteracting with target D to get power traces of len %d\n", num_power_samples);

  // choose j specifically so that, for k1 attack, alpha^j==1
  int j_block = 0;

  int e; // store return values of scanf calls

  // for each i, send (j,i) to D and receive trace and plaintext
  for (int ix = 0; ix < num_i; ix++) {
    // send block address j to att target D
    fprintf(target_in, "%u\n", j_block);
    fflush(target_in);

    // send i
    for (int j = 0; j < 16; j++)
      fprintf(target_in, "%02X", i[ix][j]);
    fprintf(target_in, "\n");
    fflush(target_in);

    // read power trace length
    int l;
    e = fscanf(target_out, "%d", &l);
    if (1 != e) abort();

    // read power trace values
    uint8_t trace[l];
    for (int j = 0; j < l; j++) {
      e = fscanf(target_out, ",%" SCNu8, &trace[j]);
      if (1 != e) abort();
    }
    fscanf(target_out,"%*[^\n]"); // get anything left on line

    if (num_power_samples > l)
      abort(); // NOTE maybe deal with this better

    // read the first and last num_power_samples from trace
    for (int j = 0; j < num_power_samples; j++) {
      traces_start[j][ix] = trace[j];
      traces_end[j][ix]   = trace[l - num_power_samples + j];
    }

    // read and discard plaintext
    for (int j = 0; j < 16; j++) {
      e = fscanf(target_out, "%2hhx", &m[ix][j]);
      if (1 != e) abort();
    }
  }
  printf("...done\n");
}


/*
 *** DPA Attack on AES
*/


byte power_model(byte d_i, byte k_j) {
  return hamming_weight[ SBox[ gf28_add(d_i, k_j) ] ];
}


float correlation_coeff(int n, uint8_t xs[n], uint8_t ys[n]) {
  int x[n], y[n], xy[n], xsquare[n], ysquare[n];
  int i = 0, xsum = 0, ysum = 0, xysum = 0, xsqr_sum = 0, ysqr_sum = 0;
  float coeff, num, deno;

  // initialise
  for (i = 0; i < n; i++) {
    x[i]       = (int) xs[i];
    y[i]       = (int) ys[i];
    xy[i]      = 0;
    xsquare[i] = 0;
    ysquare[i] = 0;
  }

  // calculate pearson correlation coeff
  for (i = 0; i < n; i++) {
    xy[i] = x[i] * y[i];
    xsquare[i] = x[i] * x[i];
    ysquare[i] = y[i] * y[i];
    xsum = xsum + x[i];
    ysum = ysum + y[i];
    xysum = xysum + xy[i];
    xsqr_sum = xsqr_sum + xsquare[i];
    ysqr_sum = ysqr_sum + ysquare[i];
  }

  num   = 1.0 * ((n * xysum) - (xsum * ysum));
  deno  = 1.0 * ((n * xsqr_sum - xsum * xsum) * (n * ysqr_sum - ysum * ysum));
  coeff = num / sqrt(deno);

  return coeff;
}


void recover_key_at_pos(int key_byte_pos, byte * key, int num_i, int num_power_samples, uint8_t traces[num_power_samples][num_i], byte i[num_i][16]) {
  // alloc a table to store all hypothetical power consumptions
  uint8_t HammingWeights[256][num_i];

  // loop through all combinations of a sector address i[ix] with all possible key bytes k
  for (int k = 0; k < 256; k++) {
    for (int ix = 0; ix < num_i; ix++) {
      // get result using our power model
      HammingWeights[k][ix] = power_model(i[ix][key_byte_pos], k);
    }
  }

  // keep track of highest correlation
  float current_max = 0.0;
  byte  key_at_pos  = 0;

  // now, calculate correlations
  //  between HWs for a key hypothesis and real power at time t
  for (int k = 0; k < 256; k++) {
    for (int t = 0; t < num_power_samples; t++) {
      // we want coeff of HWs[k][:] and traces[t][:]
      float correlation = correlation_coeff(num_i, HammingWeights[k], traces[t]);
      if (correlation > current_max) {
        current_max = correlation;
        key_at_pos  = k;
      }
    }
  }

  // print max corr
  //rintf("max correlation rho_ck,ct = %f\n", current_max);

  // set k_pos in key
  key[key_byte_pos] = key_at_pos;
}


void recover_key(byte * key, int num_i, int num_power_samples, uint8_t traces[num_power_samples][num_i], byte i[num_i][16]) {
  #pragma omp parallel for schedule(dynamic)
  for (int key_byte_pos = 0; key_byte_pos < 16; key_byte_pos++) {
    recover_key_at_pos(key_byte_pos, key, num_i, num_power_samples, traces, i);
  }
}



/*
 *** Attack on XTS-AES.blockDec k1
*/

// encrypt i under recovered key k2
void enc_tweaks(byte key[16], int num_i, byte i[num_i][16], byte encrypted_tweaks[num_i][16]) {
  printf("\nEncrypting sector addresses / tweaks under recovered k2\n");
  for (int ix = 0; ix < num_i; ix++) {
    byte c[16];
    aes_enc(i[ix], key, c);
    memcpy(encrypted_tweaks[ix], c, 16 * sizeof(byte));
  }
  printf("...done\n");
}


bool test_key(byte k1[16], int num_i, byte T[num_i][16], byte P_real[num_i][16]) {
  printf("\n\nTesting key k = k1 || k2\n");

  // run XTS-AES.blockDec(k=k1||k2, C=0, i, j=0) on all i's
  // but note, we already have T's, so can skip first step

  bool all_pass = true;
  byte P_guess[num_i][16];

  for (int ix = 0; ix < num_i; ix++) {
    // decrypt under k1
    AES_KEY dec_key;
    AES_set_decrypt_key(k1, 128, &dec_key);
    AES_decrypt(T[ix], P_guess[ix], &dec_key);
    bool pass = 0 == memcmp(P_guess[ix], P_real[ix], 16 * sizeof(byte));
    all_pass = all_pass && pass;
  }
  return all_pass;
}


/*
 *** Attack Launch
*/

void launch_attack(int NUM_I, int NUM_POWER_SAMPLES) {
  // retry attack with more NUM_I, NUM_POWER_SAMPLES if it fails
  bool found_key = false; // TODO XXX this doesn't work

  while (!found_key) {
    printf("\nLaunching attack on XTS-AES key\n");
    printf("\nAttemping to recover k with (%d) number of sector addresses i and (%d) length power samples\n", NUM_I, NUM_POWER_SAMPLES);

    // generate a load of random valid i's (sector addresses)
    byte i[NUM_I][16];
    generate_is(NUM_I, i);

    // send to D and get power traces
    // use first NUM_POWER_SAMPLES for k2, and last NUM_POWER_SAMPLES for k1
    uint8_t traces_start[NUM_POWER_SAMPLES][NUM_I];
    uint8_t traces_end[NUM_POWER_SAMPLES][NUM_I];
    byte    P[NUM_I][16]; // results of XTS-AES.blockDec(k=k1|k2, c, i, j)
    get_power_traces(NUM_I, NUM_POWER_SAMPLES, P, traces_start, traces_end, i);

    // ATTACK K2

    // run attack to recover k2
    byte k2[16] = { 0 };
    recover_key(k2, NUM_I, NUM_POWER_SAMPLES, traces_start, i);

    // print result
    printf("\n\nk2 RECOVERED\n");
    printf("k2 = 0x");
    print_bytes(k2, 0, 16);

    // ATTACK K1

    // we chose j=0 so alpha^j = 1
    // we have just recovered k2
    // => we know T = alpha^j * AES-Enc(Key2, i) = AES-Enc(Key2, i)
    byte T[NUM_I][16];
    enc_tweaks(k2, NUM_I, i, T);

    // due to device behaviour (given inquestion spec), we know C = { 0 }
    //  when i or j invalid. we guarenteed i is always invalid.
    // => we know CC = C ^ T = T
    // => we can attack AES-Dec(Key1, CC)
    // we know T and P so we can get PP
    // use PP as input to AES-Dec(Key1, CC) and it will work

    // get PP (just store in P)
    for (int ix = 0; ix < NUM_I; ix++) {
      for (int j = 0; j < 16; j++) {
        P[ix][j] = P[ix][j] ^ T[ix][j];
      }
    }

    // now, recover k1
    byte k1[16] = { 0 };
    recover_key(k1, NUM_I, NUM_POWER_SAMPLES, traces_end, P);

    // print result
    printf("\n\nk1 RECOVERED\n");
    printf("k1 = 0x");
    print_bytes(k1, 0, 16);

    // TEST RESULT

    if (test_key(k1, NUM_I, T, P)) {
      // print success summary
      printf("...SUCCESS\n");
      printf("\n----SUMMARY----\n");
      printf("\nSuccessfully recovered XTS-AES key k\n");

      // print entire key
      byte k[32];
      memcpy(k   , k1, 16 * sizeof(byte));
      memcpy(k+16, k2, 16 * sizeof(byte));
      printf("k = 0x"); print_bytes(k, 0, 32);

      // print interactions
      printf("total interactions: %d\n", NUM_I);

      found_key = true;
    }
    else {
      printf("...FAIL\n");
      printf("KEY ATTACK UNSUCCESSFUL. TRYING AGIN WITH MORE I'S AND TRACES\n\n");

      // try again with more NUM_I and NUM_TRACES
      NUM_I *= 2;
      NUM_POWER_SAMPLES *= 2;
    }
  }

}




/*
 *** Main
*/

int main(int argc, char* argv[]) {
  // ensure we clean-up correctly if Control-C (or similar) is signalled.
  signal(SIGINT, &cleanup);

  // create pipes to/from attack target device
  if(pipe(target_raw) == -1) {
    abort();
  }
  if(pipe(attack_raw) == -1) {
    abort();
  }

  // fork process so we can exec the attack target
  pid = fork();

  if (pid > 0) { // parent
    // construct handles to attack target standard input and output.
    if ((target_out = fdopen( attack_raw[ 0 ], "r" )) == NULL) {
      abort();
    }
    if ((target_in  = fdopen( target_raw[ 1 ], "w" )) == NULL) {
      abort();
    }

    // execute the attack
    printf("Starting attack...\n");
    int NUM_I = 20; //20, 3000
    int NUM_POWER_SAMPLES = 3000;
    launch_attack(NUM_I, NUM_POWER_SAMPLES);
  }

  else if (pid == 0) { // child
    // (re)connect standard input and output to pipes.
    close(STDOUT_FILENO);
    if (dup2(attack_raw[ 1 ], STDOUT_FILENO) == -1) {
      abort();
    }
    close( STDIN_FILENO);
    if (dup2(target_raw[ 0 ],  STDIN_FILENO) == -1) {
      abort();
    }

    // produce a sub-process representing the attack target.
    execl(argv[ 1 ], argv[ 0 ], NULL);
  }

  else if (pid <  0) { // error
    // the fork failed; reason is stored in errno, but we'll just abort.
    abort();
  }

  // clean up any resources we've hung on to.
  printf("\nFinished. Cleaning up...\n");
  cleanup(SIGINT);

  return 0;
}
