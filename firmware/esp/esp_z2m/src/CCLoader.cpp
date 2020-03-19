#include "CCLoader.h"

void CCLoader::write_debug_byte(unsigned char data)
{
    unsigned char i;
    for (i = 0; i < 8; i++)
    {
        // Set clock high and put data on DD line
        digitalWrite(DC, HIGH);
        if (data & 0x80)
        {
            digitalWrite(DD, HIGH);
        }
        else
        {
            digitalWrite(DD, LOW);
        }
        data <<= 1;
        digitalWrite(DC, LOW); // set clock low (DUP capture flank)
    }
}

unsigned char CCLoader::read_debug_byte(void)
{
    unsigned char i;
    unsigned char data = 0x00;
    for (i = 0; i < 8; i++)
    {
        digitalWrite(DC, HIGH); // DC high
        data <<= 1;
        if (HIGH == digitalRead(DD))
        {
            data |= 0x01;
        }
        digitalWrite(DC, LOW); // DC low
    }
    return data;
}

unsigned char CCLoader::wait_dup_ready(void)
{
    // DUP pulls DD low when ready
    unsigned int count = 0;
    while ((HIGH == digitalRead(DD)) && count < 16)
    {
        // Clock out 8 bits before checking if DD is low again
        read_debug_byte();
        count++;
    }
    return (count == 16) ? 0 : 1;
}

unsigned char CCLoader::debug_command(unsigned char cmd, unsigned char *cmd_bytes, unsigned short num_cmd_bytes)
{
    unsigned short i;
    unsigned char output = 0;
    // Make sure DD is output
    pinMode(DD, OUTPUT);
    // Send command
    write_debug_byte(cmd);
    // Send bytes
    for (i = 0; i < num_cmd_bytes; i++)
    {
        write_debug_byte(cmd_bytes[i]);
    }
    // Set DD as input
    pinMode(DD, INPUT);
    digitalWrite(DD, HIGH);
    // Wait for data to be ready
    wait_dup_ready();
    // Read returned byte
    output = read_debug_byte();
    // Set DD as output
    pinMode(DD, OUTPUT);

    return output;
}

void CCLoader::debug_init(void)
{
    volatile unsigned char i;

    // Send two flanks on DC while keeping RESET_N low
    // All low (incl. RESET_N)
    digitalWrite(DD, LOW);
    digitalWrite(DC, LOW);
    digitalWrite(RESET, LOW);
    delay(10);                 // Wait
    digitalWrite(DC, HIGH);    // DC high
    delay(10);                 // Wait
    digitalWrite(DC, LOW);     // DC low
    delay(10);                 // Wait
    digitalWrite(DC, HIGH);    // DC high
    delay(10);                 // Wait
    digitalWrite(DC, LOW);     // DC low
    delay(10);                 // Wait
    digitalWrite(RESET, HIGH); // Release RESET_N
    delay(10);                 // Wait
}

unsigned char CCLoader::read_chip_id(void)
{
    unsigned char id = 0;

    // Make sure DD is output
    pinMode(DD, OUTPUT);
    delay(1);
    // Send command
    write_debug_byte(CMD_GET_CHIP_ID);
    // Set DD as input
    pinMode(DD, INPUT);
    digitalWrite(DD, HIGH);
    delay(1);
    // Wait for data to be ready
    if (wait_dup_ready() == 1)
    {
        // Read ID and revision
        id = read_debug_byte(); // ID
        read_debug_byte();      // Revision (discard)
    }
    // Set DD as output
    pinMode(DD, OUTPUT);

    return id;
}

void CCLoader::burst_write_block(unsigned char *src, unsigned short num_bytes)
{
    unsigned short i;

    // Make sure DD is output
    pinMode(DD, OUTPUT);

    write_debug_byte(CMD_BURST_WRITE | HIBYTE(num_bytes));
    write_debug_byte(LOBYTE(num_bytes));
    for (i = 0; i < num_bytes; i++)
    {
        write_debug_byte(src[i]);
    }

    // Set DD as input
    pinMode(DD, INPUT);
    digitalWrite(DD, HIGH);
    // Wait for DUP to be ready
    wait_dup_ready();
    read_debug_byte(); // ignore output
    // Set DD as output
    pinMode(DD, OUTPUT);
}

void CCLoader::chip_erase(void)
{
    volatile unsigned char status;
    // Send command
    debug_command(CMD_CHIP_ERASE, 0, 0);

    // Wait for status bit 7 to go low
    do
    {
        status = debug_command(CMD_READ_STATUS, 0, 0);
    } while ((status & STATUS_CHIP_ERASE_BUSY_BM));
}

void CCLoader::write_xdata_memory_block(unsigned short address, const unsigned char *values, unsigned short num_bytes)
{
    unsigned char instr[3];
    unsigned short i;

    // MOV DPTR, address
    instr[0] = 0x90;
    instr[1] = HIBYTE(address);
    instr[2] = LOBYTE(address);
    debug_command(CMD_DEBUG_INSTR_3B, instr, 3);

    for (i = 0; i < num_bytes; i++)
    {
        // MOV A, values[i]
        instr[0] = 0x74;
        instr[1] = values[i];
        debug_command(CMD_DEBUG_INSTR_2B, instr, 2);

        // MOV @DPTR, A
        instr[0] = 0xF0;
        debug_command(CMD_DEBUG_INSTR_1B, instr, 1);

        // INC DPTR
        instr[0] = 0xA3;
        debug_command(CMD_DEBUG_INSTR_1B, instr, 1);
    }
}

