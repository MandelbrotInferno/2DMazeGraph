

#pragma once

#include "CoreMinimal.h"
#include "Math/IntPoint.h"

typedef uint32 u32;
typedef uint8 u8;
typedef FUint32Vector2 TileCoordinate;

/*
* This is used for returning paths to be traversed. One can
* change the minimal inline allocations based on prior knowledge
* of how long on average the paths will probably be in the 
* tile map based on its dimensions and other constraints.
*/
typedef TArray<TileCoordinate, TInlineAllocator<16>> TArrayTilesInline16;