
extern int packet_disconnect(struct client *client, const bedrock_packet *p);
extern void packet_send_disconnect(struct client *client, const char *reason);
