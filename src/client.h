void initialize();
char *get_username();
char *set_username();

xcp_packet_reply send_hello(int sock, char *username);
strlist get_active_users(int sock);