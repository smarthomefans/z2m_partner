/**
 *
 * CC-Debugger Protocol Library for Arduino
 * Copyright (c) 2014-2016 Ioannis Charalampidis
 * Copyright (c) 2015 Simon Schulz - github.com/fishpepper
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CCDebugger.h"

/**
 * Instruction table indices
 */
#define INSTR_VERSION   0
#define I_HALT          1
#define I_RESUME        2
#define I_RD_CONFIG     3
#define I_WR_CONFIG     4
#define I_DEBUG_INSTR_1 5
#define I_DEBUG_INSTR_2 6
#define I_DEBUG_INSTR_3 7
#define I_GET_CHIP_ID   8
#define I_GET_PC        9
#define I_READ_STATUS   10
#define I_STEP_INSTR    11
#define I_CHIP_ERASE    12

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
////                CONSTRUCTOR & CONFIGURATORS                  ////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

/**
 * Initialize CC Debugger class
 */
CCDebugger::CCDebugger( int pinRST, int pinDC, int pinDD)
{

  // Keep references
  this->pinRST  = pinRST;
  this->pinDC   = pinDC;
  this->pinDD   = pinDD;

  // Prepare CC Pins
  pinMode(pinDC,        OUTPUT);
  pinMode(pinDD,        INPUT);
  pinMode(pinRST,       OUTPUT);
  digitalWrite(pinDC,   LOW);
  digitalWrite(pinDD,   LOW); // No pull-up
  digitalWrite(pinRST,  LOW);

  // Prepare default direction
  setDDDirection(INPUT);

  // Default CCDebug instruction set for CC254x
  instr[INSTR_VERSION]    = 1;
  instr[I_HALT]           = 0x40;
  instr[I_RESUME]         = 0x48;
  instr[I_RD_CONFIG]      = 0x20;
  instr[I_WR_CONFIG]      = 0x18;
  instr[I_DEBUG_INSTR_1]  = 0x51;
  instr[I_DEBUG_INSTR_2]  = 0x52;
  instr[I_DEBUG_INSTR_3]  = 0x53;
  instr[I_GET_CHIP_ID]    = 0x68;
  instr[I_GET_PC]         = 0x28;
  instr[I_READ_STATUS]    = 0x30;
  instr[I_STEP_INSTR]     = 0x58;
  instr[I_CHIP_ERASE]     = 0x10;

  // We are active by default
  active = true;

};


/**
 * Activate/Deactivate debugger
 */
void CCDebugger::setActive( boolean on )
{

  // Reset error flag
  errorFlag = CC_ERROR_NONE;

  // Continue only if active
  if (on == this->active) return;
  this->active = on;

  if (on) {

    // Prepare CC Pins
    pinMode(pinDC,        OUTPUT);
    pinMode(pinDD,        INPUT);
    pinMode(pinRST,       OUTPUT);
    digitalWrite(pinDC,   LOW);
    digitalWrite(pinDD,   LOW); // No pull-up
    digitalWrite(pinRST,  LOW);

    // Default direction is INPUT
    setDDDirection(INPUT);

  } else {

    // Before deactivating, exit debug mode
    if (inDebugMode)
      this->exit();

    // Put everything in inactive mode
    pinMode(pinDC,        INPUT);
    pinMode(pinDD,        INPUT);
    pinMode(pinRST,       INPUT);
    digitalWrite(pinDC,   LOW);
    digitalWrite(pinDD,   LOW);
    digitalWrite(pinRST,  LOW);

  }
}

/**
 * Return the error flag
 */
