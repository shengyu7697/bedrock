#include "util/list.h"
#include "nbt/tag.h"

#include <limits.h>

typedef struct
{
	char name[128];
	char path[PATH_MAX];
	nbt_tag *data;
	bedrock_list regions;
} bedrock_world;

extern bedrock_list world_list;

extern bedrock_world *bedrock_world_create(const char *name, const char *path);
extern bool bedrock_world_load(bedrock_world *world);
extern void bedrock_world_free(bedrock_world *world);
