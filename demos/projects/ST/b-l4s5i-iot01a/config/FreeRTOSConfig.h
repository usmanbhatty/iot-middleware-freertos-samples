/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

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

/* Ensure stdint is only used by the compiler, and not the assembler. */
#if defined( __ICCARM__ ) || defined( __CC_ARM ) || defined( __GNUC__ )
    #include <stdint.h>
    extern uint32_t SystemCoreClock;
#endif

#define configSUPPORT_STATIC_ALLOCATION              1

#define configUSE_PREEMPTION                         1
#define configUSE_IDLE_HOOK                          1
#define configUSE_TICK_HOOK                          0
#define configUSE_TICKLESS_IDLE                      0
#define configUSE_DAEMON_TASK_STARTUP_HOOK           1
#define configCPU_CLOCK_HZ                           ( SystemCoreClock )
#define configTICK_RATE_HZ                           ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                         ( 7 )
#define configMINIMAL_STACK_SIZE                     ( ( uint16_t ) 90 )
#define configTOTAL_HEAP_SIZE                        ( ( size_t ) ( 60 * 1024 ) )
#define configMAX_TASK_NAME_LEN                      ( 16 )
#define configUSE_TRACE_FACILITY                     1
#define configUSE_16_BIT_TICKS                       0
#define configIDLE_SHOULD_YIELD                      1
#define configUSE_MUTEXES                            1
#define configQUEUE_REGISTRY_SIZE                    8
#define configCHECK_FOR_STACK_OVERFLOW               2
#define configUSE_RECURSIVE_MUTEXES                  1
#define configUSE_MALLOC_FAILED_HOOK                 1
#define configUSE_APPLICATION_TASK_TAG               1
#define configUSE_COUNTING_SEMAPHORES                1
#define configGENERATE_RUN_TIME_STATS                0
#define configOVERRIDE_DEFAULT_TICK_CONFIGURATION    1
#define configRECORD_STACK_HIGH_ADDRESS              1
#define configUSE_STATS_FORMATTING_FUNCTIONS         1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                        0
#define configMAX_CO_ROUTINE_PRIORITIES              ( 2 )

/* Software timer definitions. */
#define configUSE_TIMERS                             1
#define configTIMER_TASK_PRIORITY                    ( configMAX_PRIORITIES - 2 )
#define configTIMER_QUEUE_LENGTH                     10
#define configTIMER_TASK_STACK_DEPTH                 ( configMINIMAL_STACK_SIZE * 10 )

/* Set the following definitions to 1 to include the API function, or zero
 * to exclude the API function. */
#define INCLUDE_vTaskPrioritySet                     1
#define INCLUDE_uxTaskPriorityGet                    1
#define INCLUDE_vTaskDelete                          1
#define INCLUDE_vTaskCleanUpResources                0
#define INCLUDE_vTaskSuspend                         1
#define INCLUDE_vTaskDelayUntil                      1
#define INCLUDE_vTaskDelay                           1
#define INCLUDE_uxTaskGetStackHighWaterMark          1
#define INCLUDE_xTaskGetSchedulerState               1

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
    /* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
    #define configPRIO_BITS    __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS    4                                 /* 15 priority levels. */
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
 * function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         0xf

/* The highest interrupt priority that can be used by any interrupt service
 * routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
 * INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
 * PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    1

/* Interrupt priorities used by the kernel port layer itself.  These are generic
* to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
 * See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* Normal assert() semantics without relying on the provision of an assert.h
 * header file. */
#define configASSERT( x )                                        \
    if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ; ; ) {; } \
    }

/* Logging task definitions. */
void vLoggingPrintf( const char * pcFormat,
                     ... );

/* Map the FreeRTOS printf() to the logging task printf. */
#define configPRINTF( x )    vLoggingPrintf x
/*#define configPRINTF( x ) */

/* Map the logging task's printf to the board specific output function. */
#define configPRINT_STRING( x )    vMainUARTPrintString( x );

/* Sets the length of the buffers into which logging messages are written - so
 * also defines the maximum length of each log message. */
#define configLOGGING_MAX_MESSAGE_LENGTH            160

/* Set to 1 to prepend each log message with a message number, the task name,
 * and a time stamp. */
#define configLOGGING_INCLUDE_TIME_AND_TASK_NAME    1

/* The size of the global output buffer that is available for use when there
 *  are multiple command interpreters running at once (for example, one on a UART
 *  and one on TCP/IP).  This is done to prevent an output buffer being defined by
 *  each implementation - which would waste RAM.  In this case, there is only one
 *  command interpreter running, and it has its own local output buffer, so the
 *  global buffer is just set to be one byte long as it is not used and should not
 *  take up unnecessary RAM. */
#define configCOMMAND_INT_MAX_OUTPUT_SIZE           1

/* Pseudo random number generator, just used by demos so does not have to be
 * secure.  Do not use the standard C library rand() function as it can cause
 * unexpected behaviour, such as calls to malloc(). */
extern int iMainRand32( void );
#define configRAND32()    iMainRand32()

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
 * standard names. */
#define vPortSVCHandler       SVC_Handler
#define xPortPendSVHandler    PendSV_Handler
#define vHardFault_Handler    HardFault_Handler

/* IMPORTANT: This define MUST be commented when used with STM32Cube firmware,
 *            to prevent overwriting SysTick_Handler defined within STM32Cube HAL. */
/* #define xPortSysTickHandler SysTick_Handler */

#endif /* FREERTOS_CONFIG_H */
