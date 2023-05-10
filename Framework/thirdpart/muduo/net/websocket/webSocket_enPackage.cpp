/*******************************************************************************
 * 名称: webSocket_enPackage
 * 功能: websocket数据收发阶段的数据打包, 通常client发server的数据都要isMask(掩码)处理, 反之server到client却不用
 * 形参: *data：准备发出的数据
 *          dataLen : 长度
 *        *package : 打包后存储地址
 *        packageMaxLen : 存储地址可用长度
 *          isMask : 是否使用掩码     1要   0 不要
 *          type : 数据类型, 由打包后第一个字节决定, 这里默认是数据传输, 即0x81
 * 返回: 打包后的长度(会比原数据长2~16个字节不等)      <=0 打包失败
 * 说明: 无
 ******************************************************************************/
int webSocket_enPackage(unsigned char *data, unsigned int dataLen, unsigned char *package, unsigned int packageMaxLen, bool isMask, WebsocketData_Type type)
{
    unsigned char maskKey[4] = {0};    // 掩码
    unsigned char temp1, temp2;
    int count;
    unsigned int i, len = 0;
    
    if(packageMaxLen < 2)
        return -1;
    
    if(type == WDT_MINDATA)
        *package++ = 0x00;
    else if(type == WDT_TXTDATA)
        *package++ = 0x81;
    else if(type == WDT_BINDATA)
        *package++ = 0x82;
    else if(type == WDT_DISCONN)
        *package++ = 0x88;
    else if(type == WDT_PING)
        *package++ = 0x89;
    else if(type == WDT_PONG)
        *package++ = 0x8A;
    else if(type == 100)
        *package++ = 0x02;//add by cheyang//return -1;
	else if(type == 99)
        *package++ = 0x80;//add by cheyang//return -1;
		
    //
    if(isMask)
        *package = 0x80;
    len += 1;
    //
    if(dataLen < 126)
    {
        *package++ |= (dataLen&0x7F);
        len += 1;
    }
    else if(dataLen < 65536)
    {
        if(packageMaxLen < 4)
            return -1;
        *package++ |= 0x7E;
        *package++ = (char)((dataLen >> 8) & 0xFF);
        *package++ = (unsigned char)((dataLen >> 0) & 0xFF);
        len += 3;
    }
    else if(dataLen < 0xFFFFFFFF)
    {
        if(packageMaxLen < 10)
            return -1;
        *package++ |= 0x7F;
        *package++ = 0; //(char)((dataLen >> 56) & 0xFF);   // 数据长度变量是 unsigned int dataLen, 暂时没有那么多数据
        *package++ = 0; //(char)((dataLen >> 48) & 0xFF);
        *package++ = 0; //(char)((dataLen >> 40) & 0xFF);
        *package++ = 0; //(char)((dataLen >> 32) & 0xFF);
        *package++ = (char)((dataLen >> 24) & 0xFF);        // 到这里就够传4GB数据了
        *package++ = (char)((dataLen >> 16) & 0xFF);
        *package++ = (char)((dataLen >> 8) & 0xFF);
        *package++ = (char)((dataLen >> 0) & 0xFF);
        len += 9;
    }
    //
    if(isMask)    // 数据使用掩码时, 使用异或解码, maskKey[4]依次和数据异或运算, 逻辑如下
    {
        if(packageMaxLen < len + dataLen + 4)
            return -1;
        webSocket_getRandomString(maskKey, sizeof(maskKey));    // 随机生成掩码
        *package++ = maskKey[0];
        *package++ = maskKey[1];
        *package++ = maskKey[2];
        *package++ = maskKey[3];
        len += 4;
		if(type == WDT_TXTDATA){
				printf("the key is %d,%d,%d,%d.\n",maskKey[0],maskKey[1],maskKey[2],maskKey[3]);
		}
        for(i = 0, count = 0; i < dataLen; i++)
        {
            temp1 = maskKey[count];
            temp2 = data[i];
			if(type == WDT_TXTDATA){
				printf("the file name is %d,key:%d,\n",temp2,maskKey[count]);
			}
            *package++ = (char)(((~temp1)&temp2) | (temp1&(~temp2)));  // 异或运算后得到数据
			if(type == WDT_TXTDATA){
				printf("after key name is %d,\n",(char)(((~temp1)&temp2) | (temp1&(~temp2))));
			}
            count += 1;
            if(count >= sizeof(maskKey))    // maskKey[4]循环使用
                count = 0;
        }
        len += i;
        *package = '\0';
    }
    else    // 数据没使用掩码, 直接复制数据段
    {
        if(packageMaxLen < len + dataLen)
            return -1;
        memcpy(package, data, dataLen);
        package[dataLen] = '\0';
        len += dataLen;
    }
    //
    return len;
}