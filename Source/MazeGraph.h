

#pragma once

#include "CoreMinimal.h"
#include "Containers/StaticArray.h"
#include "Containers/Array.h"
#include "Containers/Queue.h"
#include "CommonTypes.h"

#include <limits>


/*
* It is primarily intended to be used for tile-based games like turn-based and tiled rpgs.
* 
* It assumes that ONLY walkable tiles will 
* be used as the vertices and that the graph 
* to be constructed is undirected connected one 
* where each tile has the same cost.
* 
* Vertices and all its edges can be added at once only for now.
* This will change in the futrue where I will try adding any
* number of adjacent edges to the graph.
* 
*It is not thread safe.
*/

class PACMANUE_API MazeGraph final
{
private:

	struct ConnectedMazeNodesToSingleNode final
	{

		void AddNode(const TileCoordinate nodeCoord, const u32 tilemapWidth);

		TStaticArray<u32, 4> connectedNodes{std::numeric_limits<u32>::max()};
		u32 size{};
	};

	struct TraversedNode final
	{
		u32 nodeNumber{};
		u32 parentNodeIndex{ std::numeric_limits<u32>::max() };
	};

public:

	/*
	* Used to initialize the graph dimensions. 
	* AddEdge() should be used to fill it with vertices 
	* and edges.
	* 
	* It can be reused indefinitely to reinitialize 
	* the graph and use AddEdge to fill the vertices
	* and edges.
	*/
	void Init(const u32 tileMapWidth, const u32 tileMapHeight);


	[[nodiscard]] u32 GetTotalNumVertices() const;


	[[nodiscard]] u32 GetTotalNumEdges() const;

	/*
	* The passed tile coordinates do not need to be bound checked before passing them.
	* Any tile coordinates outside of boundary will be ignored.
	*/
	void AddEdge(const TileCoordinate v, const TileCoordinate w);


	/*
	* Make sure to check the bounds of the given tile coordinate before passing it.
	*/
	[[nodiscard]] TArrayTilesInline16 GetAllAdjacentVertices(const TileCoordinate tileCoord) const;

	/*
	* Make sure to check the bounds of the given tile coordinates before passing them.
	*/
	[[nodiscard]] TArrayTilesInline16 GetAllAdjacentVerticesWithoutOne(const TileCoordinate tileCoord
		, const TileCoordinate adjacentToExclude) const;

	/*
	* Make sure to check the bounds of the given tile coordinates before passing them.
	*/
	[[nodiscard]] bool IsAdjacent(const TileCoordinate v, const TileCoordinate w) const;

	/*
	* Make sure to check the bounds of the given tile coordinates before passing them.
	* 
	* Returned path will always have at least 1 element in it which is the source.
	*/
	[[nodiscard]] TArrayTilesInline16 GenerateShortestPathFromSourceToTarget(const TileCoordinate source
		, const TileCoordinate target) const;

	/*
	* Make sure to check the bounds of the given tile coordinate before passing it.
	* 
	* Returned path will always have at least 1 element in it which is the source.
	*/
	[[nodiscard]] TArrayTilesInline16 GenerateNonTrivialRandomPathStartingFromSource(const TileCoordinate source) const;


	/*
	* Make sure to check the bounds of the given tile coordinate before passing it.
	* 
	* Returned path will always have at least 1 element in it which is the source.
	*/
	[[nodiscard]] TArrayTilesInline16 GeneratePathWithFixedLength(const TileCoordinate source
		, const u32 length) const;


	/*
	* Make sure to check the bounds of the given tile coordinate before passing it.
	* 
	* Returned path will always have at least 1 element in it which is the source.
	*/
	[[nodiscard]] TArrayTilesInline16 GeneratePathAlongDirection(const TileCoordinate source
		, const FInt32Vector2 direction, const u32 maxLength = std::numeric_limits<u32>::max()) const;

	/*
	* Useful only when there is more than one path connecting any 2 distinct vertices.
	* Make sure to check the bounds of the given tile coordinates before passing them.
	* 
	* Returned path will always have at least 1 element in it which is the source.
	*/
	[[nodiscard]] TArrayTilesInline16 GeneratePathBetweenTwoTilesNotGoingThroughSpecifiedTile(const TileCoordinate source
	, const TileCoordinate target, const TileCoordinate tileToAvoid) const;
private:

	[[nodiscard]] TileCoordinate GetTileCoordFromVertexNumber(const u32 nodeNumber) const;

	[[nodiscard]] u32 GetVertexNumberFromTileCoord(const TileCoordinate tileCoord) const;

	[[nodiscard]] TileCoordinate GetRandomTileCoordinate() const;

	[[nodiscard]] ConnectedMazeNodesToSingleNode& GetConnectedNodeToRequestedVertexNumber(const u32 vertexNumber);
	[[nodiscard]] const ConnectedMazeNodesToSingleNode& GetConnectedNodeToRequestedVertexNumber(const u32 vertexNumber) const;

	[[nodiscard]] bool VertexVisitedBefore(const u32 vertexNumber) const;
	void MarkVertexAsVisited(const u32 vertexNumber) const;

	[[nodiscard]] u32 FindMostAlignedTileCoordIndexAlongDirection(const ConnectedMazeNodesToSingleNode& connectedNodes
		, const u32 parentVertexNumber,const FInt32Vector2 direction) const;

private:


	TArray<ConnectedMazeNodesToSingleNode> _adjList{};
	TArray<u32> _indicesToVertices{};
	u32 _currentAdjListIndex{};

	mutable TArray<TraversedNode> _tree{};
	mutable TQueue<u32> _queue{};
	mutable TQueue<FUint32Vector2> _queueWithLength{};
	mutable TArray<bool> _visited{};
	
	TArray<bool> _defaultVisited{};

	u32 tilemapWidth{};
	u32 tilemapHeight{};

};
