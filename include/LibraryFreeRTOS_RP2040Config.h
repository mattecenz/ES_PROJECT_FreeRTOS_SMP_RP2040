/*

Template for configuration of the FreeRTOS library for RP2040.

If needed it can be moved (and copied) inside each test independently.

Authors:

- Matteo Briscini
- Matteo Cenzato
- Michele Adorni

*/

#ifndef LIBRARY_FREE_RTOS_RP2040_CONFIG_H
#define LIBRARY_FREE_RTOS_RP2040_CONFIG_H

// Specify the number of cores the test has to be run
#define RP2040config_testRUN_ON_CORES 2

#define RP2040config_tskSLAVE_PRIORITY  tskIDLE_PRIORITY+1
#define RP2040config_tskMASTER_PRIORITY tskIDLE_PRIORITY+2

/*
NB: due to the RT scheduling algorithm, the test procedure terminates correctly
only if tskMASTER_PRIORITY > tskSLAVE_PRIORITY
*/

// If needed can use default configurations taken from here
#include "FreeRTOSConfig.h"

#define RP2040config_tskSLAVE_STACK_SIZE  configMINIMAL_STACK_SIZE
#define RP2040config_tskMASTER_STACK_SIZE configMINIMAL_STACK_SIZE

#endif