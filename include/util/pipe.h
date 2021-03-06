#ifndef BEDROCK_UTIL_PIPE_H
#define BEDROCK_UTIL_PIPE_H

#include "fd.h"

typedef void (*bedrock_pipe_notify_func)(void *);

struct bedrock_pipe
{
	struct bedrock_fd read_fd;
	struct bedrock_fd write_fd;
	bedrock_pipe_notify_func on_notify;
	void *data;
};
typedef struct bedrock_pipe bedrock_pipe;

extern void bedrock_pipe_open(bedrock_pipe *pipe, const char *desc, bedrock_pipe_notify_func on_notify, void *data);
extern void bedrock_pipe_close(bedrock_pipe *p);
extern void bedrock_pipe_notify(bedrock_pipe *p);

#endif // BEDROCK_UTIL_PIPE_H