byte CCDebugger::error()
{
  return errorFlag;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
////                     LOW LEVEL FUNCTIONS                     ////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

/**
 * Delay a particular number of cycles
 */
void cc_delay( unsigned char d )
{
  volatile unsigned char i = d;
  while( i-- );
}

/**
 * Enter debug mode
 */
byte CCDebugger::enter()
{
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  // =============

  // Reset error flag
  errorFlag = CC_ERROR_NONE;

  // Enter debug mode
  digitalWrite(pinRST, LOW);
  cc_delay(200);
  digitalWrite(pinDC, HIGH);
  cc_delay(3);
  digitalWrite(pinDC, LOW);
  cc_delay(3);
  digitalWrite(pinDC, HIGH);
  cc_delay(3);
  digitalWrite(pinDC, LOW);
  cc_delay(200);
  digitalWrite(pinRST, HIGH);
  cc_delay(200);

  // We are now in debug mode
  inDebugMode = 1;

  // =============

  // Success
  return 0;

};

/**
 * Write a byte to the debugger
 */
byte CCDebugger::write( byte data )
{
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  };
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }
  // =============

  byte cnt;

  // Make sure DD is on output
  setDDDirection(OUTPUT);

  // Sent bytes
  for (cnt = 8; cnt; cnt--) {

    // First put data bit on bus
    if (data & 0x80)
      digitalWrite(pinDD, HIGH);
    else
      digitalWrite(pinDD, LOW);

    // Place clock on high (other end reads data)
    digitalWrite(pinDC, HIGH);

    // Shift & Delay
    data <<= 1;
    cc_delay(2);

    // Place clock down
    digitalWrite(pinDC, LOW);
    cc_delay(2);

  }

  // =============
  return 0;
}

/**
 * Wait until input is ready for reading
 */
byte CCDebugger::switchRead(byte maxWaitCycles)
{
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }
  // =============

  byte cnt;
  byte didWait = 0;

  // Switch to input
  setDDDirection(INPUT);

  // Wait at least 83 ns before checking state t(dir_change)
  cc_delay(2);

  // Wait for DD to go LOW (Chip is READY)
  while (digitalRead(pinDD) == HIGH) {

    // Do 8 clock cycles
    for (cnt = 8; cnt; cnt--) {
      digitalWrite(pinDC, HIGH);
      cc_delay(2);
      digitalWrite(pinDC, LOW);
      cc_delay(2);
    }

    // Let next function know that we did wait
    didWait = 1;

    // Check if we ran out if wait cycles
    if (!--maxWaitCycles) {

      // If we are waiting for too long, we have lost the chip,
      // so also assume we are out of debugging mode
      errorFlag = CC_ERROR_NOT_WIRED;
      inDebugMode = 0;

      return 0;
    }
  }

  // Wait t(sample_wait)
  if (didWait) cc_delay(2);

  // =============
  return 0;
}

/**
 * Switch to output
 */
byte CCDebugger::switchWrite()
{
  setDDDirection(OUTPUT);
  return 0;
}

/**
 * Read an input byte
 */
byte CCDebugger::read()
{
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  // =============

  byte cnt;
  byte data = 0;

  // Switch to input
  setDDDirection(INPUT);

  // Send 8 clock pulses if we are HIGH
  for (cnt = 8; cnt; cnt--) {
    digitalWrite(pinDC, HIGH);
    cc_delay(2);

    // Shift and read
    data <<= 1;
    if (digitalRead(pinDD) == HIGH)
      data |= 0x01;

    digitalWrite(pinDC, LOW);
    cc_delay(2);
  }

  // =============

  return data;
}

/**
 * Switch reset pin
 */
void CCDebugger::setDDDirection( byte direction )
{

  // Switch direction if changed
  if (direction == ddIsOutput) return;
  ddIsOutput = direction;

  // Handle new direction
  if (ddIsOutput) {
    pinMode(pinDD, OUTPUT);   // Enable output
    digitalWrite(pinDD, LOW); // Switch to low
  } else {
    pinMode(pinDD, INPUT);    // Disable output
    digitalWrite(pinDD, LOW); // Don't use output pull-up
  }

}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
////                    HIGH LEVEL FUNCTIONS                     ////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////


/**
 * Exit from debug mode
 */
byte CCDebugger::exit()
{
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  byte bAns;
  (void)bAns;   // avoid warning about variable not being used

  write( instr[I_RESUME] ); // RESUME
  switchRead();
  bAns = read(); // debug status
  switchWrite();

  inDebugMode = 0;

  return 0;
}
/**
 * Get debug configuration
 */
byte CCDebugger::getConfig() {
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  byte bAns;

  write( instr[I_RD_CONFIG] ); // RD_CONFIG
  switchRead();
  bAns = read(); // Config
  switchWrite();

  return bAns;
}

/**
 * Set debug configuration
 */
byte CCDebugger::setConfig( byte config ) {
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  byte bAns;

  write( instr[I_WR_CONFIG] ); // WR_CONFIG
  write( config );
  switchRead();
  bAns = read(); // Config
  switchWrite();

  return bAns;
}

/**
 * Invoke a debug instruction with 1 opcode
 */
