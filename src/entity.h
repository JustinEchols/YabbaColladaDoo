#if !defined(ENTITY_H)

enum entity_type
{
	EntityType_Player,
	EntityType_Vampire,
	EntityType_Light,
	EntityType_Block,
	EntityType_Paladin,
	EntityType_Ninja,
	EntityType_Arissa,
	EntityType_Maria,
	EntityType_Vangaurd,
	EntityType_Brute,
	EntityType_Mannequin,
	EntityType_ErikaArcher,
	EntityType_BulkyKnight,
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
