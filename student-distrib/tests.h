#ifndef TESTS_H
#define TESTS_H

// test launcher
void launch_tests();

// use this variable as a global variable, RTC will update this value and then we can check 
// to see if its increasing to prove that our RTC is working
// we will also display this somewhere on the screen as suggested in the spec
extern int rtc_test_counter;

#endif /* TESTS_H */
