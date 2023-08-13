#pragma once

#include "xcp.h"

void initialize();
char *get_username();
char *set_username();

xcp_packet_reply send_hello(int sock, char *username);

void list_active_users(int sock, char *username);
strlist get_active_users(int sock, char *username);

xcp_packet_reply send_message(int sock, char dest[256], char *message);

void show_commands_list(char *username);