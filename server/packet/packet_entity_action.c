#include "server/client.h"
#include "server/packet.h"
#include "server/packets.h"
#include "server/column.h"

enum
{
	METADATA_NONE = 0,
	METADATA_FIRE = 1 << 0,
	METADATA_CROUCHED = 1 << 1,
	METADATA_RIDING = 1 << 2,
	METADATA_SPRINTING = 1 << 3,
	METADATA_EATING = 1 << 4
};

int packet_entity_action(struct client *client, bedrock_packet *p)
{
	int32_t eid;
	int8_t aid;
	int32_t jump_boost;

	packet_read_varint(p, &eid);
	packet_read_byte(p, &aid);
	packet_read_varint(p, &jump_boost);

	if (p->error)
		return p->error;
	if (eid != client->id)
		return ERROR_UNEXPECTED;

	client->action = aid;

	switch (client->action)
	{
		case ACTION_CROUCH:
		{
			uint8_t b = METADATA_CROUCHED;
			packet_send_entity_metadata(client, ENTITY_METADATA_INDEX_FLAGS, ENTITY_METADATA_TYPE_BYTE, &b, sizeof(b));
			break;
		}
		case ACTION_UNCROUCH:
		case ACTION_STOP_SPRINTING:
		{
			uint8_t b = METADATA_NONE;
			packet_send_entity_metadata(client, ENTITY_METADATA_INDEX_FLAGS, ENTITY_METADATA_TYPE_BYTE, &b, sizeof(b));
			break;
		}
		case ACTION_START_SPRINTING:
		{
			uint8_t b = METADATA_SPRINTING;
			packet_send_entity_metadata(client, ENTITY_METADATA_INDEX_FLAGS, ENTITY_METADATA_TYPE_BYTE, &b, sizeof(b));
			break;
		}
		case ACTION_LEAVE_BED:
		default:
			client->action = ACTION_NONE;
	}

	return ERROR_OK;
}
