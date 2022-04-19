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


void led_rgb(u8 data)
{
	switch(data)
	{
		case 0x01:tls_gpio_write(R,0);tls_gpio_write(G,1);tls_gpio_write(B,1);break;//1
		case 0x02:tls_gpio_write(R,1);tls_gpio_write(G,0);tls_gpio_write(B,1);break;//2
		case 0x03:tls_gpio_write(R,1);tls_gpio_write(G,1);tls_gpio_write(B,0);break;//3
		case 0x04:tls_gpio_write(R,0);tls_gpio_write(G,0);tls_gpio_write(B,1);break;//12
		case 0x05:tls_gpio_write(R,0);tls_gpio_write(G,1);tls_gpio_write(B,0);break;//13
		case 0x06:tls_gpio_write(R,1);tls_gpio_write(G,0);tls_gpio_write(B,0);break;//23
		default:break;
	}
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

//add by zxx start
//发送数据的长度：0x5a+data_len+校验+0xa5
////校验位是有效数据的和不包括帧头和尾部,cpu8字节温湿度8字节
bt_send_data_len = 11+8;
typedef union ADC_DATA
{
    u8 adc_i[8];
	//包含CPU电压和温度
    float adc_f[2];
}ADC_DATA;


void my_ble_msg_task(void *sdata)
{

	//传过来的消息队列指针，这里我定义的是u8类型的
	u8 *msg;
	//蓝牙发送数据
	u8 send_data[20] = {0x00}; 
	ADC_DATA adc_data;
	CHT_DATA cht_data;
	RGB_DATA rgb_data;

	demo_bt_enable();
	while(bt_adapter_state == WM_BT_STATE_OFF)
	{
		tls_os_time_delay(5000 /HZ);
	}
	tls_os_time_delay(5000 /HZ);
	demo_ble_server_on();
	printf("ble ready ok \r\n");
	
	//初始化三个IO口，注意对应开发板上的IO 这里使用5 25 26
	//tls_gpio_cfg(WM_IO_PB_01, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_FLOATING);
	//tls_gpio_cfg(WM_IO_PB_02, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_FLOATING);
	//tls_gpio_cfg(WM_IO_PB_26, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_FLOATING);
	

	//PWM
	pwm_demo(0,100000,255,4,0);
	pwm_demo(1,100000,255,4,0);
	pwm_demo(2,100000,255,4,0);
	
	//IIC INIT
	wm_i2c_scl_config(WM_IO_PA_01);
    wm_i2c_sda_config(WM_IO_PA_04);
    
	tls_i2c_init(5000);
	
	for(;;)
	{


		while (0)
			{
						read_cht8305c_read(&cht_data);
						tls_os_time_delay(100);
					
			}
	
		//接收手机发送的数据，注意是数据是按照字节进行的接收
		tls_os_queue_receive(ble_q,&msg, 0, 0);
		//打印ble收到数据的长度
		printf("ble revice len:%d\n",msg[0]);
		//依次打印收到的ble数据 头 led(1) pwm_led(2) 校验和 尾
		for(u8 i=0;i<msg[0];i++){
			printf("0x%x ",msg[i+1]);
		}printf("\n");
		
		rgb_data.rgb_u8[0] = msg[2];
		rgb_data.rgb_u8[1] = msg[3];
		

		//pu信息信息：
		adc_data.adc_f[0] = (float)adc_get_interVolt()/1000.0;
		adc_data.adc_f[1] = (float)adc_temp()/1000.0;
		//温湿度传感器信息
		read_cht8305c_read(&cht_data);
		
		//led_rgb(msg[1]);
		//RGB开
		if(msg[1]>>3 &0x01 != 0)
			led_pwm_rgb(rgb_data);
		else
		{
			tls_pwm_duty_config(0,255);
			tls_pwm_duty_config(1,255);
			tls_pwm_duty_config(2,255);
		}
		//led_on_off(msg[1]);
		
		//0xa5(0) 8个数据(1-8) 校验(9) 0x5a(10)
		//加入头帧
		send_data[0] = 0xA5;
		send_data[bt_send_data_len-2] = 0;
		for(u8 i=0;i<8;i++)
		{
			//转存cpu电压和温度数据到发送数组
			send_data[i+1] = adc_data.adc_i[i];
			//转存温湿度传感器数据到发送数组
			send_data[i+8+1] = cht_data.cht_i[i];
			//计算校验和，有效数据之和
			send_data[bt_send_data_len-2] += adc_data.adc_i[i];
			send_data[bt_send_data_len-2] += cht_data.cht_i[i];
		}
		//加入尾帧
		send_data[bt_send_data_len-1] = 0x5A;
		
		//printf("send state:%d \r\n",tls_ble_server_demo_api_send_msg(send_data,3));
		tls_ble_server_demo_api_send_notify_msg(send_data,bt_send_data_len);
	}
	
}


//创建任务
void My_task(void)
{
	if(tls_os_queue_create(&ble_q, 32)!=TLS_OS_SUCCESS)
	{
		printf("create queue fail\n");
		return;
	}
	tls_os_task_create(NULL, NULL,
                       my_ble_msg_task,
                       NULL,
                       (void *)MyBLETaskStk,          /* task's stack start address */
                       MYBLE_TASK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
                       MYBLE_TASK_PRIO,
                       0);


}

//add by zxx end

#endif	//DEMO_CONSOLE

