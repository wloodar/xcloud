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

		printf("%s\n", line);
		username = strchr(line, ':');
		*(username++) = 0;

		userid = string_to_userid(line);
		nusername = strlen(username);

		printf("username[%d]='%s'\n", nusername, username);

		list->users[list->size-1] = malloc(sizeof(struct user) + nusername + 1);
		list->users[list->size-1]->id = userid;
		list->users[list->size-1]->name[nusername] = 0;
		memcpy(list->users[list->size-1]->name, username, nusername);

		printf("loaded %zx as %s\n", list->users[list->size-1]->id,
				list->users[list->size-1]->name);
	}

	fclose(f);
	return 0;
}

int userlist_find_by_id(struct userlist *list, xcp_userid id)
{
	for (int i = 0; i < list->size; i++) {
		if (id == list->users[i]->id)
			return i;
	}

	return -1;
}

int userlist_find_by_name(struct userlist *list, char *name)
{
	for (int i = 0; i < list->size; i++) {
		if (!strcmp(name, list->users[i]->name))
			return i;
	}

	return -1;
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

	for (int i = 0; i < list->size; i++)
		fprintf(f, "%016zx:%s\n", list->users[i]->id, list->users[i]->name);

	fclose(f);
	return 0;
}
