// COMS20001 - Cellular Automaton Farm - Initial Code Skeleton
// (using the XMOS i2c accelerometer demo code)

#include <platform.h>
#include <stdio.h>
#include <string.h>
#include <xs1.h>

#include "pgmIO.h"
#include "i2c.h"

//register addresses for orientation
#define FXOS8700EQ_I2C_ADDR 0x1E
#define FXOS8700EQ_XYZ_DATA_CFG_REG 0x0E
#define FXOS8700EQ_CTRL_REG_1 0x2A
#define FXOS8700EQ_DR_STATUS 0x0
#define FXOS8700EQ_OUT_X_MSB 0x1
#define FXOS8700EQ_OUT_X_LSB 0x2
#define FXOS8700EQ_OUT_Y_MSB 0x3
#define FXOS8700EQ_OUT_Y_LSB 0x4
#define FXOS8700EQ_OUT_Z_MSB 0x5
#define FXOS8700EQ_OUT_Z_LSB 0x6

#define CYCLES_PER_MILLISECOND 100000 // 100MHz clock

#define IMHT 64  //image height
#define IMWD 64  //image width

#define INFNAME "providedtests/64x64.pgm" //in file name
#define OUTFNAME "providedtests/64x64_out.pgm" //out file name

#define MAXROUND 2

//= Adapt constants to work with small image sizes =//
#if (IMHT < 16 || IMWD < 16)
  #define LINEPARTSIZE 8           //bit width of a linePart
  typedef uint8_t li2enePart;        //a section of a line, should be consistent with LINEPARTSIZE
#elif (IMHT < 32 || IMWD < 32)
  #define LINEPARTSIZE 16           //bit width of a linePart
  typedef uint16_t linePart;        //a section of a line, should be consistent with LINEPARTSIZE
#else
  #define LINEPARTSIZE 32           //bit width of a linePart
  typedef uint32_t linePart;        //a section of a line, should be consistent with LINEPARTSIZE
#endif

#define NUMWORKERS 8             //number of worker threads

#define STRIPHEIGHT (IMHT / NUMWORKERS) // height of a strip a worker works on
#define LINEPARTSPERROW (IMWD / LINEPARTSIZE)

#define SGREEN 1
#define BLUE 2
#define GREEN 4
#define RED 8
#define CLEAR 0

typedef unsigned char uchar;      //using uchar as shorthand

on tile[0]: port p_scl = XS1_PORT_1E;       //interface ports to orientation
on tile[0]: port p_sda = XS1_PORT_1F;
on tile[0]: in port buttons = XS1_PORT_4E; //port to access xCore-200 buttons
on tile[0]: out port leds = XS1_PORT_4F;   //port to access xCore-200 LEDs


// Better modulo function, can deal with 'a' being negative
int mod(int a, int b) {
   int ret = a % b;
   if(ret < 0) {
     ret += b;
   }
   return ret;
}


//===========GoL helpers============//

// Returns the value of the nth bit in a linePart, 0th is leftmost bit
uint returnNthBitLinePart(linePart linepart, int n) {
  // Shift linePart right by the correct amount then & with 1 to return just the specified bit
  return ((linepart >> (LINEPARTSIZE - n - 1)) & 1);
}

// Returns the value of the nth bit in a row of lineParts
uint returnNthBitRow(linePart row[LINEPARTSPERROW], int n) {
  // Loop n over the length of the row
  int modN = mod(n, LINEPARTSIZE*LINEPARTSPERROW);
  // Find which linePart the required bit is in by diving the looped n by the size of each linePart (integer division in C discards remainder)
  int x = modN / LINEPARTSIZE;

  return returnNthBitLinePart(row[x], mod(n, LINEPARTSIZE));
}

