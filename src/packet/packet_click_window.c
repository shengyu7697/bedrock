#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_set_slot.h" // XXX for WINDOW_
#include "packet/packet_confirm_transaction.h"
#include "nbt/nbt.h"

#include <math.h>

int packet_click_window(struct bedrock_client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint8_t window;
	uint16_t slot;
	uint8_t right_click;
	uint16_t action;
	uint8_t shift;
	int16_t item_id;
	uint8_t item_count = 0;
	int16_t item_metadata = 0;

	packet_read_int(p, &offset, &window, sizeof(window));
	packet_read_int(p, &offset, &slot, sizeof(slot));
	packet_read_int(p, &offset, &right_click, sizeof(right_click));
	packet_read_int(p, &offset, &action, sizeof(action));
	packet_read_int(p, &offset, &shift, sizeof(shift));
	packet_read_int(p, &offset, &item_id, sizeof(item_id));
	if (item_id != -1)
	{
		packet_read_int(p, &offset, &item_count, sizeof(item_count));
		packet_read_int(p, &offset, &item_metadata, sizeof(item_metadata));

		// XXX more here?
	}

	if (window == WINDOW_INVENTORY)
	{
		if (slot < 9 || slot > 44)
			return ERROR_NOT_ALLOWED;

		if (slot >= 36)
			slot -= 36;

		nbt_tag *tag;

		tag = client_get_inventory_tag(client, slot);

		if (tag != NULL)
		{
			uint16_t id;
			//uint8_t count;
			int16_t metadata;
			//int16_t *id = nbt_get(tag, TAG_SHORT, 1, "id");
			uint8_t *count = nbt_read(tag, TAG_BYTE, 1, "Count");
			//int16_t *metadata = nbt_get(tag, TAG_SHORT, 1, "Damage");
			bool replaced = false;

			nbt_copy(tag, TAG_SHORT, &id, sizeof(id), 1, "id");
			//nbt_copy(tag, TAG_BYTE, &count, sizeof(count), 1, "Count");
			nbt_copy(tag, TAG_SHORT, &metadata, sizeof(metadata), 1, "Damage");

			bedrock_log(LEVEL_DEBUG, "click window: %s clicks on slot %d which contains %s", client->name, slot, item_find_or_create(id)->name);

			// If I am already dragging an item replace it with this slot completely, even if I right clicked this slot.
			if (client->window_drag_data.id)
			{
				// However if I am right clicking and this slot is of the same type as my drag type, move one item
				if (right_click && id == client->window_drag_data.id && *count < BEDROCK_MAX_ITEMS_PER_STACK)
				{
					++*count;
					--client->window_drag_data.count;

					bedrock_log(LEVEL_DEBUG, "click window: %s moves one item from stack of %s to %d", client->name, client->window_drag_data.count, item_find_or_create(client->window_drag_data.id)->name, slot);

					// Might have been the last item, zero out drag state
					if (client->window_drag_data.count == 0)
					{
						client->window_drag_data.id = 0;
						client->window_drag_data.metadata = 0;
					}
				}
				// Replacing a slot
				else
				{
					// Replace this slot with drag data
					nbt_set(tag, TAG_SHORT, &client->window_drag_data.id, sizeof(client->window_drag_data.id), 1, "id");
					nbt_set(tag, TAG_BYTE, &client->window_drag_data.count, sizeof(client->window_drag_data.count), 1, "Count");
					nbt_set(tag, TAG_SHORT, &client->window_drag_data.metadata, sizeof(client->window_drag_data.metadata), 1, "Damage");

					bedrock_log(LEVEL_DEBUG, "click window: %s replaces slot %d with %d blocks of %s", client->name, slot, client->window_drag_data.count, item_find_or_create(client->window_drag_data.id)->name);
				}

				replaced = true;
			}

			/* This item is now being dragged and is no longer in its slot */
			client->window_drag_data.id = id;
			client->window_drag_data.metadata = metadata;

			if (right_click && replaced == false)
			{
				/* We only want half of the blocks here. If there is an odd count then they are holding the larger. */
				double dcount = (double) *count / 2;
				bool even = floor(dcount) == dcount;

				// If this isn't an even outcome give the client one more
				if (even == false)
					++dcount;

				client->window_drag_data.count = dcount;
				*count /= 2;

				// Slot can be empty (right clicking a 1 item slot)
				if (*count == 0)
					nbt_free(tag);
			}
			else
			{
				client->window_drag_data.count = *count;

				/* Slot goes away */
				if (replaced == false)
					nbt_free(tag);
			}
		}
		else
		{
			// Clicked an empty slot, might be placing blocks there
			if (client->window_drag_data.id)
			{
				bedrock_node *node;
				// Create the new inventory entry
				nbt_tag *tag = bedrock_malloc(sizeof(nbt_tag));
				tag->owner = nbt_get(client->data, TAG_LIST, 1, "Inventory");
				tag->type = TAG_COMPOUND;

				// On a right click we only transfer one item, otherwise take them all
				nbt_add(tag, TAG_SHORT, "id", &client->window_drag_data.id, sizeof(client->window_drag_data.id));
				nbt_add(tag, TAG_SHORT, "Damage", &client->window_drag_data.metadata, sizeof(client->window_drag_data.metadata));
				if (right_click)
				{
					uint8_t count = 1;
					nbt_add(tag, TAG_BYTE, "Count", &count, sizeof(count));

					--client->window_drag_data.count;

					bedrock_log(LEVEL_DEBUG, "click window: %s right click places 1 block of %s in empty slot %d", client->name, item_find_or_create(client->window_drag_data.id)->name, slot);
				}
				else
				{
					nbt_add(tag, TAG_BYTE, "Count", &client->window_drag_data.count, sizeof(client->window_drag_data.count));
					bedrock_log(LEVEL_DEBUG, "click window: %s places %d blocks of %s in empty slot %d", client->name, client->window_drag_data.count, item_find_or_create(client->window_drag_data.id)->name, slot);
				}
				nbt_add(tag, TAG_BYTE, "Slot", &slot, sizeof(slot));

				// Zero out drag state if this wasn't a right click replace or if there are no items left
				if (right_click == false || client->window_drag_data.count == 0)
				{
					client->window_drag_data.id = 0;
					client->window_drag_data.count = 0;
					client->window_drag_data.metadata = 0;
				}

				// Insert slot
				LIST_FOREACH(&tag->owner->payload.tag_list.list, node)
				{
					nbt_tag *c = node->data;
					uint8_t *cslot = nbt_read(c, TAG_BYTE, 1, "Slot");

					if (*cslot > slot)
					{
						// Insert before c
						bedrock_list_add_node_before(&tag->owner->payload.tag_list.list, bedrock_malloc(sizeof(bedrock_node)), node, tag);
						packet_send_confirm_transaction(client, window, action, true);
						return offset;
					}
				}

				// Insert at the end
				bedrock_list_add(&tag->owner->payload.tag_list.list, tag);
			}
		}

		packet_send_confirm_transaction(client, window, action, true);
	}

	return offset;
}
