/* Test the actual kernel extension against task churn and bounded output. */
#define TD_TASK_SNAPSHOT_TEST
#include "../ports/crowpanel7/main/td_task_snapshot.inc"
#include "td_test.h"

int main(void)
{
    td_task_snapshot_t out[4];
    memset(out, 0xa5, sizeof(out));
    uint32_t total = 0;
    CHECK_EQ(td_task_snapshot_fast(NULL, 4, &total), 0);
    CHECK_EQ(td_task_snapshot_fast(out, 0, &total), 0);
    CHECK_EQ(critical_depth, 0);
    fake_count = 4;
    for (unsigned i = 0; i < fake_count; i++)
    {
        fake_tasks[i] = (struct fake_task){.number = i + 1, .priority = i + 2, .run = 100 + i, .core = (int)i % 2, .state = (eTaskState)i};
        snprintf(fake_tasks[i].name, sizeof(fake_tasks[i].name), "task-%u", i);
    }
    snprintf(fake_tasks[1].name, sizeof(fake_tasks[1].name), "a-long-task-name-that-is-clipped");
    CHECK_EQ(td_task_snapshot_fast(out, 2, &total), 2);
    CHECK_EQ(info_calls, 2);
    CHECK_EQ(critical_depth, 0);
    CHECK_EQ(total, 12345);
    CHECK_EQ(out[2].name[0], (char)0xa5); /* capacity respected */
    CHECK_EQ(strlen(out[1].name), configMAX_TASK_NAME_LEN - 1);
    CHECK_EQ(td_task_snapshot_fast(out, 4, &total), 4);
    for (unsigned i = 0; i < 4; i++)
    {
        CHECK(out[i].task.pcTaskName == out[i].name);
        CHECK_EQ(out[i].task.xTaskNumber, i + 1);
        CHECK_EQ(out[i].task.ulRunTimeCounter, 100 + i);
        CHECK_EQ(out[i].core, (int)i % 2);
        CHECK_EQ(out[i].task.eCurrentState, (eTaskState)i);
    }
    /* Task names remain valid after a task disappears/reuses its storage. */
    memset(fake_tasks, 0, sizeof(fake_tasks));
    fake_count = 0;
    CHECK(!strcmp(out[0].task.pcTaskName, "task-0"));
    CHECK_EQ(td_task_snapshot_fast(out, 4, NULL), 0);
    CHECK_EQ(critical_depth, 0);
    return TD_TEST_RESULT();
}
