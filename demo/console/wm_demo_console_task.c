/*****************************************************************************
*
* File Name : wm_demo_console_task.c
*
* Description: demo console task
*
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*****************************************************************************/
#include <string.h>
#include "wm_include.h"
#include "wm_demo.h"

#if DEMO_CONSOLE
#include "wm_demo_console.h"

#define    DEMO_TASK_SIZE      2048
static tls_os_queue_t 	*demo_q = NULL;
static OS_STK 			DemoTaskStk[DEMO_TASK_SIZE];
static Demo_Console 	gstConsole;

//BT
#define    MYBLE_TASK_SIZE      2048
static OS_STK 			MyBLETaskStk[MYBLE_TASK_SIZE];
#define  MYBLE_TASK_PRIO               32

//WIFI
static OS_STK 			MyBLEWIFITaskStk[MYBLE_TASK_SIZE];

#define DEMO_CONSOLE_BUF_SIZE   512

extern int strtodec(int *dec, char *str);


void demo_console_malloc(void)
{
    gstConsole.rx_buf = tls_mem_alloc(DEMO_CONSOLE_BUF_SIZE + 1);
    if(NULL == gstConsole.rx_buf)
    {
        printf("\nmalloc rx fail\n");
        return;
    }
    memset(gstConsole.rx_buf, 0, DEMO_CONSOLE_BUF_SIZE + 1);
}

s16 demo_console_rx(u16 len, void* user_data)
{
    gstConsole.rx_data_len += len;

    if (gstConsole.MsgNum < 3)
    {
        gstConsole.MsgNum ++;
        tls_os_queue_send(demo_q, (void *)1, 0);
    }

    return 0;
}

char *demo_cmd_get_first_comma(char *buf, int len)
{
    char prec = '\0', curc;
    int n = 0;
    if(len <= 0)
        return NULL;
    if(*buf == '"')
    {
        for(n = 1; n < len; n++)
        {
            curc = *(buf + n);
            if(curc == ',' && prec == '"')
            {
                if(n < 3 || *(buf + n - 2) != '\\')
                {
                    return buf + n;
                }
            }
            prec = curc;
        }
        return NULL;
    }
    else
        return strchr(buf, ',');
}

int demo_cmd_param(void *param, int type)
{
    if (type == 0)
    {
        return (int)((char *)param);
    }
    else
    {
        return *(int *)param;
    }
}

int demo_cmd_quotation_filter(u8 **keyInfo, u8 *inbuf)
{
    u8 len = strlen((char *)inbuf);
    int i;
    if (*inbuf == '"')
    {
        /* argument such as  "xxxx" */
        inbuf++; /* skip 1st <"> */
        len -= 1;
        *keyInfo = inbuf;
        if((*(inbuf + len - 1) == '"') && (*(inbuf + len) == '\0'))
        {
            *(inbuf + len - 1) = '\0';
            len -= 1;
            for(i = 0; i < len; i++)
            {
                if(inbuf[i] == '\\')
                {
                    len -= 1;
                    memcpy(inbuf + i, inbuf + i + 1, len - i);
                }
            }
            inbuf[len] = '\0';
        }
        else
        {
            return 1;
        }
    }
    else
    {
        *keyInfo = inbuf;
    }

    return 0;
}

