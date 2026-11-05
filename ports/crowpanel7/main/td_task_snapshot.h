/* CrowPanel task statistics without the long interrupt-masked stack scan.
 * The implementation is compiled at FreeRTOS's task-additions extension point. */
#ifndef TD_TASK_SNAPSHOT_H
#define TD_TASK_SNAPSHOT_H
#ifdef TD_TASK_SNAPSHOT_TEST
#include "task_snapshot_platform.h"
#else
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

typedef struct
{
    TaskStatus_t task;
    char name[configMAX_TASK_NAME_LEN];
    BaseType_t core;
} td_task_snapshot_t;

/* Names and core affinity are copied under the kernel lock as well: no
 * dereferencing a task handle or borrowed task-name pointer after return. */
UBaseType_t td_task_snapshot_fast(td_task_snapshot_t *out, UBaseType_t capacity,
                                  configRUN_TIME_COUNTER_TYPE *total);
#endif
