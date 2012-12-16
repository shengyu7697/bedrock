/* Hard configuration for stuff that probably shouldn't change */

/* Tick length in milliseconds */
#define BEDROCK_TICK_LENGTH 50
/* Day length, in ticks */
#define BEDROCK_DAY_LENGTH 24000

/* Min username length */
#define BEDROCK_USERNAME_MIN 2
/* Max username length (with trailing \0), is there a standard for this? */
#define BEDROCK_USERNAME_MAX 17

/* The number of chunks around the player to send to them */
#define BEDROCK_VIEW_LENGTH 10

/* Protocol version we support */
#define BEDROCK_PROTOCOL_VERSION 49

/* Size of the initial buffer for clients */
#define BEDROCK_CLIENT_SEND_SIZE 4096

/* Max amount of data allowed to be recieved from a client before they are dropped */
#define BEDROCK_CLIENT_RECVQ_LENGTH 1024

/* The maximum string length allowed, including the trailing \0 */
#define BEDROCK_MAX_STRING_LENGTH 120

/* The maximum number of items per stack */
#define BEDROCK_MAX_ITEMS_PER_STACK 64

/* Number of blocks on one side of a chunk. Cube to get the number of blocks actually in a chunk */
#define BEDROCK_BLOCKS_PER_CHUNK   16
/* The number of chunks per column */
#define BEDROCK_CHUNKS_PER_COLUMN  16
/* The number of columns on the side of a region. Square for (max) columns per region. */
#define BEDROCK_COLUMNS_PER_REGION 32

#define BEDROCK_BLOCK_LENGTH BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK
#define BEDROCK_DATA_LENGTH BEDROCK_BLOCK_LENGTH / 2
#define BEDROCK_BIOME_LENGTH BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK
#define BEDROCK_HEIGHTMAP_LENGTH BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK

#define BEDROCK_REGION_HEADER_SIZE 1024
#define BEDROCK_REGION_SECTOR_SIZE 4096

#define BEDROCK_PLAYER_PATH "%s/players/%s.dat"

#define BEDROCK_REGIION_PATH "%s/region/r.%d.%d.mca"

#define BEDROCK_WORLD_LEVEL_FILE "level.dat"

/* Size of RSA key pair generated on startup */
#define BEDROCK_CERT_BIT_SIZE 1024

/* Length of verify token sent in encryption key request */
#define BEDROCK_VERIFY_TOKEN_LEN 4

/* Length of shared secret generated by clients */
#define BEDROCK_SHARED_SECRET_LEN 16

