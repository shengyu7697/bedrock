#include "server/bedrock.h"
#include "server/world.h"
#include "server/config.h"
#include "server/listener.h"
#include "io/io.h"

#include <time.h>
#include <errno.h>

bool bedrock_running = true;
struct timespec bedrock_time = { 0, 0 };
uint16_t bedrock_tick = 0;
static struct timespec last_tick;

void bedrock_update_time()
{
	uint64_t diff;
	uint16_t tick_diff;

	if (clock_gettime(CLOCK_MONOTONIC, &bedrock_time) == -1)
	{
		bedrock_log(LEVEL_WARN, "bedrock: Unable to update clock - %s", strerror(errno));
		return;
	}

	/* This is in nano seconds, which is 10^-9 */
	diff = (bedrock_time.tv_sec * 1000000000 + bedrock_time.tv_nsec) - (last_tick.tv_sec * 1000000000 + last_tick.tv_sec);
	/* Get milli seconds */
	diff /= 1000000;

	tick_diff = diff / BEDROCK_TICK_LENGTH;
	if (tick_diff > 0)
	{
		last_tick = bedrock_time;
		bedrock_tick += tick_diff;
		bedrock_tick %= BEDROCK_DAY_LENGTH;
	}
}

void bedrock_log(bedrock_log_level level, const char *msg, ...)
{
	va_list args;
	char buffer[512];

	va_start(args, msg);
	vsnprintf(buffer, sizeof(buffer), msg, args);
	va_end(args);

	if (level != LEVE_NBT_DEBUG)
		fprintf(stdout, "%s\n", buffer);
}

int main(int argc, char **argv)
{
	clock_gettime(CLOCK_MONOTONIC, &last_tick);

	bedrock_world *world = bedrock_world_create(BEDROCK_WORLD_NAME, BEDROCK_WORLD_BASE);
	if (bedrock_world_load(world) == false)
		exit(1);

	bedrock_io_init();
	init_listener();

	while (bedrock_running)
	{
		bedrock_io_process();
		bedrock_client_process_exits();
	}

	bedrock_io_shutdown();
	bedrock_world_free(world);

	return 0;
}
