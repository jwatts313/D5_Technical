# D5_Technical
Print Hello World to USART for STM32f4 dev kit simulation using renode
# Design Considerations
## libopencm3
libopencm3 (https://github.com/libopencm3/libopencm3) provides a multitude of dev boards to run in renode. libopencm3 also provides templates and examples for quickly creating projects
## STM32f4
There are a lot of easy to find resources for creating projects for the STM32f4, as wel as one of the available dev boards inside of the libopencm3 repository
# Quick Test Guide
* Clone the repository
* In the command line navigate to the directory `D5_Technical/interrupt/example/renode`. 
* Start renode (https://github.com/renode/renode use the provided readme files for proper installation if renode is not already installed), in the monitor window type in `mach create` creates a machine
* Type `machine LoadPlatformDescription @platforms/boards/stm32f4_discovery-kit.repl` configures the machine with a prebuilt STM32f4 devkit configuration
* Type `sysbus LoadELF @renode-example.elf` this is the hello world firmware
* Type `machine LoadPlatformDescription @add-ccm.repl` adds CCM RAM to the machine
* Type `showAnalyzer sysbus.usart2` this opens a window for the usart2
* Type `start` to run the machine and print Hello World to the usart2 window.
Below is my renode monitor input, my usart2 output, and debug terminal
![D5technical](https://user-images.githubusercontent.com/61755803/228134833-98e1607c-51e4-4dfd-8f1b-0f78c5176ee5.png)
