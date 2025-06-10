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

#pragma GCC pop_options

create_test_pipeline_function(test_addition, 
    type_t, 
    "%ld", 
    addition,
    10,
    3)

create_test_pipeline_function(test_subtraction, 
    type_t, 
    "%ld", 
    subtraction,
    10,
    3)

create_test_pipeline_function(test_multiplication, 
    type_t, 
    "%ld", 
    multiplication,
    10,
    3)

create_test_pipeline_function(test_division, 
    type_t, 
    "%ld", 
    division,
    10,
    3)

int main(void) {

    start_hw();

    start_test_pipeline(test_addition);
    start_test_pipeline(test_subtraction);
    start_test_pipeline(test_multiplication);
    start_test_pipeline(test_division);

    start_FreeRTOS();
}