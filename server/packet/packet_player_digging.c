#include "server/client.h"
#include "server/packet.h"
#include "server/packets.h"
#include "server/column.h"
#include "blocks/blocks.h"
#include "nbt/nbt.h"
#include "windows/window.h"
#include "util/math.h"

static struct item *get_weilded_item(struct client *client)
{
	struct item_stack *weilded_item = &client->inventory[INVENTORY_HOTBAR_START + client->selected_slot];
	if (weilded_item->count)
		return item_find_or_create(weilded_item->id);
	return item_find_or_create(ITEM_NONE);
}

/* Is the given item a weakness for the given block? */
static bool is_weakness(struct block *block, struct item *item)
{
	bool has_tool_requirement, has_type_requirement;

	bedrock_assert((block->weakness & ~(TOOL_NAME_MASK | TOOL_TYPE_MASK)) == 0, ;);

	// Extract tool name from block requirement, if one exists, and see if the player has a tool of that type. if there is no tool requirement anything goes.
	has_tool_requirement = block->weakness & TOOL_NAME_MASK ? block->weakness & item->flags & TOOL_NAME_MASK : true;
	// And the same for tool type requirement
	has_type_requirement = block->weakness & TOOL_TYPE_MASK ? block->weakness & item->flags & TOOL_TYPE_MASK : true;

	return has_tool_requirement && has_type_requirement;
}

/* Can the given item harvest the given block? */
static bool can_harvest(struct block *block, struct item *item)
{
	bool has_tool_requirement, has_type_requirement;

	bedrock_assert((block->harvest & ~(TOOL_NAME_MASK | TOOL_TYPE_MASK)) == 0, ;);

	// Extract tool name from block requirement, if one exists, and see if the player has a tool of that type. if there is no tool requirement anything goes.
	has_tool_requirement = block->harvest & TOOL_NAME_MASK ? block->harvest & item->flags & TOOL_NAME_MASK : true;
	// And the same for tool type requirement
	has_type_requirement = block->harvest & TOOL_TYPE_MASK ? block->harvest & item->flags & TOOL_TYPE_MASK : true;

	return has_tool_requirement && has_type_requirement;
}

/* Calculate how long a block should take to mine using the given item.
 * See: http://www.minecraftwiki.net/wiki/Digging
 */
static double calculate_block_time(struct client bedrock_attribute_unused *client, struct block *block, struct item *item)
{
	// Start with the time, in seconds, it takes to mine the block for no harvest
	double delay = block->no_harvest_time;

	// If this block can be harvested by this item
	if (is_weakness(block, item))
	{
		// Set the delay to the hardness
		delay = block->hardness;

		// If the block has a weakness
		if (block->weakness != ITEM_FLAG_NONE)
		{
			bedrock_assert((block->weakness & ~(TOOL_NAME_MASK | TOOL_TYPE_MASK)) == 0, ;);

			// If our item matches one of the weaknesses
			if (block->weakness & item->flags & TOOL_NAME_MASK)
			{
				// Reduce delay accordingly
				if (item->flags & ITEM_FLAG_GOLD)
					delay /= 12;
				else if (item->flags & ITEM_FLAG_DIAMOND)
					delay /= 8;
				else if (item->flags & ITEM_FLAG_IRON)
					delay /= 6;
				else if (item->flags & ITEM_FLAG_STONE)
					delay /= 4;
				else if (item->flags & ITEM_FLAG_WOOD)
					delay /= 2;
			}
		}
	}

	return delay;
}

