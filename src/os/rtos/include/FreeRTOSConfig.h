/*
    FreeRTOS V7.0.2 - Copyright (C) 2011 Real Time Engineers Ltd.
	

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H



/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE. 
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION		1	//使用抢先式内核
#define configUSE_IDLE_HOOK			1	//使用空闲钩子
#define configUSE_TICK_HOOK			0	//不使用时间片钩子

#define configCPU_CLOCK_HZ			( ( unsigned long ) 40000000 )	/* =12.0MHz xtal multiplied by 5 using the PLL. *///内部处理器执行频率

#define configTICK_RATE_HZ			( ( portTickType ) 500u)//500u )	//时间片中断的频率
#define configMAX_PRIORITIES		( ( unsigned portBASE_TYPE ) 63)	//应用程序中可用优先级的数目
#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 90 )	//空闲任务使用的堆栈大小
#define configTOTAL_HEAP_SIZE		( ( size_t ) 12 * 1024 )		//内核中可用的RAM数量,heap2使用
#define configMAX_TASK_NAME_LEN		( 8 )	//创建任务名称最大允许长度
#define configUSE_TRACE_FACILITY	1		//是否使用可视化追踪
#define configUSE_16_BIT_TICKS		0	//定义portTickType
#define configIDLE_SHOULD_YIELD		1	//讲阻止空闲任务让出时间直到他的时间片用完
#define configUSE_HEAP3				0

#define configQUEUE_REGISTRY_SIZE 	0
#define configSEMAPHORE_INIT_VALUE	5	//创建信号量的最大计数值

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 		0	//不使用合作轮转式程序
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )	//合作式应用程序中可用的优先级数目

#define configTIMER_TASK_STACK_DEPTH 400


/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources	0
#define INCLUDE_vTaskSuspend			1
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay				1

#define configUSE_MUTEXES               1
#define configUSE_RECURSIVE_MUTEXES     1

#define INCLUDE_xTaskGetCurrentTaskHandle   1 //可以获取当前任务


#endif /* FREERTOS_CONFIG_H */
