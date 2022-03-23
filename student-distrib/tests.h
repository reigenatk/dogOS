#ifndef TESTS_H
#define TESTS_H

// test launcher
void launch_tests();

// use this variable as a global variable, RTC will update this value and then we can check 
// to see if its increasing to prove that our RTC is working
extern int rtc_test_counter;

#endif /* TESTS_H */