static int demo_call_fn(int (*fn)(), int *param, int count)
{
    int ret;

    if(NULL == fn || NULL == param)
    {
        return WM_FAILED;
    }

    switch(count)
    {
    case 0:
        ret = (int)fn();
        break;
    case 1:
        ret = (int)fn(param[0]);
        break;
    case 2:
        ret = (int)fn(param[0], param[1]);
        break;
    case 3:
        ret = (int)fn(param[0], param[1], param[2]);
        break;
    case 4:
        ret = (int)fn(param[0], param[1], param[2], param[3]);
        break;
    case 5:
        ret = (int)fn(param[0], param[1], param[2], param[3], param[4]);
        break;
    case 6:
        ret = (int)fn(param[0], param[1], param[2], param[3], param[4], param[5]);
        break;
    case 7:
        ret = (int)fn(param[0], param[1], param[2], param[3], param[4], param[5], param[6]);
        break;
    case 8:
        ret = (int)fn(param[0], param[1], param[2], param[3], param[4], param[5], param[6], param[7]);
        break;
    case 9:
        ret = (int)fn(param[0], param[1], param[2], param[3], param[4], param[5], param[6], param[7], param[8]);
        break;
    case 10:
        ret = (int)fn(param[0], param[1], param[2], param[3], param[4], param[5], param[6], param[7], param[8], param[9]);
        break;
    default:
        ret = (int)fn( );
        break;
    }

    return ret;
}

