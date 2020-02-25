/*
 * elink_logic.h
 *
 *  Created on: 2018年7月31日
 *      Author: hxfky
 */

#ifndef APPLICATION_MLINKDEMO_ELINK_COMMON_ELINK_LOGIC_H_
#define APPLICATION_MLINKDEMO_ELINK_COMMON_ELINK_LOGIC_H_

typedef struct
{
	int loginTcpFd;
	int loginFlag;	//
	int loginFailCount;
	int loginAuthInterval;
	int loginAuthTimeoutInterval;
	int loginlasttime;
	int loginBind;
	char sequence[16];
}eLink_DevChannel_Param;

#if 0
unsigned char g_elink_devid[48]=
{   "00B00200030100110100010000CG3E0011"};
unsigned char g_elink_devPin[48]=
{   "60782249592283024575511663321568"};
unsigned char g_sessionKey[16]=
{   "cd3d633670f29c1c"};
#else
#if(TEST == 1)
unsigned char g_elink_devid[48] = { "00B03000FF010410014453904847955389" };//01

unsigned char g_elink_devPin[48] = { "72011541886666940364097774171293" };//01

unsigned char *sublock_deviceid = "00B03000041001100200010000CG3E0011";     //子设备deviceid 01

unsigned char *lock_pin = "74156234486343278387744859063799";     // 01
#elif(TEST == 2)
#if 0
unsigned char g_elink_devid[48] = { "00B03000FF010410014170829966587436" }; // 02

unsigned char g_elink_devPin[48] = { "39170222910112169810256002620447" }; // 02
#else
unsigned char g_elink_devid[48] = { "00B0300004100110025386628623101946" }; // 02
//unsigned char g_elink_devPin[48] = "39170222910112169810256002620447"; // 02
unsigned char g_elink_devPin[48] = "EKQK8NDGL24R73YUW7JS9AVF26POP6GD"; // 02
#endif

unsigned char *sublock_deviceid = "00B03000041001100200010000CG3E0012";     //子设备deviceid 02

unsigned char *lock_pin = "39170222910112169810256002620447";     // 02
#elif(TEST == 3)
unsigned char g_elink_devid[48] = { "00B03000FF010410010349777855769314" };  // 03

unsigned char g_elink_devPin[48] = { "74156234486343278387744859063799" }; // 03

unsigned char *sublock_deviceid = "00B03000041001100200010000CG3E0013";     //子设备deviceid 03

unsigned char *lock_pin = "72011541886666940364097774171293";     // 03
#endif

unsigned char g_sessionKey[16] = { "cd3d633670f29c1c" };
#endif

#endif /* APPLICATION_MLINKDEMO_ELINK_COMMON_ELINK_LOGIC_H_ */
