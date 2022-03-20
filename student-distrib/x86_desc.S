# x86_desc.S - Set up x86 segment descriptors, descriptor tables
# vim:ts=4 noexpandtab

#define ASM     1
#include "x86_desc.h"

.text

.globl ldt_size, tss_size
.globl gdt_desc, ldt_desc, tss_desc
.globl tss, tss_desc_ptr, ldt, ldt_desc_ptr
.globl gdt_ptr
.globl idt_desc_ptr, idt

.align 4


tss_size:
    .long tss_bottom - tss - 1

ldt_size:
    .long ldt_bottom - ldt - 1

    .word 0 # Padding
ldt_desc: # ldtr
    .word KERNEL_LDT # segment selector (bytes 0-15) = 0x38. 
    .long ldt # base address of segment descriptor (bytes 16-47)

.align 4
  .word 0
# The GDTR register holds the 32-bit base address and 16-bit table limit for the GDT. The base
#address specifies the linear address of byte 0 of the GDT; the table limit specifies the number of
#bytes in the table
gdt_desc: 
    .word gdt_bottom - gdt - 1 # table limit
    .long gdt # base address

    .align 4
tss:
_tss:
    .rept 104 # this is 104 because that's the size of a TSS (all the general registers, flags, etc.) Page 211 in manual
    .byte 0
    .endr
tss_bottom:

    .align  16
gdt:
_gdt:

    # First GDT entry cannot be used
    .quad 0

    # NULL entry
    .quad 0

    # Segmentation will not be used
    # CS and DS both are 0-4GB r/w segments
    #
    # The layout is (from Intel IA-32 reference manual):
    #  31        24 23  22  21  20  19   16 15  14 13 12  11   8 7          0
    # |----------------------------------------------------------------------|
    # |            |   | D |   | A |  Seg  |   |  D  |   |      |            |
    # | Base 31:24 | G | / | 0 | V | Limit | P |  P  | S | Type | Base 23:16 |
    # |            |   | B |   | L | 19:16 |   |  L  |   |      |            |
    # |----------------------------------------------------------------------|
    #
    # |----------------------------------------------------------------------|
    # |                                    |                                 |
    # | Base 15:0                          | Segment Limit 15:0              |
    # |                                    |                                 |
    # |----------------------------------------------------------------------|

gdt_ptr:
    # all entries here have S = 1 which indicates code + data segment as opposed to system segment (0)
    # Also DPL = 0 for first 2 segments but 3 for the other 2 which makes sense since its 2 kernel, 2 user
    # And finally, type (which deal with read/write access)
    # is different between CS and DS. Other than these changes, they are identical.
    
    # Set up an entry for kernel CS
    .quad 0x00CF9A000000FFFF # type here is A, which is execute/read. 

    # Set up an entry for kernel DS
    .quad 0x00CF92000000FFFF # type = 2, read/write

    # Set up an entry for user CS
    .quad 0x00CFFA000000FFFF # 

    # Set up an entry for user DS
    .quad 0x00CFF2000000FFFF

    # Set up an entry for TSS
tss_desc_ptr:
    .quad 0

    # Set up one LDT
ldt_desc_ptr:
    .quad 0

gdt_bottom:

    .align 16
ldt:
    .rept 4
    .quad 0
    .endr
ldt_bottom:

.align 4
    .word 0 # Padding
idt_desc_ptr: # IDTR, 16 bit table limit (# of bytes in table) and 32 bit linear base address of idt
    .word idt_bottom - idt - 1
    .long idt


    .align  16
idt:
_idt:
    .rept NUM_VEC
    .quad 0
    .endr

idt_bottom:
