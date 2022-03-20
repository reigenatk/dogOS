
#define ASM 1

# let the wrapper functions be globally seen
.globl keyboard_handler_wrapper, rtc_handler_wrapper

# wouldn't work unless I added an IRET. Needed to do this because otherwise it would 
# generate general protection exception. 
# https://wiki.osdev.org/Interrupt_Service_Routines- just add an ASM wrapper so 
# the function prologue (leave, ret) doesn't get called. We only need IRET here
keyboard_handler_wrapper:													
	call keyboard_INT													
	iret									

rtc_handler_wrapper:							
	call RTC_interrupt_handler						
	iret		

# #define INTEL_WRAPPER_FUNC(wrapper_name, name) \
# .globl wrapper_name														 \
# wrapper_name: 																 \
# 	call name														  			 \
# 	iret						
	
# INTEL_WRAPPER_FUNC(divide_exception_wrapper, divide_exception)
# INTEL_WRAPPER_FUNC(debug_exception_wrapper, debug_exception)
# INTEL_WRAPPER_FUNC(nmi_interrupt_wrapper, nmi_interrupt)
# INTEL_WRAPPER_FUNC(breakpoint_exception_wrapper, breakpoint_exception)
# INTEL_WRAPPER_FUNC(overflow_exception_wrapper, overflow_exception)
# INTEL_WRAPPER_FUNC(bound_range_exception_wrapper, bound_range_exception)
# INTEL_WRAPPER_FUNC(invalid_opcode_exception_wrapper, invalid_opcode_exception)
# INTEL_WRAPPER_FUNC(device_not_available_exception_wrapper, device_not_available_exception)
# INTEL_WRAPPER_FUNC(double_fault_exception_wrapper, double_fault_exception)
# INTEL_WRAPPER_FUNC(coprocessor_segment_overrun_exception_wrapper, coprocessor_segment_overrun_exception)
# INTEL_WRAPPER_FUNC(invalid_tss_exception_wrapper, invalid_tss_exception)
# INTEL_WRAPPER_FUNC( segment_not_present_exception_wrapper, segment_not_present_exception)
# INTEL_WRAPPER_FUNC( stack_fault_exception_wrapper, stack_fault_exception)
# INTEL_WRAPPER_FUNC( general_protection_exception_wrapper, general_protection_exception)
# INTEL_WRAPPER_FUNC( page_fault_exception_wrapper, page_fault_exception)
# INTEL_WRAPPER_FUNC( floating_point_error_exception_wrapper, floating_point_error_exception)
# INTEL_WRAPPER_FUNC( alignment_check_exception_wrapper, alignment_check_exception)
# INTEL_WRAPPER_FUNC( machine_check_exception_wrapper, machine_check_exception)
# INTEL_WRAPPER_FUNC( simd_fp_exception_wrapper, simd_fp_exception)