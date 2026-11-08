#ifndef TASK_SNAPSHOT_TEST_PLATFORM_H
#define TASK_SNAPSHOT_TEST_PLATFORM_H
#include <stdint.h>
#include <string.h>
#undef NDEBUG /* the checks are asserts: keep them in Release builds */
#include <assert.h>
typedef unsigned UBaseType_t;
typedef int BaseType_t;
typedef uint32_t configRUN_TIME_COUNTER_TYPE;
typedef struct fake_task *TaskHandle_t;
typedef enum
{
    eRunning,
    eReady,
    eBlocked,
    eSuspended,
    eDeleted,
    eInvalid
} eTaskState;
typedef struct
{
    TaskHandle_t xHandle;
    const char *pcTaskName;
    UBaseType_t xTaskNumber, uxCurrentPriority, usStackHighWaterMark;
    eTaskState eCurrentState;
    configRUN_TIME_COUNTER_TYPE ulRunTimeCounter;
} TaskStatus_t;
typedef struct
{
    unsigned next;
    TaskHandle_t pxTaskHandle;
} TaskIterator_t;
#define configMAX_TASK_NAME_LEN 16
#define pdFALSE                 0
static int xKernelLock, critical_depth, info_calls;
struct fake_task
{
    char name[32];
    unsigned number, priority;
    uint32_t run;
    int core;
    eTaskState state;
};
static struct fake_task fake_tasks[4];
static unsigned fake_count;
#define prvENTER_CRITICAL_OR_SUSPEND_ALL(lock) \
    do                                         \
    {                                          \
        (void)(lock);                          \
        assert(!critical_depth);               \
        critical_depth++;                      \
    } while (0)
#define prvEXIT_CRITICAL_OR_RESUME_ALL(lock) ((void)(lock), critical_depth--, 0)
static int xTaskGetNext(TaskIterator_t *it)
{
    assert(critical_depth == 1);
    if (it->next >= fake_count)
        return -1;
    it->pxTaskHandle = &fake_tasks[it->next++];
    return 0;
}
static void vTaskGetInfo(TaskHandle_t handle, TaskStatus_t *out, BaseType_t scan, eTaskState state)
{
    assert(critical_depth == 1);
    assert(scan == pdFALSE);
    assert(state == eInvalid);
    info_calls++;
    *out = (TaskStatus_t){.xHandle = handle, .pcTaskName = handle->name, .xTaskNumber = handle->number, .uxCurrentPriority = handle->priority, .eCurrentState = handle->state, .ulRunTimeCounter = handle->run};
}
static BaseType_t xTaskGetCoreID(TaskHandle_t handle)
{
    assert(critical_depth == 1);
    return handle->core;
}
static uint32_t fake_counter(void)
{
    assert(critical_depth == 1);
    return 12345;
}
#define portGET_RUN_TIME_COUNTER_VALUE() fake_counter()
#endif
