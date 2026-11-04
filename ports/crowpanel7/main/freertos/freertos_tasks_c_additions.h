/* FreeRTOS's supported task-additions hook. Preserve all ESP-IDF additions,
 * then add the LCD-safe sampler while the task lists/kernel lock are visible.
 * No SDK files are modified; this include path applies only to CrowPanel. */
#include_next "freertos_tasks_c_additions.h"
#include "../td_task_snapshot.inc"
