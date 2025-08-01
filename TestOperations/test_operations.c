#include "LibraryFreeRTOS_RP2040.h"
#include "ApplicationHooks.h"

#define type_t int32_t

#pragma GCC push_options
#pragma GCC optimize ("O0")

static type_t addition(type_t num1, type_t num2){
    return num1+num2;
}

static type_t subtraction(type_t num1, type_t num2){
    return num1-num2;
}

static type_t multiplication(type_t num1, type_t num2){
    return num1*num2;
}

static type_t division(type_t num1, type_t num2){
    return num1/num2;
}


static bool check_equals(type_t return_core_0, type_t return_core_1) {
    return return_core_0 == return_core_1+rand()%5; //rand is used to simulate a small error in the result
}

#pragma GCC pop_options

create_multicore_function_validator(test_addition, 
    type_t, 
    "%ld", 
    addition,
    check_equals,
    10,
    3)

create_multicore_function_validator(test_subtraction, 
    type_t, 
    "%ld", 
    subtraction,
    DEFAULT_CHECK,
    10,
    3)

create_multicore_function_validator(test_multiplication, 
    type_t, 
    "%ld", 
    multiplication,
    check_equals,
    10,
    3)

create_multicore_function_validator(test_division, 
    type_t, 
    "%ld", 
    division,
    DEFAULT_CHECK,
    10,
    3)

int main(void) {

    start_hw();

    start_master(test_addition);
    start_master(test_subtraction);
    start_master(test_multiplication);
    start_master(test_division);


    start_FreeRTOS();
}