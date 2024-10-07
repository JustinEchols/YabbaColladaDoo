#if !defined(ENTITY_H)

enum entity_type
{
	EntityType_Player,
	EntityType_Vampire,
	EntityType_Light,
};

struct entity
{
	entity_type Type;

	v3 P;
	v3 dP;
	v3 ddP;
	quaternion Orientation;
};

#define ENTITY_H
#endif
