
internal entity * 
EntityAdd(game_state *GameState, entity_type Type)
{
	Assert(GameState->EntityCount < ArrayCount(GameState->Entities));
	entity *Entity = GameState->Entities + GameState->EntityCount++;
	Entity->Type = Type;
	return(Entity);
}

internal void
BlockAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Block);
	Entity->P = V3(-55.0f, 0.0f, -5.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal void
ErikaArcherAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_ErikaArcher);
	Entity->P = V3(-40.0f, 0.0f, -5.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal void
VampireAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Vampire);
	Entity->P = V3(-25.0f, 0.0f, -5.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal void
PlayerAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Player);
	Entity->P = V3(-10.0f, 0.0f, -5.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal void
PaladinAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Paladin);
	Entity->P = V3(5.0f, 0.0f, -5.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal void
NinjaAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Ninja);
	Entity->P = V3(20.0f, 0.0f, -5.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal void
MariaAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Maria);
	Entity->P = V3(35.0f, 0.0f, -5.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal void
BruteAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Brute);
	Entity->P = V3(50.0f, 0.0f, -5.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal void
MannequinAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Mannequin);
	Entity->P = V3(65.0f, 0.0f, -5.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}



internal void
LightAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Light);
	Entity->P = V3(20.0f, 5.0f, 0.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal mat4
EntityTransform(entity *Entity, f32 Scale = 1.0f)
{
	mat4 Result = Mat4Identity();

	mat4 R = QuaternionToMat4(Entity->Orientation);

	Result = Mat4(Scale * Mat4ColumnGet(R, 0),
				  Scale * Mat4ColumnGet(R, 1),
				  Scale * Mat4ColumnGet(R, 2),
				  Entity->P);

	return(Result);
}
