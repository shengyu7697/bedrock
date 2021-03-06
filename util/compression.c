#include "util/util.h"
#include "util/compression.h"
#include "util/memory.h"

compression_buffer *compression_compress_init(size_t buffer_size)
{
	int i;
	compression_buffer *buffer;
	
	buffer = bedrock_malloc(sizeof(compression_buffer));

	buffer->type = ZLIB_COMPRESS;
	buffer->buffer_size = buffer_size;

	buffer->stream.zalloc = Z_NULL;
	buffer->stream.zfree = Z_NULL;
	buffer->stream.opaque = Z_NULL;

	i = deflateInit2(&buffer->stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY);
	if (i != Z_OK)
	{
		bedrock_log(LEVEL_CRIT, "zlib: Error initializing deflate stream - error code %d", i);
		bedrock_free(buffer);
		return NULL;
	}

	buffer->buffer = bedrock_buffer_create("compression buffer", NULL, 0, buffer->buffer_size);

	return buffer;
}

void compression_compress_end(compression_buffer *buffer)
{
	bedrock_assert(buffer->type == ZLIB_COMPRESS, return);
	deflateEnd(&buffer->stream);
	bedrock_buffer_free(buffer->buffer);
	bedrock_free(buffer);
}

void compression_compress_reset(compression_buffer *buffer)
{
	int i;

	bedrock_assert(buffer != NULL && buffer->type == ZLIB_COMPRESS, return);

	i = deflateReset(&buffer->stream);
	if (i != Z_OK)
		bedrock_log(LEVEL_CRIT, "zlib: Error reinitializing deflate stream - error code %d", i);

	if (buffer->buffer != NULL)
		buffer->buffer->length = 0;
	else
		buffer->buffer = bedrock_buffer_create("compression buffer", NULL, 0, buffer->buffer_size);
}

static void compress_deflate(compression_buffer *buffer, const unsigned char *data, size_t len, int type)
{
	int i;
	z_stream *stream;

	bedrock_assert(buffer != NULL && buffer->type == ZLIB_COMPRESS && data != NULL && len > 0, return);

	stream = &buffer->stream;

	stream->next_in = (void *) data;
	stream->avail_in = len;

	bedrock_assert(stream->avail_in == len, return);

	do
	{
		bedrock_buffer *buf = buffer->buffer;

		bedrock_buffer_ensure_capacity(buf, BEDROCK_BUFFER_DEFAULT_SIZE);

		stream->next_out = buf->data + buf->length;
		stream->avail_out = BEDROCK_BUFFER_DEFAULT_SIZE;

		i = deflate(stream, type);
		if (i == Z_OK || i == Z_STREAM_END)
			buf->length += BEDROCK_BUFFER_DEFAULT_SIZE - buffer->stream.avail_out;
	}
	while (stream->avail_out == 0);

	if (i != Z_OK && i != Z_STREAM_END)
		bedrock_log(LEVEL_CRIT, "zlib: Error deflating stream - error code %d", i);
}

void compression_compress_deflate(compression_buffer *buffer, const unsigned char *data, size_t len)
{
	compress_deflate(buffer, data, len, Z_BLOCK);
}

void compression_compress_deflate_finish(compression_buffer *buffer, const unsigned char *data, size_t len)
{
	compress_deflate(buffer, data, len, Z_FINISH);
}

compression_buffer *compression_decompress_init(size_t buffer_size)
{
	int i;
	compression_buffer *buffer;
	
	buffer = bedrock_malloc(sizeof(compression_buffer));

	buffer->type = ZLIB_DECOMPRESS;
	buffer->buffer_size = buffer_size;

	buffer->stream.zalloc = Z_NULL;
	buffer->stream.zfree = Z_NULL;
	buffer->stream.opaque = Z_NULL;

	i = inflateInit2(&buffer->stream, 15 + 32);
	if (i != Z_OK)
	{
		bedrock_log(LEVEL_CRIT, "zlib: Error initializing deflate stream - error code %d", i);
		bedrock_free(buffer);
		return NULL;
	}

	buffer->buffer = bedrock_buffer_create("decompression buffer", NULL, 0, buffer_size);

	return buffer;
}

void compression_decompress_end(compression_buffer *buffer)
{
	bedrock_assert(buffer->type == ZLIB_DECOMPRESS, return);
	inflateEnd(&buffer->stream);
	bedrock_buffer_free(buffer->buffer);
	bedrock_free(buffer);
}

void compression_decompress_reset(compression_buffer *buffer)
{
	int i;

	bedrock_assert(buffer != NULL && buffer->type == ZLIB_DECOMPRESS, return);

	i = inflateReset(&buffer->stream);
	if (i != Z_OK)
		bedrock_log(LEVEL_CRIT, "zlib: Error reinitializing deflate stream - error code %d", i);

	if (buffer->buffer != NULL)
		buffer->buffer->length = 0;
	else
		buffer->buffer = bedrock_buffer_create("decompression buffer", NULL, 0, buffer->buffer_size);
}

void compression_decompress_inflate(compression_buffer *buffer, const unsigned char *data, size_t len)
{
	int i;
	z_stream *stream;

	bedrock_assert(buffer != NULL && buffer->type == ZLIB_DECOMPRESS && data != NULL && len > 0, return);

	stream = &buffer->stream;

	stream->next_in = (void *) data;
	stream->avail_in = len;

	bedrock_assert(stream->avail_in == len, return);

	do
	{
		bedrock_buffer *buf = buffer->buffer;

		bedrock_buffer_ensure_capacity(buf, BEDROCK_BUFFER_DEFAULT_SIZE);

		stream->next_out = buf->data + buf->length;
		stream->avail_out = BEDROCK_BUFFER_DEFAULT_SIZE;

		i = inflate(stream, Z_NO_FLUSH);
		if (i == Z_OK || i == Z_STREAM_END)
			buf->length += BEDROCK_BUFFER_DEFAULT_SIZE - stream->avail_out;
	}
	while (stream->avail_out == 0);

	if (i != Z_OK && i != Z_STREAM_END)
		bedrock_log(LEVEL_CRIT, "zlib: Error inflating stream - error code %d", i);
}

compression_buffer *compression_compress(size_t buffer_size, const unsigned char *data, size_t len)
{
	compression_buffer *buffer = compression_compress_init(buffer_size);
	compression_compress_deflate_finish(buffer, data, len);
	bedrock_buffer_resize(buffer->buffer, buffer->buffer->length);
	return buffer;
}

compression_buffer *compression_decompress(size_t buffer_size, const unsigned char *data, size_t len)
{
	compression_buffer *buffer = compression_decompress_init(buffer_size);
	compression_decompress_inflate(buffer, data, len);
	bedrock_buffer_resize(buffer->buffer, buffer->buffer->length);
	return buffer;
}