// Returns the number of neighbours around a particular bit
uint countAliveNeighbours(linePart strip[STRIPHEIGHT+2][LINEPARTSPERROW], int y, int x, int i) {
  uint count = 0;
  // Find the index of a bit within the row as a whole
  int rowIndex = x * LINEPARTSIZE + i;

  // Check 2 neighbours in same row
  count += returnNthBitRow(strip[y], rowIndex - 1);
  count += returnNthBitRow(strip[y], rowIndex + 1);

  // Check 3 neighbours in row above, mod over STRIPHEIGHT+2 to include ghosts
  count += returnNthBitRow(strip[mod(y - 1, STRIPHEIGHT+2)], rowIndex);
  count += returnNthBitRow(strip[mod(y - 1, STRIPHEIGHT+2)], rowIndex + 1);
  count += returnNthBitRow(strip[mod(y - 1, STRIPHEIGHT+2)], rowIndex - 1);

  // Check 3 neighbours in row below, mod over STRIPHEIGHT+2 to include ghosts
  count += returnNthBitRow(strip[mod(y + 1, STRIPHEIGHT+2)], rowIndex);
  count += returnNthBitRow(strip[mod(y + 1, STRIPHEIGHT+2)], rowIndex + 1);
  count += returnNthBitRow(strip[mod(y + 1, STRIPHEIGHT+2)], rowIndex - 1);

  return count;
}

// Returns the updated bit for a particular bit in a linePart based on the number of its neighbours
uint applyRules(linePart linepart, int i, uint neighbours) {
  // Get the current value of the bit being looked at
  uint updated = returnNthBitLinePart(linepart, i);

  if (updated) { // If cell is living
    if (neighbours < 2) updated = 0; // Dies from underpopulation
    else if (neighbours > 3) updated = 0; // Dies from overcrowding
  }
  else { // If cell is dead
    if (neighbours == 3) updated = 1; // Birth of previously dead cell
  }

  // Return the value of the bit being looked at once the rules have been applied
  return updated;
}


//===========BIT helpers===========//

// Convert 1D char array to array of lineParts, check linePartArray is zeroed out before
void convertCharsToLineParts(linePart linePartArray[LINEPARTSPERROW], uchar charLine[IMWD]) {
  // Loop through the array of chars
  for (int i = 0; i < IMWD; i++) {
    // 0xff = 255 (if a living cell is found)
    if (charLine[i] == 0xff) {
      // make the corresponding bit in the correct linePart a 1
      linePartArray[i/LINEPARTSIZE] |= (1 << (LINEPARTSIZE - mod(i, LINEPARTSIZE) - 1));
    }
  }
}

// Convert array of lineParts to char array
void convertLinePartsToChars(linePart linePartArray[LINEPARTSPERROW], uchar charLine[IMWD]) {
  // Make a copy of the array passed in to avoid modifying it
  linePart linePartArrayCopy[LINEPARTSPERROW];

  // Loop through the passed in linePartArray and copy the values into the new array
  for (int i = 0; i < LINEPARTSPERROW; i++) {
    linePartArrayCopy[i] = linePartArray[i];
  }

  // Loop through lineParts in the array
  for (int i = 0; i < LINEPARTSPERROW; i++) {
    // Loop through the bits in the lineParts
    for (int j = LINEPARTSIZE-1; j >= 0; j--) {
      if (linePartArrayCopy[i] & 1) { // If last bit of it line is 1
        charLine[i*LINEPARTSIZE + j] = 255; // Set the corresponding character to 255
      }
      else {
        charLine[i*LINEPARTSIZE + j] = 0; // Set the corresponding character to 0
      }
      linePartArrayCopy[i] = linePartArrayCopy[i] >> 1; // Shift right to check the next bit
    }
  }
}

// Read Image from PGM file from path inFName[] to channel c_out
void dataInStream(linePart board[IMHT][LINEPARTSPERROW]) {
  char inFName[] = INFNAME;
  int res;
  uchar line[IMWD];

  //Open PGM file
  res = _openinpgm(inFName, IMWD, IMHT);
  if (res) {
    printf( "dataInStream: Error openening %s\n", inFName );
    return;
  }

  //Read image line-by-line, convert to lineParts and fill in board with lineParts
  for (int y = 0; y < IMHT; y++) {
    _readinline(line, IMWD);
    convertCharsToLineParts(board[y], line);
  }

  //Close PGM image file
  _closeinpgm();
  return;
}


