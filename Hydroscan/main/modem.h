
#ifndef MODEM_H
#define MODEM_H

#include <stdbool.h>
#include "esp_err.h"

esp_err_t modem_init(void);

bool modem_is_ready(void);

int modem_read_response(char *buffer,
                        size_t max_len,
                        uint32_t timeout_ms);

esp_err_t modem_send_at(const char *cmd);

#endif