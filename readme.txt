Elink�������أ��ð汾����˵��
1������Զ�̿���
2����¼�ϱ�
3�����Ը�����ƽ̨���жԽ�
��ע��
�̳���֮ǰzigbee����MlinkЭ�飬�޸Ĵ���ͨ��Ϊ����ͨ��

1:2018-04-28 ����������η��ͣ�������������
���Ĳ�����int cmd_security_check(RecvPacket *recvPacket)������	
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

2:���ֵ�һ������sn����mdns���û������������sn

3:�޸��ϱ�����

4:Զ�ֿ̲�ssh��ַ��

git clone ssh://git@10.110.114.18:30001/chengc/ElinkBLE_GateWay.git
WEB��ַ
http://10.110.114.18:30000/chengc/ElinkBLE_GateWay.git

git commit -am "�޸�����"   ���汾�زֿ�

git push origin master  �ϴ�Զ�ֿ̲�


5������Զ�ֿ̲⣺

git remote add origin ssh://git@10.110.114.18:30001/chengc/ElinkBLE_GateWay.git

6�����Ͼ������ϣ��޸����ϵ��Զ������״̬���󶨳�ʱ1����

7����ƽ̨���ǲ���֮ǰʹ�õ�DeviceID��Ϊ��½ʹ��

8�����������С�����������ڴ治��
      memcpy( g_Aes_key, g_elink_devPin, 16 );
      memcpy( g_Aes_IV, g_elink_devPin + 16, 16 );
	  ����g_Aes_key��g_Aes_IV�������ֻ��16���ᵼ���ڴ����Խ��

9������base64���ܣ������\n������ƽ̨���ܲ��ˣ���Ҫȥ��
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
		line_len += 4;
		if (line_len >= 72) {
//			*pos++ = '\n';   //���ϻᵼ��ƽ̨���ܲ���
			line_len = 0;
		}
	}


1508592785@qq.com
cgc126827


