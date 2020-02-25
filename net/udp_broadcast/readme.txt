/**
  @page "udp_broadcast"  demo
  
  @verbatim
  ******************** (C) COPYRIGHT 2016 MXCHIP MiCO SDK*******************
  * @file    udp_broadcast/readme.txt 
  * @author  MDWG (MiCO Documentation Working Group)
  * @version v2.4.x
  * @date    11-April-2016
  * @brief   Description of the  "udp_broadcast"  demo.
  ******************************************************************************

 @par Demo Description 
  This demo shows:  how to start an UDP broadcast sevice  and transmit information to  designated port after Easylink configuration.


@par Directory contents 
    - Demos/tcpip/udp_broadcast/udp_broadcast.c       UDP broadcast demo program
    - Demos/tcpip/udp_broadcast/mico_config.h         MiCO function header file


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
 - View operating results and system serial log (Serial port: Baud rate: 115200, data bits: 8bit, parity: No, stop bits: 1).   Please referring to http://mico.io/wiki/doku.php?id=com.mxchip.basic

**/

