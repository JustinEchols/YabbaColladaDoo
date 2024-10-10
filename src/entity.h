#if !defined(ENTITY_H)

enum entity_type
{
	EntityType_Player,
	EntityType_Vampire,
	EntityType_Light,
	EntityType_Block,
	EntityType_Paladin,
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