int demo_cmd_execute(Demo_Console *sys)
{
    int ifcmd = 0;
    int i = 0;
    int j = 0;
    int remain_len;
    int ret = 0;

#define   MAX_DEMO_ARG 20
    int   param[MAX_DEMO_ARG];
    int   arg_count = 0;

    u8 *ptmp_param = NULL;

    u8 *buf = NULL;
	u8 *pparam_equal = NULL;
    u8 *pparam_begin = NULL;
    u8 *pparam_end = NULL;
    u8 *comma;
    u8 *arg[MAX_DEMO_ARG] = {NULL};
    int len;
	u8 *strfirst = NULL;
	u8 *str_r = NULL;
	u8 *str_n = NULL;

    for(i = 0; ; i++)
    {
    	strfirst = (u8 *)strstr((char *)sys->rx_buf, console_tbl[i].cmd);	
        if (strfirst != NULL)
        {
			/*remove \r\n from input string*/
			str_r = (u8 *)strchr((char *)strfirst, '\r');
			str_n = (u8 *)strchr((char *)strfirst, '\n');			
			if (str_r&&(str_n == NULL))
			{
				if (str_r > strfirst)
				{
					strfirst[str_r - strfirst] = '\0';
				}
			}
			else if ((str_r == NULL)&&str_n)
			{
				if (str_n > strfirst)
				{
					strfirst[str_n - strfirst] = '\0';
				}				
			}
			else if (str_r && str_n)
			{
				if (((str_r > str_n) && (str_r > strfirst))
					||((str_r < str_n) && (str_n > strfirst)))
				{
					strfirst[str_n - strfirst] = '\0';
				}
			}

            /*parser()*/
			pparam_equal = (u8 *)strchr((char *)(sys->rx_buf + strlen(console_tbl[i].cmd)), '=');
			pparam_begin = (u8 *)strchr((char *)(sys->rx_buf + strlen(console_tbl[i].cmd)), '(');
			if (pparam_equal)
			{
				if (pparam_begin && (pparam_begin > pparam_equal))
				{
					if (pparam_equal - strfirst > strlen(console_tbl[i].cmd))
					{
						continue;
					}
				}
			}
			else
			{
				if (pparam_begin && (pparam_begin - strfirst > strlen(console_tbl[i].cmd)))
				{
					continue;
				}

				/*if no '(', compare the cmd string with input string*/
				if (!pparam_begin && (strlen((char *)strfirst) != strlen(console_tbl[i].cmd)))
				{
					continue;
				}
			}

            pparam_end = (u8 *)strchr((char *)(pparam_begin + 1), ')');
            if (!pparam_begin && !pparam_end)
            {
                /*No Parameter,use default parameter to execute*/
                printf("[CMD]%s", console_tbl[i].cmd);
                for (j = 0; j < console_tbl[i].param_cnt; j++)
                {
                    if (!((console_tbl[i].type >> j) & 0x1))
                    {
                        param[j] = (int)NULL;
                    }
                    else
                    {
                        param[j] = -1;
                    }
                }
                ret = demo_call_fn((int (*)())console_tbl[i].callfn, param, console_tbl[i].param_cnt);
                if(WM_FAILED == ret)
                {
                    printf("\nrun demo failed\n");
                }
                return DEMO_CONSOLE_CMD;
            }
            else if (pparam_begin && pparam_end && ((pparam_end - pparam_begin) > 0))
            {
                remain_len =  pparam_end - pparam_begin;
                buf = pparam_begin + 1;
                arg[0] = buf;
                arg_count = 0;
                *(u8 *)pparam_end = '\0';
                while (remain_len > 0)
                {
                    comma = (u8 *)demo_cmd_get_first_comma((char *)buf, remain_len);
                    if (pparam_end && !comma)
                    {
                        if (arg_count >= (console_tbl[i].param_cnt - 1))
                            break;
                        /* last parameter */
                        *(u8 *)pparam_end = '\0';
                        remain_len -= (pparam_end - buf);
                        if(remain_len <= 1)
                            break;
                        if (pparam_end != buf)
                            arg_count++;
                        arg[arg_count] = pparam_end + 1;
                    }
                    else
                    {
                        *(u8 *)comma = '\0';
                        if (arg_count >= (console_tbl[i].param_cnt - 1))
                            break;
                        arg_count++;
                        arg[arg_count] = comma + 1;
                        remain_len -= (comma - buf + 1);
                        buf = comma + 1;
                    }
                }
                for (j = 0; j <= arg_count; j++)
                {
                    while(' ' == *(arg[j]))
                    {
                        arg[j] ++;
                    }
                    len = strlen((char *)arg[j]);
                    while(len > 0 && ' ' == *(arg[j] + len - 1))
                    {
                        *(arg[j] + len - 1) = 0;
                        len --;
                    }
                    if (!((console_tbl[i].type >> j) & 0x1))
                    {
                        if(0 == len)
                            param[j] = (int)NULL;
                        else
                        {
                            demo_cmd_quotation_filter(&ptmp_param, arg[j]);
                            param[j] = (int)ptmp_param;
                        }
                    }
                    else
                    {
                        if(0 == len)
                            param[j] = -1;
                        else
                        {
                            if (strtodec(&param[j], (char *)arg[j]) < 0)
                            {
                                printf("parameter err\r\n");
                                return DEMO_CONSOLE_WRONG_CMD;
                            }
                        }
                    }
                }

                for (j = arg_count + 1; j < console_tbl[i].param_cnt; j++)
                {
                    if (!((console_tbl[i].type >> j) & 0x1))
                    {
                        param[j] = (int)NULL;
                    }
                    else
                    {
                        param[j] = -1;
                    }
                }

                ret = demo_call_fn((int (*)())console_tbl[i].callfn, param, console_tbl[i].param_cnt);
                if(WM_FAILED == ret)
                {
                    printf("\nrun demo failed\n");
                }

                /*Use input param to execute function*/
                ifcmd = DEMO_CONSOLE_CMD;
                break;
            }
            else if (pparam_begin && !pparam_end)
            {
                return DEMO_CONSOLE_SHORT_CMD;
            }
            else
            {
                /*wrong cmd parameter,discard this cmd*/
                printf("\nwrong cmd param\n");
                //demo_console_show_help(NULL);
                return DEMO_CONSOLE_WRONG_CMD;
            }
        }

        if(strstr(console_tbl[i].cmd, "lastcmd") != NULL)	//last command
        {
            /*wrong cmd parameter,discard this cmd*/
            //demo_console_show_help(NULL);
            return DEMO_CONSOLE_WRONG_CMD;
        }
    }

    return ifcmd;
}