int packet_player_digging(struct client *client, bedrock_packet *p)
{
	int8_t status;
	struct position pos;
	int8_t face;

	packet_read_byte(p, &status);
	packet_read_position(p, &pos);
	packet_read_byte(p, &face);

	if (p->error)
		return p->error;

	if (status == START_DIGGING)
	{
		struct chunk *chunk = find_chunk_which_contains(client->world, pos.x, pos.y, pos.z);
		uint8_t *block_id;
		struct block *block;
		double delay;
		struct item *item = get_weilded_item(client);

		if (abs(client->x - pos.x) > 6 || abs(client->y - pos.y) > 6 || abs(client->z - pos.z) > 6)
			return ERROR_NOT_ALLOWED;

		// Reset dig state
		memset(&client->digging_data, 0, sizeof(client->digging_data));

		if (chunk == NULL)
			return ERROR_NOT_ALLOWED;

		block_id = chunk_get_block(chunk, pos.x, pos.y, pos.z);
		if (block_id == NULL)
			return ERROR_NOT_ALLOWED;

		block = block_find_or_create(*block_id);

		bedrock_log(LEVEL_DEBUG, "player digging: %s is digging coords %d,%d,%d which is of type %s", client->name, pos.x, pos.y, pos.z, block->item.name);

		delay = calculate_block_time(client, block, item);

		// Special case, unmineable
		if (delay < 0)
			return ERROR_OK;

		if (client->gamemode == GAMEMODE_CREATIVE)
		{
			status = STOP_DIGGING;
		}
		else
		{
			client->digging_data.x = pos.x;
			client->digging_data.y = pos.y;
			client->digging_data.z = pos.z;
			client->digging_data.block_id = block->item.id;
			client->digging_data.item_id = item->id;
			client->digging_data.end = bedrock_time + (delay * 1000.0);
		}
	}
	if (status == STOP_DIGGING)
	{
		struct item *item = get_weilded_item(client);
		struct chunk *chunk;
		uint8_t *block_id;
		struct block *block;
		int32_t *height;
		bedrock_node *node;
		int i;

		if (abs(client->x - pos.x) > 6 || abs(client->y - pos.y) > 6 || abs(client->z - pos.z) > 6)
			return ERROR_NOT_ALLOWED;

		chunk = find_chunk_which_contains(client->world, pos.x, pos.y, pos.z);
		if (chunk == NULL)
			return ERROR_NOT_ALLOWED;

		block_id = chunk_get_block(chunk, pos.x, pos.y, pos.z);
		if (block_id == NULL)
			return ERROR_NOT_ALLOWED;

		if (client->gamemode == GAMEMODE_CREATIVE)
		{
			bedrock_log(LEVEL_DEBUG, "player digging: Instant mine for %s at %d,%d,%d", client->name, pos.x, pos.y, pos.z);
		}
		else
		{
			if (pos.x != client->digging_data.x || pos.y != client->digging_data.y || pos.z != client->digging_data.z || client->digging_data.end == 0 || client->digging_data.item_id != item->id || client->digging_data.block_id != *block_id)
			{
				bedrock_log(LEVEL_DEBUG, "player digging: Mismatch in dig data - saved: X: %d Y: %d Z: %d T: %lu I: %d B: %d - got: X: %d Y: %d Z: %d I: %d B: %d",
						client->digging_data.x, client->digging_data.y, client->digging_data.z, client->digging_data.end, client->digging_data.item_id, client->digging_data.block_id,
						pos.x, pos.y, pos.z, item->id, *block_id);

				packet_send_block_change(client, pos.x, pos.y, pos.z, *block_id, 0);
				return ERROR_OK;
			}

			if (bedrock_time < client->digging_data.end)
			{
				bedrock_log(LEVEL_DEBUG, "player digging: %s is digging too fast! Expected to end at %ld, but now it is %ld", client->name, client->digging_data.end, bedrock_time);
				packet_send_block_change(client, pos.x, pos.y, pos.z, *block_id, 0);
				return ERROR_OK;
			}
		}

		block = block_find_or_create(*block_id);
		*block_id = BLOCK_AIR;
		column_set_pending(chunk->column, COLUMN_FLAG_DIRTY);

		// This is the height right *above* the highest block
		height = column_get_height_for(chunk->column, pos.x, pos.z);
		if (pos.y == *height - 1)
		{
			uint8_t *height_block;

			do
				height_block = column_get_block(chunk->column, pos.x, --(*height) - 1, pos.z);
			while (*height && (height_block == NULL || *height_block == BLOCK_AIR));

			bedrock_log(LEVEL_DEBUG, "player digging: Adjusting height map of %d,%d to %d", pos.x, pos.z, *height);
		}

		// Notify players in render distance of the column to remove the block
		LIST_FOREACH(&chunk->column->players, node)
		{
			struct client *c = node->data;

			packet_send_block_change(c, pos.x, pos.y, pos.z, BLOCK_AIR, 0);
		}

		if (block->on_mine != NULL)
			block->on_mine(client, chunk, pos.x, pos.y, pos.z, block, can_harvest(block, item));

		// If the chunk is all air delete it;
		for (i = 0; i < BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK; ++i)
			if (chunk->blocks[i] != BLOCK_AIR)
				break;
		if (i == BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK)
			chunk_free(chunk);
	}
	else if (status == DROP_ITEM)
	{
		/* Dropping currently held item */
		struct item_stack *weilded_item = &client->inventory[INVENTORY_HOTBAR_START + client->selected_slot];
		if (weilded_item->count)
		{
			struct dropped_item *di = bedrock_malloc(sizeof(struct dropped_item));
			struct column *column;

			di->item = item_find_or_create(weilded_item->id);
			di->count = 1;
			di->data = weilded_item->metadata;

			di->p.id = ++entity_id;

			math_unit_vector(client->yaw, client->pitch, &di->p.velocity.x, &di->p.velocity.y, &di->p.velocity.z);

			physics_item_initialize(di, client->x + di->p.velocity.x, client->y + BEDROCK_PLAYER_HEIGHT + di->p.velocity.y, client->z + di->p.velocity.z);

			/* Now that we have the starting positiion for this item, find the column it is in */
			column = find_column_from_world_which_contains(client->world, di->p.pos.x, di->p.pos.z);
			if (column == NULL)
			{
				/* Throwing item off of the world */
				bedrock_free(di);
			}
			else
			{
				di->p.column = column;

				column_add_item(column, di);
				physics_add(column, &di->p);
			}

			bedrock_log(LEVEL_DEBUG, "player digging: %s drops a block of %s", client->name, item_find_or_create(weilded_item->id)->name);

			--weilded_item->count;

			if (weilded_item->count)
			{
				packet_send_set_slot(client, WINDOW_INVENTORY, INVENTORY_HOTBAR_START + client->selected_slot, item_find_or_create(weilded_item->id), weilded_item->count, weilded_item->metadata);
			}
			else
			{
				/* Item goes away */
				packet_send_set_slot(client, WINDOW_INVENTORY, INVENTORY_HOTBAR_START + client->selected_slot, NULL, 0, 0);
			}
		}
	}
	else
		bedrock_log(LEVEL_DEBUG, "player digging: Unrecognized dig status %d", status);

	return ERROR_OK;
}
