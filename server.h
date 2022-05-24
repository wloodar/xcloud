#pragma once

#include <stdint.h>
#include "xcp.h"

void send_package(int client_sock, int type, void *buf, int size);
