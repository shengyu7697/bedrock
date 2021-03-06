#include "util/util.h"
#include "util/list.h"
#include "console/socket.h"
#include "console/ncurses.h"

bool console_running = true;

int main()
{
	if (!socket_init())
		return -1;

	ncurses_init();

	ncurses_print("Welcome to the bedrock console.\n");
	ncurses_print("To exit type \"quit\"\n");

	while (console_running)
		socket_process();

	ncurses_shutdown();
	socket_shutdown();

	return 0;
}

