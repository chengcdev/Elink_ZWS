Elink蓝牙网关：该版本功能说明
1、可以远程开锁
2、记录上报
3、可以跟电信平台进行对接
备注：
继承了之前zigbee网关Mlink协议，修改串口通信为蓝牙通信

1:2018-04-28 更改蓝牙多次发送，导致蓝牙死机
更改部分在int cmd_security_check(RecvPacket *recvPacket)函数中	
	errCnt ++;
	if(errCnt%3 == 0)
	{
		return BLE_OK;
		errCnt = 0;
	}
	else
	{
		ATCmdSend(AT_R, AT_SET, sendAtBuf);
		return BLE_OK;	
	}

2:出现第一次配置sn，用mdns配置会出现搜索不了sn

3:修改上报问题

4:远程仓库ssh地址：

git clone ssh://git@10.110.114.18:30001/chengc/ElinkBLE_GateWay.git
WEB地址
http://10.110.114.18:30000/chengc/ElinkBLE_GateWay.git

git commit -am "修改内容"   保存本地仓库

git push origin master  上传远程仓库


5：连接远程仓库：

git remote add origin ssh://git@10.110.114.18:30001/chengc/ElinkBLE_GateWay.git

6、在南京程序上，修改了上电自动进入绑定状态，绑定超时1分钟

7、跟平台还是采用之前使用的DeviceID作为登陆使用

8、发现数组大小拷贝，分配内存不够
      memcpy( g_Aes_key, g_elink_devPin, 16 );
      memcpy( g_Aes_IV, g_elink_devPin + 16, 16 );
	  但是g_Aes_key和g_Aes_IV这个定义只有16，会导致内存访问越界

9、发现base64加密，多加了\n，导致平台加密不了，需要去除
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
		line_len += 4;
		if (line_len >= 72) {
//			*pos++ = '\n';   //加上会导致平台加密不了
			line_len = 0;
		}
	}


1508592785@qq.com
cgc126827


