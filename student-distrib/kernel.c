/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "tests.h"

#include "paging.h"
#include "interrupt_handlers.h"
#include "keyboard.h"
#include "RTC.h"
#include "filesystem.h"

#define RUN_TESTS

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void entry(unsigned long magic, unsigned long addr) {

    multiboot_info_t *mbi;

    /* Clear the screen. */
    clear();

    /* Am I booted by a Multiboot-compliant boot loader? */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("Invalid magic number: 0x%#x\n", (unsigned)magic);
        return;
    }

    /* Set MBI to the address of the Multiboot information structure. */
    mbi = (multiboot_info_t *) addr;

    /* Print out the flags. */
    printf("flags = 0x%#x\n", (unsigned)mbi->flags);

    /* Are mem_* valid? */
    if (CHECK_FLAG(mbi->flags, 0))
        printf("mem_lower = %uKB, mem_upper = %uKB\n", (unsigned)mbi->mem_lower, (unsigned)mbi->mem_upper);

    /* Is boot_device valid? */
    if (CHECK_FLAG(mbi->flags, 1))
        printf("boot_device = 0x%#x\n", (unsigned)mbi->boot_device);

    /* Is the command line passed? */
    if (CHECK_FLAG(mbi->flags, 2))
        printf("cmdline = %s\n", (char *)mbi->cmdline);

    if (CHECK_FLAG(mbi->flags, 3)) {
        int mod_count = 0;
        int i;
        module_t* mod = (module_t*)mbi->mods_addr;
        while (mod_count < mbi->mods_count) {
            printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
            printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
            printf("First few bytes of module:\n");
            for (i = 0; i < 16; i++) {
                printf("0x%x ", *((char*)(mod->mod_start+i)));
            }
            printf("%s\n", mod->string);
            mod_count++;
            mod++;
        }
    }
    /* Bits 4 and 5 are mutually exclusive! */
    if (CHECK_FLAG(mbi->flags, 4) && CHECK_FLAG(mbi->flags, 5)) {
        printf("Both bits 4 and 5 are set.\n");
        return;
    }

    /* Is the section header table of ELF valid? */
    if (CHECK_FLAG(mbi->flags, 5)) {
        elf_section_header_table_t *elf_sec = &(mbi->elf_sec);
        printf("elf_sec: num = %u, size = 0x%#x, addr = 0x%#x, shndx = 0x%#x\n",
                (unsigned)elf_sec->num, (unsigned)elf_sec->size,
                (unsigned)elf_sec->addr, (unsigned)elf_sec->shndx);
    }

    /* Construct an LDT entry in the GDT */
    // /* Are mmap_* valid? */
    // if (CHECK_FLAG(mbi->flags, 6)) {
    //     memory_map_t *mmap;
    //     printf("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
    //             (unsigned)mbi->mmap_addr, (unsigned)mbi->mmap_length);
    //     for (mmap = (memory_map_t *)mbi->mmap_addr;
    //             (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
    //             mmap = (memory_map_t *)((unsigned long)mmap + mmap->size + sizeof (mmap->size)))
    //         printf("    size = 0x%x, base_addr = 0x%#x%#x\n    type = 0x%x,  length    = 0x%#x%#x\n",
    //                 (unsigned)mmap->size,
    //                 (unsigned)mmap->base_addr_high,
    //                 (unsigned)mmap->base_addr_low,
    //                 (unsigned)mmap->type,
    //                 (unsigned)mmap->length_high,
    //                 (unsigned)mmap->length_low);
    // }

    // print bootloader name
    if (CHECK_FLAG(mbi->flags, 9)) {
        printf("bootlader name: %s\n", mbi->boot_loader_name);
    }

    /* Construct an LDT entry in the GDT. Before this its been set to ".quad 0" in x86_desc.S, so all zeros. Now we intiialize it*/
    {
        seg_desc_t the_ldt_desc;
        the_ldt_desc.granularity = 0x0; // is segment limit in byte units or 4kb units
        the_ldt_desc.opsize      = 0x1; // 0 = 16bit segmenet, 1 = 32 bit segment. Use 1 always for 32 bit addresses (which we are doing) 
        the_ldt_desc.reserved    = 0x0; // always set this to zero
        the_ldt_desc.avail       = 0x0; // avilable to system use
        the_ldt_desc.present     = 0x1;
        the_ldt_desc.dpl         = 0x0; // DPL, privlege level of the segment. 
        the_ldt_desc.sys         = 0x0; // Specifies whether the segment descriptor is for a system segment (S flag is clear) or a code or data segment (S flag is set).
        the_ldt_desc.type        = 0x2;

        // all this does is fill in the rest of the 64 bit desciptor entry, since up to now we've only done the flags
        SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);

        // sets the variable in x86_desc.h to this newly made descriptor
        // and that variable is declared extern so what I think this basically is doing is, it 
        // lets us write directly to the memory address that the x86_desc.S's ldt_desc_ptr label would be pointing to
        ldt_desc_ptr = the_ldt_desc;

        lldt(KERNEL_LDT);
    }

    /* Construct a TSS entry in the GDT */
    {
        seg_desc_t the_tss_desc;
        the_tss_desc.granularity   = 0x0;
        the_tss_desc.opsize        = 0x0;
        the_tss_desc.reserved      = 0x0;
        the_tss_desc.avail         = 0x0;
        the_tss_desc.seg_lim_19_16 = TSS_SIZE & 0x000F0000;
        the_tss_desc.present       = 0x1;
        the_tss_desc.dpl           = 0x0;
        the_tss_desc.sys           = 0x0;
        the_tss_desc.type          = 0x9;
        the_tss_desc.seg_lim_15_00 = TSS_SIZE & 0x0000FFFF;

        // set tss descriptor in gdt to point to tss
        SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

        // sets tss seg desc to address of descriptor which will go in gdt
        tss_desc_ptr = the_tss_desc;

        tss.ldt_segment_selector = KERNEL_LDT;
        tss.ss0 = KERNEL_DS;
        tss.esp0 = 0x800000;

        // load task register with the 16bit seg selector to the tss entry in gdt
        // implicitly populates the shadow register too for tss descirptor
        ltr(KERNEL_TSS);
    }

    // populate the interrupt descriptor table
    printf("Populating IDT with descriptors");

    init_interrupt_descriptors();

    // initiate filesystem
    printf("Init Filesystem");
    module_t* mod = (module_t*)mbi->mods_addr;
    init_filesystem(mod->mod_start);

    /* Init the PIC + all exception handlers */
    printf("Initializing PIC");
    i8259_init();

    // devices (Keyboard + RTC)
    printf("Initializing Keyboard\n");
    init_keyboard();
    printf("Initializing RTC\n");
    open_RTC("rtc");

    // paging
    printf("Initializing Paging");
    setup_paging();

    init_terminal();

    

    /* Enable interrupts */
    /* Do not enasble the following until after you have set up your
        * IDT correctly otherwise QEMU will triple fault and simple close
        * without showing you any output */

    printf("Enabling Interrupts\n");
    sti();

#ifdef RUN_TESTS
    /* Run tests */
    launch_tests();
#endif
    /* Execute the first program ("shell") ... */

    /* Spin (nicely, so we don't chew up cycles) */
    asm volatile (".1: hlt; jmp .1;");
}