//=======Concurrent Processes======//

// Passes data between dataInStream, Workers, dataOutStream, buttonListener, orientation, and showLEDs
void distributor(chanend c_out, chanend c_orientation, chanend c_workers[NUMWORKERS], chanend c_buttons, chanend c_LEDs) {

  //= Initialise variables =//
  linePart board[IMHT][LINEPARTSPERROW] = {{0}}; // Board represented as array linePart's
  int roundCount = 0, button = 0, orientation = 0, colour = CLEAR; // Varibales to help with IO
  timer t;
  uint32_t startCycles, endCycles;

  //Starting up and wait for tilting of the xCore-200 Explorer
  printf( "ProcessImage: Start, size = %dx%d\n", IMHT, IMWD );
  printf( "IMHT=%d, IMWD=%d, NUMWORKERS=%d, LINEPARTSIZE=%d\n", IMHT, IMWD, NUMWORKERS, LINEPARTSIZE );
  printf( "STRIPHEIGHT=%d, LINEPARTSPERROW=%d\n", STRIPHEIGHT, LINEPARTSPERROW );
  printf( "Waiting for SW1 button...\n" );
  int start = 0;
  while (start != 14) {
    c_buttons :> start;
  }

  // Light the green LED
  c_LEDs <: GREEN;

  //Read lineParts from dataInStream
  printf( "distributor: Reading in lineParts...\n" );
  dataInStream(board);
  printf( "distributor: Done.\n" );


  //= Add testing glider to bottom right of blank image read in
  // board[IMHT-3][LINEPARTSPERROW-1] = 2;
  // board[IMHT-2][LINEPARTSPERROW-1] = 1;
  // board[IMHT-1][LINEPARTSPERROW-1] = 7;


  // Clear the LEDs
  c_LEDs <: CLEAR;

  // Start timer
  t :> startCycles;
  //= START MAIN LOOP =//
  //while (roundCount < MAXROUND) {
  while (1) {

    if (roundCount % 2) colour = SGREEN;
    else colour = CLEAR;
    c_LEDs <: colour;

    // Send main strips to workers
    for (int i = 0; i < NUMWORKERS; i++) {
      for (int y = 0; y < STRIPHEIGHT; y++) {
        for (int x = 0; x < LINEPARTSPERROW; x++) {
          c_workers[i] <: board[i*STRIPHEIGHT + y][x];
        }
      }
    }

    // Send ghost strips to workers
    for (int i = 0; i < NUMWORKERS; i++) {
      for (int x = 0; x < LINEPARTSPERROW; x++) {
        c_workers[i] <: board[mod(i*STRIPHEIGHT - 1, IMHT)][x];
      }
      for (int x = 0; x < LINEPARTSPERROW; x++) {
        c_workers[i] <: board[mod(i*STRIPHEIGHT + STRIPHEIGHT, IMHT)][x];
      }
    }

    // Receive double lines from workers and input them into board
    for (int i = 0; i < NUMWORKERS; i++) {
      for (int y = 0; y < STRIPHEIGHT; y++){
        for (int x = 0; x < LINEPARTSPERROW; x++) {
          c_workers[i] :> board[i*STRIPHEIGHT + y][x];
        }
      }
    }

    // pause or export current game state if user says to
    select {
      case c_buttons :> button:
        if (button == 13) {
          c_LEDs <: BLUE + colour;

          c_out <: 0;

          // Send updated state of board to data out
          printf("Sending new board to data out...\n\n");
          for (int y = 0; y < IMHT; y++) {   //go through all lines
            for (int x = 0; x < LINEPARTSPERROW; x++) { //go through each pixel per line
              c_out <: board[y][x]; //send some modified pixel out
            }
          }

          c_LEDs <: colour;
        }
        break;

      case c_orientation :> orientation:
        if (orientation == 1) {
          // wait to be told that its flat again
          printf("PAUSED\n");
          c_LEDs <: RED + colour;
          c_orientation :> orientation;
          c_LEDs <: colour;
          printf("RESUMED\n");
        }
        break;

      default:
        break;
    }

    button = 0;
    roundCount++;
  }
  //=== following will not run unless MAXROUND was set in while condition

  t :> endCycles;

  printf("Elapsed time for %u processing rounds was %ums\n", MAXROUND, (endCycles - startCycles) / CYCLES_PER_MILLISECOND);

  c_LEDs <: BLUE + colour;

  c_out <: 0;

  // Send updated state of board to data out
  printf("Sending new board to data out...\n\n");
  for (int y = 0; y < IMHT; y++) {   //go through all lines
    for (int x = 0; x < LINEPARTSPERROW; x++) { //go through each pixel per line
      c_out <: board[y][x]; //send some modified pixel out
    }
  }

  c_LEDs <: colour;
}

// One instance of a worker to work on a particular strip of the image
void worker(int id, chanend c_distributor) {
  while (1) {
    //= Initialise variables =//
    linePart strip[STRIPHEIGHT + 2][LINEPARTSPERROW] = {{0}}; // STRIPHEIGHT+2 to include ghost lines
    linePart updatedStrip[STRIPHEIGHT + 2][LINEPARTSPERROW] = {{0}}; // STRIPHEIGHT+2 to include ghost lines
    uint neighbours = 0, updatedBit = 0; // Variables to use to keep track later in the function

    //= Read in data from distributor =//
    // Read in main strip
    // Loop through each row being worked on
    for (int y = 1; y <= STRIPHEIGHT; y++) { // Reserve 0th and last line for ghost
      // Loop through each linePart in the row
      for (int x = 0; x < LINEPARTSPERROW; x++) {
        // Read that linePart into the correct place in the strip array
        c_distributor :> strip[y][x];
      }
    }
    // Read in ghost above main strip
    for (int x = 0; x < LINEPARTSPERROW; x++) {
      c_distributor :> strip[0][x]; // Place the top ghost strip at y = 0
    }
    // Read in ghost below main strip
    for (int x = 0; x < LINEPARTSPERROW; x++) {
      c_distributor :> strip[STRIPHEIGHT+1][x]; // Place the bottom ghost strip at the bottom
    }

    //= Apply GoL rules functions =//
    // Loop through each row in the strip (but not the ghost rows)
    for (int y = 1; y <= STRIPHEIGHT; y++) {
      // Loop through each linePart in the row
      for (int x = 0; x < LINEPARTSPERROW; x++) {
        // Loop through each bit in the linePart
        for (int i = 0; i < LINEPARTSIZE; i++) {
          // Count the neighbours for each bit
          neighbours = countAliveNeighbours(strip, y, x, i);

          // Get the value of the updated bit based on the rules applied with the number of neighbours
          updatedBit = applyRules(strip[y][x], i, neighbours);

          // Update the bit
          updatedStrip[y][x] |= (updatedBit << (LINEPARTSIZE - i - 1));
        }
      }
    }


    //= Send data back to distributor =//
    // Loop through each row in the strip being worked on
    for (int y = 1; y <= STRIPHEIGHT; y++) { // Reserve 0th and last line for ghost
      // Loop through each linePart in the row
      for (int x = 0; x < LINEPARTSPERROW; x++) {
        // Send the linePart over the channel
        c_distributor <: updatedStrip[y][x];
      }
    }
  }
}

// Write pixel stream from channel c_in to PGM image file
void dataOutStream(chanend c_in) {
  char outFName[] = OUTFNAME;
  int res;
  uchar line[IMWD] = {0};
  linePart linePartArray[LINEPARTSPERROW] = {0};

  while (1) {
    c_in :> int start;
    //Open PGM file
    printf ("dataOutStream: Start...\n");
    res = _openoutpgm(outFName, IMWD, IMHT);
    if (res) {
      printf ("dataOutStream: Error opening %s\n.", outFName);
      return;
    }

    //Compile each line of the image and write the image line-by-line
    for (int y = 0; y < IMHT; y++) {
      for (int x = 0; x < LINEPARTSPERROW; x++) {
        c_in :> linePartArray[x];
      }
      convertLinePartsToChars(linePartArray, line);
      _writeoutline(line, IMWD);
    }

    //Close the PGM image
    _closeoutpgm();
    printf("dataOutStream: Done...\n");
  }

}

