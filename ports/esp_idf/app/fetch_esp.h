/* fetch_esp.h - downloads on the ESP-IDF boards (td_fetch_ops_t). */
#pragma once

#include "tinydesk/td_sysinfo.h"

/* The download operations, or NULL without memory. */
const td_fetch_ops_t *fetch_esp_ops(void);
