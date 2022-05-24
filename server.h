#pragma once

void die(const char *fmt, ...);

void send_package(int client_sock, int type, void *buf, int size);
