# Profiler for performance
Timer class to profile code. Provides APIs to analyse execution time, flop rate and bandwidth with support to print elegant reports

## Features
* The user can time code with ```start``` and ```stop``` methods, with a timer name as argument
* ```setFLop``` can used to set a flop count for each timer. This helps to calculate the flop rate for the enclosed code.
* ```setMemory``` can be used to set the memory transactions for each timer. This helps to calculate the bandwidth for the enclosed code.
* Supports nested timers and prints concise and elaborate reports 

