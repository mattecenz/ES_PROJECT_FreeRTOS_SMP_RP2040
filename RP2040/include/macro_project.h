
#ifndef MACRO_PROJECT_H
#define MACRO_PROJECT_H

/*

Macro used to send the data over a generic channel defined by the user.

Parameters:

- struct_shared_1: char[] containing the first datum already serialized ready to be sent over the channel
- struct_shared_2: char[] containing the second datum already serialized ready to be sent over the channel
- function_name: name of the function which has to be executed when sending

*/

# define SEND_RESULTS_OVER_DRIVER(function_name, __VA_ARGS__) \
    int err_1=function_name(__VA_ARGS__);


/*

Macro used to launch the application on both cores using the FreeRTOS scheduler.

*/

# define LAUNCH_FUNCTION_BOTH_CORES(var_res1, var_res2, output_type, function_name, __VA_ARGS__)

#endif MACRO_PROJECT_H