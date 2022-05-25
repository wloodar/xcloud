#pragma once

#include <stdint.h>
#include "xcp.h"


struct user
{
	xcp_userid id;
	char name[];
};

struct userlist
{
	int size;
	struct user **users;
};


void send_package(int client_sock, int type, void *buf, int size);

/* Load the userlist from `file`. */
int userlist_load(struct userlist *list, char *file);

/* Return the index of the found user in the list, otherwise return -1. */
int userlist_find_by_id(struct userlist *list, xcp_userid id);
int userlist_find_by_name(struct userlist *list, char *name);

/* Free the allocated data in `list`. */
int userlist_free(struct userlist *list);

/* Add the following user to the userlist. If a user with the given name or ID
   already exists, returns 1 for an existing name, and 2 for an existing ID. */
int userlist_add(struct userlist *list, xcp_userid id, char *name);

/* Remove the given user from the list. If a user with the given ID cannot be
   found, 1 is returned for failure. */
int userlist_remove(struct userlist *list, xcp_userid id);

/* Save the userlist back to `file`. */
int userlist_save(struct userlist *list, char *file);