//console task use UART0 as communication port with PC
void demo_console_task(void *sdata)
{
    void *msg;
    int ret = 0;

    demo_console_show_help(NULL);
    demo_console_malloc();
    gstConsole.rptr = 0;
    tls_uart_set_baud_rate(TLS_UART_0, 115200);
	tls_uart_rx_callback_register(TLS_UART_0, demo_console_rx, NULL);

    for(;;)
    {
        tls_os_queue_receive(demo_q, (void **)&msg, 0, 0);
        switch((u32)msg)
        {
        case 1:
            ret = tls_uart_read(TLS_UART_0, gstConsole.rx_buf + gstConsole.rptr, gstConsole.rx_data_len);
            if(ret <= 0)
                break;
            gstConsole.rx_data_len -= ret;
            gstConsole.rptr += ret;
            ret = demo_cmd_execute(&gstConsole);	//parse command and execute if needed
            if((DEMO_CONSOLE_CMD == ret) || (DEMO_CONSOLE_WRONG_CMD == ret))	
            {
                /*modify*/
                memset(gstConsole.rx_buf, 0, DEMO_CONSOLE_BUF_SIZE);	/*After command finished transfering, clear buffer*/
                gstConsole.rptr = 0;
            }
            else if(DEMO_CONSOLE_SHORT_CMD == ret)
            {
                //param not passed all, do nothing.
            }

            if(gstConsole.MsgNum)
                gstConsole.MsgNum --;
            break;
        default:
            break;
        }
    }
}

void CreateDemoTask(void)
{
    tls_os_queue_create(&demo_q, DEMO_QUEUE_SIZE);
    tls_os_task_create(NULL, NULL,
                       demo_console_task,
                       NULL,
                       (void *)DemoTaskStk,          /* task's stack start address */
                       DEMO_TASK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
                       DEMO_TASK_PRIO,
                       0);
}


//add by zxx start
#define i2c_cht8305c_addr 0x80


/**
 * @brief	read one byte from the specified address of the eeprom
 * @param addr the eeprom address will be read from
 * @retval     the read data
 */
u8 IIC_ReadOneByte(u16 addr)
{				  
	u8 temp=0;		  	    																 
	//printf("\nread addr=%x\n",ReadAddr);
	tls_i2c_write_byte(0xA0,1);   
	tls_i2c_wait_ack(); 
    	tls_i2c_write_byte(addr,0);   
	tls_i2c_wait_ack();	    

	tls_i2c_write_byte(0xA1,1);
	tls_i2c_wait_ack();	 
	temp=tls_i2c_read_byte(0,1);
	//printf("\nread byte=%x\n",temp);
	return temp;
}

/**
 * @brief	read multibytes from the specified address of the eeprom
 * @param[in] dev_addr the dev address 
 * @param[in] addr the eeprom address will be read from
 * @param[in] buf 	Pointer to data buffer
 * @param[in] len	amount of data to be read
 * @retval    null
 */
void IIC_ReadLenByte(u8 dev_addr,u16 reg_addr,u8 *buf,u16 len)
{				  
	//printf("\nread len addr=%x\n",ReadAddr);
	//1表示开始信号
	tls_i2c_write_byte(dev_addr,1);   
	tls_i2c_wait_ack(); 
	tls_i2c_write_byte(reg_addr,0);   
	tls_i2c_wait_ack();	 
	tls_i2c_stop();
	
	tls_os_time_delay(21); 	 
	
	tls_i2c_write_byte(dev_addr|0x01,1);
	tls_i2c_wait_ack();	
	while(len > 1)
	{
		*buf++ = tls_i2c_read_byte(1,0);
		tls_i2c_wait_ack();	
		len --;
	}
   	*buf = tls_i2c_read_byte(0,1);
	tls_i2c_stop();
}

/**
 * @brief	write one byte to the specified address of the eeprom
 * @param addr the eeprom address will be write to
 * @retval     null
 */
void IIC_WriteOneByte(u16 addr, u8 data)
{				   	  	    																 
	tls_i2c_write_byte(0X80, 1); 
	tls_i2c_wait_ack();	   
	tls_i2c_write_byte(addr, 0);
	tls_i2c_wait_ack(); 										  		   
	tls_i2c_write_byte(data, 0); 				   
	tls_i2c_wait_ack();  	   
 	tls_i2c_stop();
	tls_os_time_delay(1);
}

