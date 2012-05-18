#include "server/client.h"
#include "packet/packet.h"
#include "nbt/nbt.h"

int packet_login_request(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	int32_t version, i;
	char string[BEDROCK_USERNAME_MAX];
	int8_t b;

	packet_read_int(buffer, len, &offset, &version, sizeof(version));
	packet_read_string(buffer, len, &offset, string, sizeof(string)); /* Username is sent here too, why? */
	packet_read_string(buffer, len, &offset, string, sizeof(string));
	packet_read_int(buffer, len, &offset, &i, sizeof(i));
	packet_read_int(buffer, len, &offset, &i, sizeof(i));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));

	if (offset == ERROR_EAGAIN)
		return ERROR_EAGAIN;

	// Check version, should be 29
	if (version != BEDROCK_PROTOCOL_VERSION)
		;

	client_send_header(client, LOGIN_REQUEST);
	client_send_int(client, &entity_id, sizeof(entity_id)); /* Entity ID */
	++entity_id;
	client_send_string(client, "");
	client_send_string(client, nbt_read_string(client->world->data, 2, "Data", "generatorName")); /* Generator name */
	client_send_int(client, nbt_read(client->world->data, TAG_INT, 2, "Data", "GameType"), sizeof(uint32_t)); /* Game type */
	client_send_int(client, nbt_read(client->data, TAG_INT, 1, "Dimension"), sizeof(uint32_t)); /* Dimension */
	client_send_int(client, nbt_read(client->world->data, TAG_BYTE, 2, "Data", "hardcore"), sizeof(uint8_t)); /* hardcore */
	b = 0;
	client_send_int(client, &b, sizeof(b));
	b = 8;
	client_send_int(client, &b, sizeof(b)); /* Max players */

	client->authenticated = STATE_BURSTING;

	// Spawn Position (0x06)
	client_send_header(client, 0x06);
	client_send_int(client, nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnX"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnY"), sizeof(uint32_t)); // Y
	client_send_int(client, nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnZ"), sizeof(uint32_t)); // Z

	client_update_chunks(client);

	// Player Position & Look (0x0D)
	client_send_header(client, 0x0D);
	client_send_int(client, client_get_pos_x(client), sizeof(double)); // X
	double d = *client_get_pos_y(client);
	d += 2;
	client_send_int(client, &d, sizeof(d)); // Stance
	client_send_int(client, &d, sizeof(d)); // Y
	client_send_int(client, client_get_pos_z(client), sizeof(double)); // Z
	float f = 0;
	client_send_int(client, &f, sizeof(f)); // Yaw
	client_send_int(client, &f, sizeof(f)); // Pitch
	b = 1;
	client_send_int(client, &b, sizeof(b)); // On ground

	return offset;
}