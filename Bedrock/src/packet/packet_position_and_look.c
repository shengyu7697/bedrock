#include "server/client.h"
#include "packet/packet.h"

int packet_position_and_look(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	double x, y, stance, z;
	float yaw, pitch;
	uint8_t on_ground;

	client->authenticated = STATE_AUTHENTICATED;

	packet_read_int(buffer, len, &offset, &x, sizeof(x));
	packet_read_int(buffer, len, &offset, &y, sizeof(y));
	packet_read_int(buffer, len, &offset, &stance, sizeof(stance));
	packet_read_int(buffer, len, &offset, &z, sizeof(z));
	packet_read_int(buffer, len, &offset, &yaw, sizeof(yaw));
	packet_read_int(buffer, len, &offset, &pitch, sizeof(pitch));
	packet_read_int(buffer, len, &offset, &on_ground, sizeof(on_ground));

	return offset;
}

