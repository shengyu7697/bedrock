#include "server/bedrock.h"
#include "server/command.h"

void command_version(struct bedrock_client *client, int argc, const char **argv)
{
	command_reply(client, "Bedrock version %d.%d%s Compiled on %s %s.", BEDROCK_VERSION_MAJOR, BEDROCK_VERSION_MINOR, BEDROCK_VERSION_EXTRA, __DATE__, __TIME__);
}