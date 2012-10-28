%{
#include "server/bedrock.h"
#include "server/world.h"
#include "server/oper.h"
#include "config/config.h"
#include "util/string.h"

extern int yylex();
extern void yyerror(const char *s);

static struct bedrock_world *world;
static struct bedrock_oper *oper;

%}

%error-verbose

%union
{
	int ival;
	char *sval;
}

%token <ival> CINT
%token <sval> STRING

/* World */
%token WORLD
%token NAME
%token PATH

/* Server */
%token SERVER
%token DESCRIPTION
%token MAXUSERS
%token IP
%token PORT
%token LOG_LEVEL
%token CRIT
%token WARN
%token INFO
%token DEBUG
%token COLUMN
%token NBT_DEBUG
%token THREAD
%token BUFFER
%token IO_DEBUG
%token PACKET_DEBUG

/* Operator */
%token OPER
%token PASSWORD
%token COMMANDS

%%

conf: | conf conf_items;

conf_items: world_entry | server_entry | operator_entry;

/* World */
world_entry: WORLD
{
	world = bedrock_malloc(sizeof(struct bedrock_world));
}
'{' world_items '}'
{
	bedrock_list_add(&world_list, world);
};

world_items: | world_item world_items;
world_item: world_name | world_path;

world_name: NAME '=' STRING ';'
{
	strncpy(world->name, yylval.sval, sizeof(world->name));
};

world_path: PATH '=' STRING ';'
{
	strncpy(world->path, yylval.sval, sizeof(world->path));
};

/* Server */
server_entry: SERVER '{' server_items '}';

server_items: | server_item server_items;
server_item: server_description | server_maxusers | server_ip | server_port | server_log_level;

server_description: DESCRIPTION '=' STRING ';'
{
	strncpy(server_desc, yylval.sval, sizeof(server_desc));
};

server_maxusers: MAXUSERS '=' CINT ';'
{
	server_maxusers = yylval.ival;
};

server_ip: IP '=' STRING ';'
{
	strncpy(server_ip, yylval.sval, sizeof(server_ip));
};

server_port: PORT '=' CINT ';'
{
	server_port = yylval.ival;
};

server_log_level: LOG_LEVEL '=' server_log_level_items ';'
{
};

server_log_level_items: server_log_level_items ',' server_log_level_item | server_log_level_item;
server_log_level_item:
CRIT
{
	bedrock_conf_log_level |= LEVEL_CRIT;
}
| WARN
{
	bedrock_conf_log_level |= LEVEL_WARN;
}
| INFO
{
	bedrock_conf_log_level |= LEVEL_INFO;
}
| DEBUG
{
	bedrock_conf_log_level |= LEVEL_DEBUG;
}
| COLUMN
{
	bedrock_conf_log_level |= LEVEL_COLUMN;
}
| NBT_DEBUG
{
	bedrock_conf_log_level |= LEVEL_NBT_DEBUG;
}
| THREAD
{
	bedrock_conf_log_level |= LEVEL_THREAD;
}
| BUFFER
{
	bedrock_conf_log_level |= LEVEL_BUFFER;
}
| IO_DEBUG
{
	bedrock_conf_log_level |= LEVEL_IO_DEBUG;
}
| PACKET_DEBUG
{
	bedrock_conf_log_level |= LEVEL_PACKET_DEBUG;
};


/* Operator */
operator_entry: OPER
{
	oper = bedrock_malloc(sizeof(struct bedrock_oper));
	oper->commands.free = bedrock_free;
}
'{' oper_items '}'
{
	oper_conf_list.free = (bedrock_free_func) oper_free;
	bedrock_list_add(&oper_conf_list, oper);
};

oper_items: | oper_item oper_items;
oper_item: oper_name | oper_password | oper_commands;

oper_name: NAME '=' STRING ';'
{
	strncpy(oper->username, yylval.sval, sizeof(oper->username));
};

oper_password: PASSWORD '=' STRING ';'
{
	strncpy(oper->password, yylval.sval, sizeof(oper->password));
};

oper_commands: COMMANDS '=' STRING ';'
{
	char *start = yylval.sval;
	char *p = strchr(start, ' ');

	while (start != NULL)
	{
		if (p != NULL)
			*p++ = 0;

		bedrock_list_add(&oper->commands, bedrock_strdup(start));

		start = p;
		if (p != NULL)
			p = strchr(start, ' ');
	}
};

%%
