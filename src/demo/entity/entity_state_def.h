#ifndef ENTITY_STATE_DEF_H__
#define ENTITY_STATE_DEF_H__

struct Game_State;
struct World;
struct Entity;

typedef enum {
  HERO_STATE_WALK,
  HERO_STATE_DASH,
  HERO_STATE_COUNT,
} Hero_State;

typedef struct {
  void (*on_enter)(struct World *world, struct Entity *entity);
  void (*on_exit)(struct World *world, struct Entity *entity);
  void (*on_update)(struct World *world, struct Entity *entity, f32 dt);
} State_Func;

typedef struct {
  State_Func funcs[HERO_STATE_COUNT];
  Hero_State state;
} Hero_State_Machine;


#endif