// Initialise and read orientation, send tilted boolean to distributor
void orientation(client interface i2c_master_if i2c, chanend c_distributor) {
  i2c_regop_res_t result;
  char status_data = 0;
  int tilted = 0;

  // Configure FXOS8700EQ
  result = i2c.write_reg(FXOS8700EQ_I2C_ADDR, FXOS8700EQ_XYZ_DATA_CFG_REG, 0x01);
  if (result != I2C_REGOP_SUCCESS) {
    printf("I2C write reg failed\n");
  }

  // Enable FXOS8700EQ
  result = i2c.write_reg(FXOS8700EQ_I2C_ADDR, FXOS8700EQ_CTRL_REG_1, 0x01);
  if (result != I2C_REGOP_SUCCESS) {
    printf("I2C write reg failed\n");
  }

  // wait until distributor says x

  //Probe the orientation x-axis forever
  while (1) {

    //check until new orientation data is available
    do {
      status_data = i2c.read_reg(FXOS8700EQ_I2C_ADDR, FXOS8700EQ_DR_STATUS, result);
    } while (!status_data & 0x08);

    //get new x-axis tilt value
    int x = read_acceleration(i2c, FXOS8700EQ_OUT_X_MSB);

    //printf("orientation: x = %d\n", x);
    //printf("orietnation: tilted = %d\n", tilted);

    //send signal to distributor after first tilt
    if (!tilted) {
      if (x > 30) {
        tilted = 1;
        //printf("orientation: sending a 1 c_distributor\n");
        c_distributor <: 1;
        //printf("orietnation: sent a 1\n");
      }
    }
    else {
      if (x <= 30) {
        tilted = 0;
        //printf("orietnation: sending a 0 to c_distributor\n");
        c_distributor <: 0;
        //printf("orientation: sent a 0\n");
      }
    }
  }
}

// Displays an LED pattern
int showLEDs(out port p, chanend c_distributor) {
  int pattern; //1st bit...separate green LED
               //2nd bit...blue LED
               //3rd bit...green LED
               //4th bit...red LED
  while (1) {
    c_distributor :> pattern;   //receive new pattern from visualiser
    p <: pattern;                //send pattern to LED port
  }
  return 0;
}

// Reads buttons and sends button pattern to distributor
void buttonListener(in port b, chanend c_distributor) {
  int r;
  while (1) {
    b when pinseq(15)  :> r;    // check that no button is pressed
    b when pinsneq(15) :> r;    // check if some buttons are pressed
    if ((r==13) || (r==14))     // if either button is pressed
    c_distributor <: r;             // send button pattern to userAnt
  }
}


//=============MAIN===============//

// Orchestrates concurrent system and starts up all threads
int main(void) {

  // interface to orientation
  i2c_master_if i2c[1];

  // channel defs
  chan c_outIO, c_control, c_buttons, c_LEDs;
  chan cDistributorWorkers[NUMWORKERS];

  par {
    on tile[0]: i2c_master(i2c, 1, p_scl, p_sda, 10);   //server thread providing orientation data
    on tile[0]: orientation(i2c[0], c_control);          //client thread reading orientation data
    on tile[0]: dataOutStream(c_outIO);        //thread to write out a PGM image
    on tile[0]: distributor(c_outIO, c_control, cDistributorWorkers, c_buttons, c_LEDs); //thread to coordinate work
    on tile[0]: buttonListener(buttons, c_buttons); // thread to deal with buttons
    on tile[0]: showLEDs(leds, c_LEDs); // thread to deal with leds

    //worker threads to apply GoL rules to given strip(s) of board
    par (int i = 0; i < 2; i++) {
      on tile[0]: worker(i, cDistributorWorkers[i]);
    }

    par (int i = 2; i < NUMWORKERS; i++) {
      on tile[1]: worker(i, cDistributorWorkers[i]);
    }
  }

  return 0;
}
