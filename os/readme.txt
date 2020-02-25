/**
  @page " os "  demo
  
  @verbatim
  ******************** (C) COPYRIGHT 2016 MXCHIP MiCO SDK*******************
  * @file    os/readme.txt 
  * @author  MDWG (MiCO Documentation Working Group)
  * @version v2.4.x
  * @date    11-April-2016
  * @brief   Description of the  ��os��  demo.
  ******************************************************************************


  @par Demo Description 
This demo inclues the following five parts:
    -os.thread.c: MiCO RTOS Thread control demo. Show how to creat two threads and realize specifical function.
    -os.timer.c:   MiCO RTOS Timer demo. Show how to count numbers based on timer.
    -os.mutex.c: MiCO RTOS Mutex demo. Show how to realize multi-thread function based on mutex.
    -os.sem.c:    MiCO RTOS Sem demo. Show how to use sem to control thread.
    -os.queue.c: MiCO RTOS Queue demo. Show how to creat and use queue.
 

@par Directory contents 
    - Demos/os/os_thread.c        RTOS thread controller demo
    - Demos/os/os_timer.c         RTOS timer demo
    - Demos/os/os_mutex.c         RTOS mutex demo
    - Demos/os/os_sem.c           RTOS semaphore demo
    - Demos/os/os_queue.c         RTOS message queue demo
    - Demos/os/mico_config.h      MiCO function header file


@par Hardware and Software environment       
    - This demo has been tested on MiCOKit-3165 board.
    - This demo can be easily tailored to any other supported device and development board.


@par How to use it ? 
In order to make the program work, you must do the following :
 - Open your preferred toolchain, 
    - IDE:  IAR 7.30.4 or Keil MDK 5.13.       
    - Debugging Tools: JLINK or STLINK
 - Modify header file path of  "mico_config.h".   Please referring to "http://mico.io/wiki/doku.php?id=confighchange"
 - Rebuild all files and load your image into target memory.   Please referring to "http://mico.io/wiki/doku.php?id=debug"
 - Run the demo.
 - View operating results and system serial log (Serial port: Baud rate: 115200, data bits: 8bit, parity: No, stop bits: 1).   Please referring to http://mico.io/wiki/doku.php?id=Demos

**/

