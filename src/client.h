#pragma once

#include "xcp.h"

void initialize();
void listening_thread(char *host, char *username);
void *listening(void *host);
char *get_username();
char *set_username();

xcp_packet_reply send_hello(int sock, char *username, xcp_packet_type type);

void list_active_users(int sock, char *username);
strlist get_active_users(int sock, char *username);

xcp_packet_reply send_message(int sock, char dest[256], char *message);

void show_commands_list(char *username);

static pthread_mutex_t *get_console_lock();