/**
 * @brief	write multibytes from the specified address of the eeprom
 * @param[in] addr the eeprom address will be read from
 * @param[in] buf 	Pointer to data buffer
 * @param[in] len	amount of data to be write
 * @retval    null
 */
void IIC_Write(u16 addr, u8 *buf, u16 len)
{
	while(len--)
	{
		IIC_WriteOneByte(addr,*buf);
		addr++;
		buf++;
	}
} 

typedef union CHT_DATA
{
    u8 cht_i[8];
    float cht_f[2];
}CHT_DATA;

void read_cht8305c_read(CHT_DATA *cht_data)
{
	uint16 temp_i = 0;
	uint16 humi_i = 0;
	//数据说明：temp_h temp_l humi_h humi_l
	u8 cht_d[4] = {0};
	IIC_ReadLenByte(i2c_cht8305c_addr,0x00,cht_d,4);
	
	temp_i = (cht_d[0]<<8) + cht_d[1];
	humi_i = (cht_d[2]<<8) + cht_d[3];
	cht_data->cht_f[0] = (165*temp_i)/65535.0-40.0;
	cht_data->cht_f[1] = humi_i/65535.0;
	
	cht_data->cht_f[0] = (int)(cht_data->cht_f[0]*100)/100.0;
	cht_data->cht_f[1] = (int)(cht_data->cht_f[1]*100)/100.0;
	
	printf("%f   %f  %x  %x  %x  %x\r\n",cht_data->cht_f[0],cht_data->cht_f[1],cht_d[0],cht_d[1],cht_d[2],cht_d[3]);
	
}

//add by zxx end


//add by zxx start
#define R WM_IO_PB_05
#define G WM_IO_PB_25
#define B WM_IO_PB_26


typedef union RGB_DATA
{
	u8 rgb_u8[2];
    int16 rgb_i;
    
}RGB_DATA;

void led_pwm_rgb(RGB_DATA rgb_data)
{

	tls_pwm_duty_config(0,255 - (u8)(rgb_data.rgb_i>>10 & 0x1f)*8);
	tls_pwm_duty_config(1,(u8)((rgb_data.rgb_i<<5)>>10 & 0x1f)*8+1);
	tls_pwm_duty_config(2,255 - (u8)((rgb_data.rgb_i<<10)>>10 & 0x1f)*8);

	//printf("%d  pwm: %d   %d   %d ",rgb_data.rgb_i,(u8)(rgb_data.rgb_i>>10 & 0x1f)*8,255 - (u8)((rgb_data.rgb_i<<5)>>10 & 0x1f)*8,(u8)((rgb_data.rgb_i<<10)>>10 & 0x1f)*8);
	
}



void led_on_off(u8 data)
{
	//分别判断每一位数据，注意需要把每一位数据移到最低位进行判断，对应开关灯：00 开  其他数据 关
	if(data & 0x01 != 0)
		tls_gpio_write(WM_IO_PB_00,0);
	else
		tls_gpio_write(WM_IO_PB_00,1);
	if(data >> 1&0x01 != 0)
		tls_gpio_write(WM_IO_PB_01,0);
	else
		tls_gpio_write(WM_IO_PB_01,1);
	if(data >> 2&0x01 != 0)
		tls_gpio_write(WM_IO_PB_02,0);
	else
		tls_gpio_write(WM_IO_PB_02,1);	
}

//add by zxx end
//独特ID
#define CMD_START_Pos                         8U                                          /*!< CMD start position */
#define CMD_START_Msk                         (1UL << CMD_START_Pos)                         /*!< CMD start Mask */
#define RSA_BASE       0x40000000

