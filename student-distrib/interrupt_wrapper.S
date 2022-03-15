
#define ASM 1

.globl keyboard_handler_wrapper, rtc_handler_wrapper

# The reason we don't need to import rtc.h or keyboard.h is because the interrupt handler functions are 
# extern and therefore global, so compiler knows what its a reference to here.

keyboard_handler_wrapper:
	pushal	# push all general purpose registers								
	pushfl	# push flags register EFLAGS 								
	call keyboard_INT					
	popfl									
	popal									
	iret									

rtc_handler_wrapper:
	pushal									
	pushfl									
	call RTC_interrupt_handler				
	popfl									
	popal									
	iret		
