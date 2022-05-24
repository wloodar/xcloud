#include "server.h"
#include "../common/common.h"
#include <stdio.h>
#include <string.h>

int userlist_load(struct userlist *list, char *file)
{
    FILE *f = fopen(file, "r");

    if (!f)
        die("Couldnt open users list file");

    char line[256];
    while (fgets(line, 256, f)) {
        printf("%s\n", line);
        char *test = strchr(line, ':');
        puts(test);
    }

    fclose(f);
    return 0;
}

/* Return the index of the found user in the list, otherwise return -1. */
int userlist_find_by_id(struct userlist *list, xcp_userid id);
int userlist_find_by_name(struct userlist *list, char *name);

/* Free the allocated data in `list`. */
int userlist_free(struct userlist *list);

/* Save the userlist back to `file`. */
int userlist_save(struct userlist *list, char *file);