typedef struct ID_DATA
{
	uint32 cpuid;
    u8 flash_id[8];
    u8 dev_id[18];
}ID_DATA;
/**
 * @brief          This function is used to read the flash_id id of the internal flash.
 *
 * @param[out]      flash_id                 Specified the address to save the flash_id, the length must be greater than or equals to 8 bytes.
 *
 * @retval         TLS_FLS_STATUS_OK	    if read sucsess
 * @retval         TLS_FLS_STATUS_EIO	    if read fail
 *
 * @note           The flash_id's length must be greater than or equals to 8 bytes.
 */
int readFlashUniqueId(u8 *flash_id,u8 fls_len)
{
    if (!flash_id || fls_len < 8) {
		printf("flash_id equal NULL or fls_led not equal 8\r\n");
        return -1;
    }
    M32(HR_FLASH_CMD_ADDR) = 0xBC04B;
    M32(HR_FLASH_CMD_START) = CMD_START_Msk;
    u32 *id32 = flash_id;
    for (u8 i = 0; i < 2; i++) {
        id32[i] = M32(RSA_BASE + (i + 1) * 4);
    }
    return 0;
}

uint32 GetCpuIdr(void)
{
    uint32 id = 0;

    asm volatile(
        "mfcr    %[id],    cr<13, 0>\n\t"
        :[id]"+&r"(id)
        :
        :
        );
    return id;
}

//add by zxx start
tls_os_queue_t 	*ble_wifi_q = NULL;

//add by zxx start

//全局变量，表示wifi的重连状态，0表示正常，1表示重新连接
u8 wifi_reconnect_state = 0;

void my_ble_wifi_task(void *sdata)
{
	//密码和账户的数组，其实这里可以定义指针并使用malloc灵活申请，我图方便直接定义固定长度
	u8 ssid[50];
	u8 pwd[50];
	//测试发送数组
	u8 test_data[10] = {0,1,2,3,4,5,6,7,8,9};
	//消息队列接收
	u8 *msg;
	
	while(1)
	{
		printf("wait connect..\n");
		//必须关闭socket,否者不能正常绑定端口
		close_udp_socket_demo();
		//消息队列，接收蓝牙解析任务传输的账号和密码
		tls_os_queue_receive(ble_wifi_q,&msg, 0, 0);
		printf("rev task:%s  len:%d\n",msg,strlen(msg));
		//字符串的总长度 不带结束字符的长度
		u8 msg_len = strlen(msg);	
		u8 ssid_size = 0;	//账户的长度 不带结束字符
		//下列循环主要用于找出账户的长度
		for(int i=0; msg[i] != '\0'; i++)
		{
			if(msg[i] != '+')
				ssid_size++;
			else
				break;
		}
		//拷贝账户名
		memcpy(ssid,msg,ssid_size);
		//添加结束字符
		ssid[ssid_size] = '\0';
		//拷贝密码，注意要排除加号
		memcpy(pwd,&msg[ssid_size+1],msg_len-ssid_size-1); //排除掉+号
		//末尾加上结束字符
		pwd[msg_len-ssid_size-1] = '\0'; 
		printf("ssid:%s len:%d pwd:%s len:%d \n",ssid,strlen(ssid),pwd,strlen(pwd));
		//连接WIFI
		demo_connect_net(ssid,pwd);
		//延时
		tls_os_time_delay(3000);
		//连接服务器
		socket_udp_demo(1,10086,"192.168.1.87");
		//正常情况下发送程序，当wifi_reconnect_state为1时，表示重新连接
		while(0 == wifi_reconnect_state)
		{
			udp_send_data_self(test_data,10);
			tls_os_time_delay(500);
		}
	}
	
}

//定义四种状态
#define MY_BLE_WIFI_STATE_START  		1	//开始连接
#define MY_BLE_WIFI_STATE_STOP  		2	//停止连接
#define MY_BLE_WIFI_STATE_RECONNECT  	3	//重新连接
#define MY_BLE_WIFI_STATE_CONNECT  		4	//连接wifi，并执行发送程序


