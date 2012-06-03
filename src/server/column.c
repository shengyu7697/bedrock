#include "server/column.h"
#include "nbt/nbt.h"
#include "util/memory.h"
#include "compression/compression.h"

#define DATA_CHUNK_SIZE 2048

bedrock_memory_pool column_pool = BEDROCK_MEMORY_POOL_INIT;

struct bedrock_column *column_create(struct bedrock_region *region, nbt_tag *data)
{
	struct bedrock_column *column = bedrock_malloc_pool(&column_pool, sizeof(struct bedrock_column));
	nbt_tag *tag;
	bedrock_node *node;
	struct nbt_tag_byte_array *byte_array;

	column->region = region;
	nbt_copy(data, TAG_INT, &column->x, sizeof(column->x), 2, "Level", "xPos");
	nbt_copy(data, TAG_INT, &column->z, sizeof(column->z), 2, "Level", "zPos");
	column->data = data;

	tag = nbt_get(data, TAG_LIST, 2, "Level", "Sections");
	LIST_FOREACH(&tag->payload.tag_list, node)
	{
		nbt_tag *chunk_tag = node->data;
		uint8_t y;

		nbt_copy(chunk_tag, TAG_BYTE, &y, sizeof(y), 1, "Y");
		bedrock_assert(y < sizeof(column->chunks) / sizeof(struct bedrock_chunk *), continue);

		bedrock_assert(column->chunks[y] == NULL, continue);

		column->chunks[y] = chunk_create(column, y, chunk_tag);
	}
	nbt_free(tag);

	{
		compression_buffer *buffer = compression_compress_init(&column_pool, DATA_CHUNK_SIZE);

		tag = nbt_get(data, TAG_BYTE_ARRAY, 2, "Level", "Biomes");
		byte_array = &tag->payload.tag_byte_array;
		bedrock_assert(byte_array->length == BEDROCK_BIOME_LENGTH, ;);

		compression_compress_deflate_finish(buffer, (const unsigned char *) byte_array->data, byte_array->length);
		bedrock_buffer_resize(buffer->buffer, buffer->buffer->length);

		column->biomes = buffer->buffer;
		buffer->buffer = NULL;

		nbt_free(tag);
		compression_compress_end(buffer);
	}

	return column;
}

void column_free(struct bedrock_column *column)
{
	int i;

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		chunk_free(column->chunks[i]);

	bedrock_buffer_free(column->biomes);

	nbt_free(column->data);
	bedrock_free_pool(&column_pool, column);
}