byte CCDebugger::exec( byte oc0 )
{
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  byte bAns;

  write( instr[I_DEBUG_INSTR_1] ); // DEBUG_INSTR + 1b
  write( oc0 );
  switchRead();
  bAns = read(); // Accumulator
  switchWrite();

  return bAns;
}

/**
 * Invoke a debug instruction with 2 opcodes
 */
byte CCDebugger::exec( byte oc0, byte oc1 )
{
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  byte bAns;

  write( instr[I_DEBUG_INSTR_2] ); // DEBUG_INSTR + 2b
  write( oc0 );
  write( oc1 );
  switchRead();
  bAns = read(); // Accumulator
  switchWrite();

  return bAns;
}

/**
 * Invoke a debug instruction with 3 opcodes
 */
byte CCDebugger::exec( byte oc0, byte oc1, byte oc2 )
{
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  byte bAns;

  write( instr[I_DEBUG_INSTR_3] ); // DEBUG_INSTR + 3b
  write( oc0 );
  write( oc1 );
  write( oc2 );
  switchRead();
  bAns = read(); // Accumulator
  switchWrite();

  return bAns;
}

/**
 * Invoke a debug instruction with 1 opcode + 16-bit immediate
 */
byte CCDebugger::execi( byte oc0, unsigned short c0 )
{
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  byte bAns;

  write( instr[I_DEBUG_INSTR_3] ); // DEBUG_INSTR + 3b
  write( oc0 );
  write( (c0 >> 8) & 0xFF );
  write(  c0 & 0xFF );
  switchRead();
  bAns = read(); // Accumulator
  switchWrite();

  return bAns;
}

/**
 * Return chip ID
 */
unsigned short CCDebugger::getChipID() {
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  unsigned short bAns;
  byte bRes;

  write( instr[I_GET_CHIP_ID] ); // GET_CHIP_ID
  switchRead();
  bRes = read(); // High order
  bAns = bRes << 8;
  bRes = read(); // Low order
  bAns |= bRes;
  switchWrite();

  return bAns;
}

/**
 * Return PC
 */
unsigned short CCDebugger::getPC() {
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  unsigned short bAns;
  byte bRes;

  write( instr[I_GET_PC] ); // GET_PC
  switchRead();
  bRes = read(); // High order
  bAns = bRes << 8;
  bRes = read(); // Low order
  bAns |= bRes;
  switchWrite();

  return bAns;
}

/**
 * Return debug status
 */
byte CCDebugger::getStatus() {
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  byte bAns;

  write( instr[I_READ_STATUS] ); // READ_STATUS
  switchRead();
  bAns = read(); // debug status
  switchWrite();

  return bAns;
}

/**
 * Step instruction
 */
byte CCDebugger::step() {
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  byte bAns;

  write( instr[I_STEP_INSTR] ); // STEP_INSTR
  switchRead();
  bAns = read(); // Accumulator
  switchWrite();

  return bAns;
}

/**
 * resume instruction
 */
byte CCDebugger::resume() {
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  byte bAns;

  write( instr[I_RESUME] ); //RESUME
  switchRead();
  bAns = read(); // Accumulator
  switchWrite();

  return bAns;
}

/**
 * halt instruction
 */
byte CCDebugger::halt() {
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  }
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  byte bAns;

  write( instr[I_HALT] ); //HALT
  switchRead();
  bAns = read(); // Accumulator
  switchWrite();

  return bAns;
}

/**
 * Mass-erase all chip configuration & Lock Bits
 */
byte CCDebugger::chipErase()
{
  if (!active) {
    errorFlag = CC_ERROR_NOT_ACTIVE;
    return 0;
  };
  if (!inDebugMode) {
    errorFlag = CC_ERROR_NOT_DEBUGGING;
    return 0;
  }

  byte bAns;

  write( instr[I_CHIP_ERASE] ); // CHIP_ERASE
  switchRead();
  bAns = read(); // Debug status
  switchWrite();

  return bAns;
}

/**
 * Update the debug instruction table
 */
byte CCDebugger::updateInstructionTable( byte newTable[16] )
{
  // Copy table entries
  for (byte i=0; i<16; i++)
    instr[i] = newTable[i];
  // Return the new version
  return instr[INSTR_VERSION];
}

/**
 * Get the instruction table version
 */
byte CCDebugger::getInstructionTableVersion()
{
  // Return version of instruction table
  return instr[INSTR_VERSION];
}
