/*
 ///////////////////////////////////////////////////////////////////////////////
 This example demonstrates the use of the CCDebugger class from CCLib.

 This is the firmware you must flash in your Wemos D1 Mini if you want to use
 the python library that comes with this project.

 ///////////////////////////////////////////////////////////////////////////////
 (C) Copyright 2014, Ioannis Charalampidis - Licensed under GNU/GPLv3 License.
 (C) Copyright 2015, Simon Schulz - github.com/fishpepper
 ///////////////////////////////////////////////////////////////////////////////
 */

////////////////////////////////////////
////////////////////////////////////////

// Pinout configuration (Configured for Tasmota Zigbee)
int CC_RST   = 12;
int CC_DC    = 13;
int CC_DD    = 14;
int SEL_0    = 16;
int SEL_1    = 5;

////////////////////////////////////////
////////////////////////////////////////

// Include the CCDebugger
#include "CCDebugger.h"

// Command constants
#define   CMD_ENTER     byte(0x01)
#define   CMD_EXIT      byte(0x02)
#define   CMD_CHIP_ID   byte(0x03)
#define   CMD_STATUS    byte(0x04)
#define   CMD_PC        byte(0x05)
#define   CMD_STEP      byte(0x06)
#define   CMD_EXEC_1    byte(0x07)
#define   CMD_EXEC_2    byte(0x08)
#define   CMD_EXEC_3    byte(0x09)
#define   CMD_BRUSTWR   byte(0x0A)
#define   CMD_RD_CFG    byte(0x0B)
#define   CMD_WR_CFG    byte(0x0C)
#define   CMD_CHPERASE  byte(0x0D)
#define   CMD_RESUME    byte(0x0E)
#define   CMD_HALT      byte(0x0F)
#define   CMD_PING      byte(0xF0)
#define   CMD_INSTR_VER byte(0xF1)
#define   CMD_INSTR_UPD byte(0xF2)

// Response constants
#define   ANS_OK       byte(0x01)
#define   ANS_ERROR    byte(0x02)
#define   ANS_READY    byte(0x03)

// Initialize properties
CCDebugger * dbg;
byte inByte, bAns, bIdle;
byte c1, c2, c3;
unsigned short s1;
int iLen, iRead;

/**
 * Initialize debugger
 */
void setup() {

  // Set USB tty to ESP
  pinMode(SEL_0, OUTPUT);
  pinMode(SEL_0, OUTPUT);
  digitalWrite(SEL_0, HIGH);
  digitalWrite(SEL_1, LOW);

  // Create debugger
  dbg = new CCDebugger( CC_RST, CC_DC, CC_DD );

  // Initialize serial port
  Serial.begin(115200);

  // Wait for chip to initialize
  delay(100);

  // Enter debug mode
  dbg->enter();

}

/**
 * Send a response frame
 */
void sendFrame( const byte ans, const byte b0=0, const byte b1=0 ) {
    Serial.write(ans);
    Serial.write(b1); // Send High-order first
    Serial.write(b0); // Send Low-order second
    Serial.flush();
}

/**
 * Check if debugger is in error state and if yes,
 * reply with an error code.
 */
boolean handleError( ) {
  if (dbg->error()) {
    sendFrame(ANS_ERROR, dbg->error());
    return true;
  }
  return false;
}

/**
 * Main program loop
 */
void loop() {

  // Wait for incoming data frame
  if (Serial.available() < 4)
    return;

  // Read input frame
  inByte = Serial.read();
      c1 = Serial.read();
      c2 = Serial.read();
      c3 = Serial.read();

  // Handle commands
  if (inByte == CMD_PING) {
    sendFrame( ANS_OK );

  } else if (inByte == CMD_ENTER) {
    bAns = dbg->enter();
    if (handleError()) return;
    sendFrame( ANS_OK );

  } else if (inByte == CMD_EXIT) {
    bAns = dbg->exit();
    if (handleError()) return;
    sendFrame( ANS_OK );

  } else if (inByte == CMD_CHIP_ID) {
    s1 = dbg->getChipID();
    if (handleError()) return;
    sendFrame( ANS_OK,
               s1 & 0xFF,       // LOW first
               (s1 >> 8) & 0xFF // HIGH second
              );

  } else if (inByte == CMD_PC) {
    s1 = dbg->getPC();
    if (handleError()) return;
    sendFrame( ANS_OK,
               s1 & 0xFF,       // LOW first
               (s1 >> 8) & 0xFF // HIGH second
              );

  } else if (inByte == CMD_STATUS) {
    bAns = dbg->getStatus();
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else if (inByte == CMD_HALT) {
    bAns = dbg->halt();
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else if (inByte == CMD_EXEC_1) {

    bAns = dbg->exec( c1 );
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else if (inByte == CMD_EXEC_2) {

    bAns = dbg->exec( c1, c2 );
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else if (inByte == CMD_EXEC_3) {
    bAns = dbg->exec( c1, c2, c3 );
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else if (inByte == CMD_BRUSTWR) {

    // Calculate the size of the incoming brust
    iLen = (c1 << 8) | c2;

    // Validate length
    if (iLen > 2048) {
      sendFrame( ANS_ERROR, 3 );
      return;
    }

    // Confirm transfer
    sendFrame( ANS_READY );

    // Prepare for brust-write
    dbg->write( 0x80 | (c1 & 0x07) ); // High-order bits
    dbg->write( c2 ); // Low-order bits

    // Start serial loop
    iRead = iLen;
    bIdle = 0;
    while (iRead > 0) {

      // When we have data, forward them to the debugger
      if (Serial.available() >= 1) {
        inByte = Serial.read();
        dbg->write(inByte);
        bIdle = 0;
        iRead--;
      }

      // If we don't have any data, check for idle timeout
      else {

        // If we are idle for more than 3s, drop command
        if (++bIdle > 3000) {

          // The PC was disconnected/stale for too long
          // complete the command by sending 0's
          while (iRead > 0) {
            dbg->write(0);
            iRead--;
          }

          // Read debug status to complete the command sequence
          dbg->switchRead();
          bAns = dbg->read();
          dbg->switchWrite();

          // Send error
          sendFrame( ANS_ERROR, 4 );
          return;

        }

        // Wait for some time
        delay(1);

      }
    }

    // Read debug status
    dbg->switchRead();
    bAns = dbg->read();
    dbg->switchWrite();

    // Handle response
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else if (inByte == CMD_RD_CFG) {
    bAns = dbg->getConfig();
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else if (inByte == CMD_WR_CFG) {
    bAns = dbg->setConfig(c1);
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else if (inByte == CMD_CHPERASE) {
    bAns = dbg->chipErase();
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else if (inByte == CMD_STEP) {
    bAns = dbg->step();
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else if (inByte == CMD_RESUME) {
    bAns = dbg->resume();
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else if (inByte == CMD_INSTR_VER) {
    bAns = dbg->getInstructionTableVersion();
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else if (inByte == CMD_INSTR_UPD) {

    // Acknowledge transfer
    sendFrame( ANS_READY );

    // Read 16 bytes from the input
    byte instrBuffer[16];
    iRead = 0;
    while (iRead < 16) {
      if (Serial.available() >= 1) {
        instrBuffer[iRead++] = Serial.read();
      }
    }

    // Update instruction buffer
    bAns = dbg->updateInstructionTable( instrBuffer );
    if (handleError()) return;
    sendFrame( ANS_OK, bAns );

  } else {
    sendFrame( ANS_ERROR, 0xFF );

  }

}