void my_ble_msg_task(void *sdata)
{
	u8 msg_state[4];
	u8 ble_wifi_state = 0;
	u8 *msg;
	
	demo_bt_enable();
	while(bt_adapter_state == WM_BT_STATE_OFF)
	{
		tls_os_time_delay(5000 /HZ);
	}
	tls_os_time_delay(5000 /HZ);
	demo_ble_server_on();
	printf("ble ready ok \r\n");
	while(1)
	{
		//接收手机发送的数据，注意是数据是按照字节进行的接收
		//msg[0]表示数据长度已经包含有'\0'
		tls_os_queue_receive(ble_q,&msg, 0, 0);
		printf("rev main:%s len:%d\n",&msg[1] ,msg[0]);
		//提取前4位，前四位为状态标志
		strncpy(msg_state,&msg[1],4);
		//判断前四位的状态
		if(!strncmp(msg_state,"star",4)) ble_wifi_state = MY_BLE_WIFI_STATE_START;
		else if(!strncmp(msg_state,"stop",4)) ble_wifi_state = MY_BLE_WIFI_STATE_STOP;
		else if(!strncmp(msg_state,"reco",4)) ble_wifi_state = MY_BLE_WIFI_STATE_RECONNECT;
		else if(!strncmp(msg_state,"conn",4)) 
		{
			//conn命令至少应该是"conn+s+p'\0'" 九个字节
			if(msg[0] < 9) 	//小于9说明命令是错的
			{
				printf("conn err...\n");
				tls_ble_server_demo_api_send_notify_msg("conn err...",sizeof("conn err..."));
				ble_wifi_state = 0;
			}
			else  
				ble_wifi_state = MY_BLE_WIFI_STATE_CONNECT;
		}
		else ble_wifi_state = 0;
			
		switch(ble_wifi_state)
		{
			case MY_BLE_WIFI_STATE_CONNECT: 
				tls_ble_server_demo_api_send_notify_msg("\'conn+ssid+pwd\'",sizeof("\'conn+ssid+pwd\'"));
				wifi_reconnect_state = 0;  //重置
				tls_os_queue_send(ble_wifi_q,&msg[6], 0);
			break;
			case MY_BLE_WIFI_STATE_STOP: 
				tls_ble_server_demo_api_send_notify_msg("stop  connect",sizeof("stop connect"));
				wifi_reconnect_state = 1;	//断开连接
			break;
			case MY_BLE_WIFI_STATE_RECONNECT: 
				tls_ble_server_demo_api_send_notify_msg("start reconnect",sizeof("start reconnect"));
				wifi_reconnect_state = 1;	//断开连接
			break;
			//只发送信息至手机端
			case MY_BLE_WIFI_STATE_START: 
				tls_ble_server_demo_api_send_notify_msg("ready connect",sizeof("ready connect"));
			break;
			default: 
				printf("ble_wifi_state is err \n");
			break;
		}
	}
	
}


//创建任务
void My_task(void)
{
	//蓝牙接收消息队列
	if(tls_os_queue_create(&ble_q, 32)!=TLS_OS_SUCCESS)
	{
		printf("create queue fail\n");
		return;
	}
	//wifi连接消息队列
	if(tls_os_queue_create(&ble_wifi_q, 32)!=TLS_OS_SUCCESS)
	{
		printf("create queue fail\n");
		return;
	}
	tls_os_task_create(NULL, NULL,
                       my_ble_msg_task,		//蓝牙接收任务
                       NULL,
                       (void *)MyBLETaskStk,          /* task's stack start address */
                       MYBLE_TASK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
                       MYBLE_TASK_PRIO,
                       0);
					   
	tls_os_task_create(NULL, NULL,
                       my_ble_wifi_task,		//wifi连接任务
                       NULL,
                       (void *)MyBLEWIFITaskStk,          /* task's stack start address */
                       MYBLE_TASK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
                       MYBLE_TASK_PRIO,
                       0);


}
//add by zxx end

#endif	//DEMO_CONSOLE

