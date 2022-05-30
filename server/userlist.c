/* struct userlist functions
   Copyright (c) 2022 wloodar, bellrise */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "server.h"
#include "../common/common.h"


int userlist_load(struct userlist *list, char *file)
{
	xcp_userid userid;
	char *username;
	int nusername;
	char line[256];
	FILE *f;

	if (!(f = fopen(file, "r")))
		die("Could not open userlist file");

	list->size = 0;
	list->users = NULL;

	while (fgets(line, 256, f)) {
		list->users = realloc(list->users, sizeof(struct user*)
				* (++list->size));

		rstrip(line);

		username = strchr(line, ':');
		*(username++) = 0;

		userid = string_to_userid(line);
		nusername = strlen(username);

		list->users[list->size-1] = malloc(sizeof(struct user) + nusername + 1);
		list->users[list->size-1]->id = userid;
		list->users[list->size-1]->name[nusername] = 0;
		memcpy(list->users[list->size-1]->name, username, nusername);
	}

	fclose(f);
	return 0;
}

int userlist_find_by_id(struct userlist *list, xcp_userid id)
{
	for (int i = 0; i < list->size; i++) {
		if (!list->users[i])
			continue;
		if (!memcmp(&id, &list->users[i]->id, 8))
			return i;
	}

	return -1;
}

int userlist_find_by_name(struct userlist *list, char *name)
{
	for (int i = 0; i < list->size; i++) {
		if (!list->users[i])
			continue;
		if (!strcmp(name, list->users[i]->name))
			return i;
	}

	return -1;
}

int userlist_add(struct userlist *list, xcp_userid id, char *name)
{
	if (userlist_find_by_name(list, name) != -1)
		return 1;
	if (userlist_find_by_id(list, id) != -1)
		return 2;

	int namelen;

	namelen = strlen(name);

	/* Enter data for the new record. */
	list->users = realloc(list->users, sizeof(struct user *) * (++list->size));
	list->users[list->size-1] = malloc(sizeof(struct user) + namelen + 1);
	list->users[list->size-1]->id = id;
	memcpy(list->users[list->size-1]->name, name, namelen);
	list->users[list->size-1]->name[namelen] = 0;

	return 0;
}

int userlist_remove(struct userlist *list, xcp_userid id)
{
	int i;

	if ((i = userlist_find_by_id(list, id)) == -1)
		return 1;

	free(list->users[i]);
	list->users[i] = NULL;

	return 0;
}

int userlist_free(struct userlist *list)
{
	for (int i = 0; i < list->size; i++)
		free(list->users[i]);
	free(list->users);

	list->users = NULL;
	list->size = 0;

	return 0;
}

int userlist_save(struct userlist *list, char *file)
{
	FILE *f;

	if (!(f = fopen(file, "wb")))
		die("Failed to open userlist file for writing");

	for (int i = 0; i < list->size; i++) {
		char id[17];
		userid_to_string(id, list->users[i]->id);
		fprintf(f, "%s:%s\n", id, list->users[i]->name);
	}

	fclose(f);
	return 0;
}
