#ifndef ACTOR_H
#define ACTOR_H

typedef void (*handle_actor_command_t)(const char* topic, const char* data, int data_len);

extern handle_actor_command_t handle_actor_command;

#endif