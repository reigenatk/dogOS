#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
/* Use these interrupt masks to determine which interrupts are enabled and disabled. Let 1 mean interrupt disabled, 0 means enabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */
#define SLAVE_PIC_LINE 0x2

/* Initialize the 8259 PIC */
void i8259_init(void) {
  // processor should not be interrupted during this
  // but its OK since processor should've turned off interrupts already via x86_desc.S

  // send the four control words to master first, then slave
  // first one sent to the first port, last three sent to the second
  outb(ICW1, MASTER_8259_PORT);
  outb(ICW2_MASTER, MASTER_8259_PORT + 1);
  outb(ICW3_MASTER, MASTER_8259_PORT + 1);
  outb(ICW4, MASTER_8259_PORT + 1);

  outb(ICW1, SLAVE_8259_PORT);
  outb(ICW2_SLAVE, SLAVE_8259_PORT + 1);
  outb(ICW3_SLAVE, SLAVE_8259_PORT + 1);
  outb(ICW4, SLAVE_8259_PORT + 1);

  // sti(); // turn on interrupts again
  // restore_flags(flags); // restore EFLAGS

  // also initialize the master_mask and slave_mask variables
  // start all interrupts disabled because we have no devices, will enable them one by one as suggested in the spec
  master_mask = 0xFF;
  slave_mask = 0xFF;

  // enable IRQ2 on master PIC which is where slave PIC connects
  enable_irq(SLAVE_PIC_LINE);
}

unsigned int turn_bit_to_zero(unsigned char num, int bit) {
  unsigned int mask = 0xFF;
  mask ^= (1 << bit);

  // printf("%#x", mask);
  return num & mask;
 
}

/* Enable (unmask) the specified IRQ */
int turn_bit_to_one(unsigned char num, int bit) {
  unsigned int mask = (1 << bit);
  // printf("%#x", mask);
  return num | mask;
}


/* Enable (unmask) the specified IRQ by turning the proper bit to 0*/
void enable_irq(uint32_t irq_num) {
  // Masking and unmasking of interrupts on an 8259A outside of the interrupt sequence requires only a single write to the
  // second port (0x21 or 0xA1), and the byte written to this port specifies which interrupts should be masked.

  // OCW1 sets and clears the mask bits in the interrupt
  // Mask Register (IMR). M7–M0 represent the eight
  // mask bits. M = 1 indicates the channel is masked
  // (inhibited), M = 0 indicates the channel is enabled.


  if (irq_num < 8) {
    // master
    unsigned int masked = master_mask & (1 << irq_num);
    if (masked > 0) {
      // then its currently masked, but we want it off
      master_mask = turn_bit_to_zero(master_mask, irq_num);
      outb(master_mask, MASTER_8259_PORT + 1);
    }
  }
  else if (irq_num >= 8 && irq_num <= 15) {
    // slave
    irq_num -= 8;
    unsigned int masked = slave_mask & (1 << irq_num);
    if (masked > 0) {

      slave_mask = turn_bit_to_zero(slave_mask, irq_num);
      outb(slave_mask, SLAVE_8259_PORT + 1);
    }    
  }
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
  if (irq_num < 8) {
    // master
    unsigned int masked = master_mask & (1 << irq_num);
    if (masked == 0) {
      // then its currently not masked, but we want it masked
      master_mask = turn_bit_to_one(master_mask, irq_num);
      outb(master_mask, MASTER_8259_PORT + 1);
    }
  }
  else if (irq_num >= 8 && irq_num <= 15) {
    // slave
    irq_num -= 8;
    unsigned int masked = slave_mask & (1 << irq_num);
    if (masked == 0) {
      slave_mask = turn_bit_to_one(slave_mask, irq_num);
      outb(slave_mask, SLAVE_8259_PORT + 1);
    }    
  }
}

/* Send end-of-interrupt signal for the specified IRQ */
/* Send end-of-interrupt signal for the specified IRQ, which is 0x60
   When sending an EOI to master, just send to master. When sending to slave, you need to send to both master + slave
   and since slave 8259 is IRQ
*/
void send_eoi(uint32_t irq_num) {
  uint32_t val = EOI | irq_num;
  uint32_t val_slave = EOI | (irq_num - 8);
  uint8_t keyboard_IRQ_PIC = 0x02;
  if (irq_num < 8)
  {
    // master
    outb(val, MASTER_8259_PORT);
  }
  else if (irq_num >= 8 && irq_num <= 15) {
    // slave, then send to both slave + master
    outb(val_slave, SLAVE_8259_PORT);
    outb(EOI + SLAVE_PIC_LINE, MASTER_8259_PORT); // send 0x60 + 0x02, since slave is IRQ 2
  }
}