void CCLoader::write_xdata_memory(unsigned short address, unsigned char value)
{
    unsigned char instr[3];

    // MOV DPTR, address
    instr[0] = 0x90;
    instr[1] = HIBYTE(address);
    instr[2] = LOBYTE(address);
    debug_command(CMD_DEBUG_INSTR_3B, instr, 3);

    // MOV A, values[i]
    instr[0] = 0x74;
    instr[1] = value;
    debug_command(CMD_DEBUG_INSTR_2B, instr, 2);

    // MOV @DPTR, A
    instr[0] = 0xF0;
    debug_command(CMD_DEBUG_INSTR_1B, instr, 1);
}

unsigned char CCLoader::read_xdata_memory(unsigned short address)
{
    unsigned char instr[3];

    // MOV DPTR, address
    instr[0] = 0x90;
    instr[1] = HIBYTE(address);
    instr[2] = LOBYTE(address);
    debug_command(CMD_DEBUG_INSTR_3B, instr, 3);

    // MOVX A, @DPTR
    instr[0] = 0xE0;
    return debug_command(CMD_DEBUG_INSTR_1B, instr, 1);
}

void CCLoader::read_flash_memory_block(unsigned char bank, unsigned short flash_addr, unsigned short num_bytes, unsigned char *values)
{
    unsigned char instr[3];
    unsigned short i;
    unsigned short xdata_addr = (0x8000 + flash_addr);

    // 1. Map flash memory bank to XDATA address 0x8000-0xFFFF
    write_xdata_memory(DUP_MEMCTR, bank);

    // 2. Move data pointer to XDATA address (MOV DPTR, xdata_addr)
    instr[0] = 0x90;
    instr[1] = HIBYTE(xdata_addr);
    instr[2] = LOBYTE(xdata_addr);
    debug_command(CMD_DEBUG_INSTR_3B, instr, 3);

    for (i = 0; i < num_bytes; i++)
    {
        // 3. Move value pointed to by DPTR to accumulator (MOVX A, @DPTR)
        instr[0] = 0xE0;
        values[i] = debug_command(CMD_DEBUG_INSTR_1B, instr, 1);

        // 4. Increment data pointer (INC DPTR)
        instr[0] = 0xA3;
        debug_command(CMD_DEBUG_INSTR_1B, instr, 1);
    }
}

void CCLoader::write_flash_memory_block(unsigned char *src, unsigned long start_addr, unsigned short num_bytes)
{
    // 1. Write the 2 DMA descriptors to RAM
    write_xdata_memory_block(ADDR_DMA_DESC_0, dma_desc_0, 8);
    write_xdata_memory_block(ADDR_DMA_DESC_1, dma_desc_1, 8);

    // 2. Update LEN value in DUP's DMA descriptors
    unsigned char len[2] = {HIBYTE(num_bytes), LOBYTE(num_bytes)};
    write_xdata_memory_block((ADDR_DMA_DESC_0 + 4), len, 2); // LEN, DBG => ram
    write_xdata_memory_block((ADDR_DMA_DESC_1 + 4), len, 2); // LEN, ram => flash

    // 3. Set DMA controller pointer to the DMA descriptors
    write_xdata_memory(DUP_DMA0CFGH, HIBYTE(ADDR_DMA_DESC_0));
    write_xdata_memory(DUP_DMA0CFGL, LOBYTE(ADDR_DMA_DESC_0));
    write_xdata_memory(DUP_DMA1CFGH, HIBYTE(ADDR_DMA_DESC_1));
    write_xdata_memory(DUP_DMA1CFGL, LOBYTE(ADDR_DMA_DESC_1));

    // 4. Set Flash controller start address (wants 16MSb of 18 bit address)
    write_xdata_memory(DUP_FADDRH, HIBYTE((start_addr))); //>>2) ));
    write_xdata_memory(DUP_FADDRL, LOBYTE((start_addr))); //>>2) ));

    // 5. Arm DBG=>buffer DMA channel and start burst write
    write_xdata_memory(DUP_DMAARM, CH_DBG_TO_BUF0);
    burst_write_block(src, num_bytes);

    // 6. Start programming: buffer to flash
    write_xdata_memory(DUP_DMAARM, CH_BUF0_TO_FLASH);
    write_xdata_memory(DUP_FCTL, 0x0A); //0x06

    // 7. Wait until flash controller is done
    while (read_xdata_memory(DUP_FCTL) & 0x80)
        ;
}

void CCLoader::RunDUP(void)
{
    volatile unsigned char i;

    // Send two flanks on DC while keeping RESET_N low
    // All low (incl. RESET_N)
    digitalWrite(DD, LOW);
    digitalWrite(DC, LOW);
    digitalWrite(RESET, LOW);
    delay(10); // Wait

    digitalWrite(RESET, HIGH);
    delay(10); // Wait
}

void CCLoader::ProgrammerInit(void)
{
    pinMode(DD, OUTPUT);
    pinMode(DC, OUTPUT);
    pinMode(RESET, OUTPUT);
    digitalWrite(DD, LOW);
    digitalWrite(DC, LOW);
    digitalWrite(RESET, HIGH);
}
