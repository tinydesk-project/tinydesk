/* Included after ESP-IDF's task additions, inside the kernel translation unit.
 * Keep the normal SDK APIs intact. Only TinyDesk's live RGB task table uses
 * this bounded copy; vTaskGetInfo(..., pdFALSE, ...) never scans task stacks. */
#include "td_task_snapshot.h"

UBaseType_t td_task_snapshot_fast(td_task_snapshot_t *out, UBaseType_t capacity,
                                configRUN_TIME_COUNTER_TYPE *total)
{
    UBaseType_t count = 0;
    if (!out || !capacity) return 0;
    prvENTER_CRITICAL_OR_SUSPEND_ALL(&xKernelLock);
    TaskIterator_t iterator = {0};
    while (count < capacity && xTaskGetNext(&iterator) != -1) {
        if (!iterator.pxTaskHandle) continue;
        td_task_snapshot_t *entry = &out[count++];
        vTaskGetInfo(iterator.pxTaskHandle, &entry->task, pdFALSE, eInvalid);
        const char *name = entry->task.pcTaskName;
        unsigned i = 0;
        while (i + 1 < sizeof(entry->name) && name && name[i]) {
            entry->name[i] = name[i];
            i++;
        }
        entry->name[i] = 0;
        entry->task.pcTaskName = entry->name;
        entry->core = xTaskGetCoreID(iterator.pxTaskHandle);
    }
    if (total) *total = portGET_RUN_TIME_COUNTER_VALUE();
    (void)prvEXIT_CRITICAL_OR_RESUME_ALL(&xKernelLock);
    return count;
}
