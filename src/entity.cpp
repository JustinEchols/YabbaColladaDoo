
internal entity * 
EntityAdd(game_state *GameState, entity_type Type)
{
	Assert(GameState->EntityCount < ArrayCount(GameState->Entities));
	entity *Entity = GameState->Entities + GameState->EntityCount++;
	Entity->Type = Type;
	return(Entity);
}

internal void
PlayerAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Player);
	Entity->P = V3(100.0f, -80.0f, -300.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal void
VampireAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Vampire);
	Entity->P = V3(-100.0f, -80.0f, -300.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal void
LightAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Light);
	Entity->P = V3(0.0f, 20.0f, -275.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal void
PaladinAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Paladin);
	Entity->P = V3(-100.0f, -80.0f, -200.0f);
	Entity->dP = V3(0.0f);
	Entity->ddP = V3(0.0f);
	Entity->Orientation = Quaternion(V3(0.0f, 1.0f, 0.0f), 0.0f);
}

internal void
BlockAdd(game_state *GameState)
{
	entity *Entity = EntityAdd(GameState, EntityType_Block);
	Entity->P = V3(0.0f, 5.0f, -30.0f);
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
