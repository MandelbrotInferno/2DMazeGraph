


#include "MazeGraph.h"
#include "Algo/Reverse.h"



void MazeGraph::ConnectedMazeNodesToSingleNode::AddNode(const TileCoordinate nodeCoord, const u32 tilemapWidth)
{

	if (connectedNodes.Num() == size) { return; }

	const u32 nodeIndex = nodeCoord.Y * tilemapWidth + nodeCoord.X;
	for (u32 i = 0U; i < size; ++i) {
		if (nodeIndex == connectedNodes[i]) {
			return;
		}
	}

	connectedNodes[size] = nodeIndex;
	++size;
}



void MazeGraph::Init(const u32 tileMapWidth, const u32 tileMapHeight)
{
	tilemapWidth = tileMapWidth;
	tilemapHeight = tileMapHeight;

	constexpr u32 totalNumDirections = 4;

	_adjList.Empty();
	_visited.Empty();
	_defaultVisited.Empty();
	_tree.Empty();

	_adjList.Reserve(64U);
	_visited.Reserve(64U);
	_defaultVisited.Reserve(64U);
	_tree.Reserve(256U);


	const auto totalNumTiles = tilemapWidth * tilemapHeight;
	_indicesToVertices.SetNumUninitialized(totalNumTiles, EAllowShrinking::Yes);

	for (size_t i = 0U; i < _indicesToVertices.Num(); ++i) {
		_indicesToVertices[i] = std::numeric_limits<u32>::max();
	}

	_currentAdjListIndex = 0U;
}


u32 MazeGraph::GetTotalNumVertices() const
{
	return (u32)_adjList.Num();
}

u32 MazeGraph::GetTotalNumEdges() const
{
	u32 totalNumEdges{};
	for (const auto& connectedNodes : _adjList) {
		totalNumEdges += connectedNodes.size;
	}

	return totalNumEdges / 2U;
}


void MazeGraph::AddEdge(const TileCoordinate v, const TileCoordinate w)
{
	if (v.X < tilemapWidth && v.Y < tilemapHeight && w.X < tilemapWidth && w.Y < tilemapHeight) [[likely]] {

		const u32 vertexIndexV = GetVertexNumberFromTileCoord(v);
		const u32 vertexIndexW = GetVertexNumberFromTileCoord(w);

		const u32 indexOfVertexIndexV = _indicesToVertices[vertexIndexV];
		const u32 indexOfVertexIndexW = _indicesToVertices[vertexIndexW];

		if (std::numeric_limits<u32>::max() == indexOfVertexIndexV) {
			ConnectedMazeNodesToSingleNode connectedNodesV{};
			connectedNodesV.AddNode(w, tilemapWidth);
			_adjList.Add(connectedNodesV);
			_visited.Add(false);
			_defaultVisited.Add(false);
			_indicesToVertices[vertexIndexV] = _currentAdjListIndex;
			++_currentAdjListIndex;
		}
		else {
			auto& connectedNodesV = _adjList[indexOfVertexIndexV];
			connectedNodesV.AddNode(w, tilemapWidth);
		}


		if (std::numeric_limits<u32>::max() == indexOfVertexIndexW) {
			ConnectedMazeNodesToSingleNode connectedNodesW{};
			connectedNodesW.AddNode(v, tilemapWidth);
			_adjList.Add(connectedNodesW);
			_visited.Add(false);
			_defaultVisited.Add(false);
			_indicesToVertices[vertexIndexW] = _currentAdjListIndex;
			++_currentAdjListIndex;
		}
		else {
			auto& connectedNodesW = _adjList[indexOfVertexIndexW];
			connectedNodesW.AddNode(v, tilemapWidth);
		}
		
	}
}


TArrayTilesInline16 MazeGraph::GetAllAdjacentVertices(const TileCoordinate tileCoord) const
{
	TArrayTilesInline16 adjacentVerticesToRequestedVertex{};

	const u32 vertexIndexTile = GetVertexNumberFromTileCoord(tileCoord);
	const auto& connectedNodes = GetConnectedNodeToRequestedVertexNumber(vertexIndexTile);
	for (u32 i = 0U; i < connectedNodes.size; ++i) {
		const auto currentConnectedNodeNumber = connectedNodes.connectedNodes[i];
		const TileCoordinate adjacentTileCoord = GetTileCoordFromVertexNumber(currentConnectedNodeNumber);
		adjacentVerticesToRequestedVertex.Add(adjacentTileCoord);
	}

	return adjacentVerticesToRequestedVertex;
}

TArrayTilesInline16 MazeGraph::GetAllAdjacentVerticesWithoutOne(const TileCoordinate tileCoord, const TileCoordinate adjacentToExclude) const
{
	TArrayTilesInline16 adjacentVerticesWithoutOneAdjacent{};

	const u32 vertexIndexTile = GetVertexNumberFromTileCoord(tileCoord);
	const auto& connectedNodes = GetConnectedNodeToRequestedVertexNumber(vertexIndexTile);
	for (u32 i = 0U; i < connectedNodes.size; ++i) {
		const auto currentConnectedNodeNumber = connectedNodes.connectedNodes[i];
		const TileCoordinate adjacentTileCoord = GetTileCoordFromVertexNumber(currentConnectedNodeNumber);
		if (adjacentToExclude != adjacentTileCoord) {
			adjacentVerticesWithoutOneAdjacent.Add(adjacentTileCoord);
		}
	}

	return adjacentVerticesWithoutOneAdjacent;
}

bool MazeGraph::IsAdjacent(const TileCoordinate v, const TileCoordinate w) const
{
	const auto vertexIndexV = GetVertexNumberFromTileCoord(v);
	const auto vertexIndexW = GetVertexNumberFromTileCoord(w);

	const auto& connectedNodes = GetConnectedNodeToRequestedVertexNumber(vertexIndexV);
	for (u32 i = 0U; i < connectedNodes.size; ++i) {
		if (vertexIndexW == connectedNodes.connectedNodes[i]) {
			return true;
		}
	}

	return false;
}


TArrayTilesInline16 MazeGraph::GeneratePathWithFixedLength(const TileCoordinate source, const u32 length) const
{
	memcpy(_visited.GetData(), _defaultVisited.GetData(), _visited.Num() * sizeof(bool));
	_queueWithLength.Empty();
	_tree.SetNum(0U, EAllowShrinking::No);
	TArrayTilesInline16 path{};

	const auto nodeNumberSource = GetVertexNumberFromTileCoord(source);

	_queueWithLength.Enqueue({ nodeNumberSource, 0U});
	TraversedNode targetTraversedNode{};
	targetTraversedNode.nodeNumber = nodeNumberSource;
	_tree.Add(targetTraversedNode);
	MarkVertexAsVisited(nodeNumberSource);

	if (0U == length) {
		path.Add(GetTileCoordFromVertexNumber(nodeNumberSource));
		return path; 
	}


	u32 currentParentIndex{};
	u32 currentLongestLengthEndNode{};
	while (false == _queueWithLength.IsEmpty()) {

		FUint32Vector2 currentNodeNumber{};
		_queueWithLength.Dequeue(currentNodeNumber);

		if (length == currentNodeNumber.Y) { break; }

		const auto& connectedNodes = GetConnectedNodeToRequestedVertexNumber(currentNodeNumber.X);
		for (size_t i = 0U; i < connectedNodes.size; ++i) {

			const auto currentConnectedNodeNumber = connectedNodes.connectedNodes[i];

			if (false == VertexVisitedBefore(currentConnectedNodeNumber)) {
				_queueWithLength.Enqueue({ currentConnectedNodeNumber, currentNodeNumber.Y + 1U});
				_tree.Add(TraversedNode{ currentConnectedNodeNumber, currentParentIndex });
				MarkVertexAsVisited(currentConnectedNodeNumber);
				++currentLongestLengthEndNode;
			}

		}

		++currentParentIndex;
	}

	u32 tempNodeNumber = _tree[currentLongestLengthEndNode].nodeNumber;
	u32 tempParentNodeNumber = _tree[currentLongestLengthEndNode].parentNodeIndex;
	path.Add(GetTileCoordFromVertexNumber(tempNodeNumber));
	while (nodeNumberSource != tempNodeNumber) {

		if (std::numeric_limits<u32>::max() == tempParentNodeNumber) {
			break;
		}
		tempNodeNumber = _tree[tempParentNodeNumber].nodeNumber;
		tempParentNodeNumber = _tree[tempParentNodeNumber].parentNodeIndex;

		path.Add(GetTileCoordFromVertexNumber(tempNodeNumber));
	}

	Algo::Reverse(path);
	return path;
}

TArrayTilesInline16 MazeGraph::GeneratePathAlongDirection(const TileCoordinate source
	, const FInt32Vector2 direction, const u32 maxLength) const
{
	memcpy(_visited.GetData(), _defaultVisited.GetData(), _visited.Num() * sizeof(bool));
	_queue.Empty();
	_tree.SetNum(0U, EAllowShrinking::No);
	TArrayTilesInline16 path{};

	const auto nodeNumberSource = GetVertexNumberFromTileCoord(source);

	_queue.Enqueue(nodeNumberSource);
	TraversedNode targetTraversedNode{};
	targetTraversedNode.nodeNumber = nodeNumberSource;
	_tree.Add(targetTraversedNode);
	MarkVertexAsVisited(nodeNumberSource);


	if (FInt32Vector2{} == direction) {
		path.Add(GetTileCoordFromVertexNumber(nodeNumberSource));
		return path;
	}

	u32 currentParentIndex{};
	u32 currentLength{};
	while (false == _queue.IsEmpty()) {

		u32 currentNodeNumber{};
		_queue.Dequeue(currentNodeNumber);

		const auto& connectedNodes = GetConnectedNodeToRequestedVertexNumber(currentNodeNumber);

		if (0U != connectedNodes.size) {
			const u32 mostAlignedTileCoordIndex = FindMostAlignedTileCoordIndexAlongDirection(connectedNodes, currentNodeNumber, direction);

			if (std::numeric_limits<u32>::max() == mostAlignedTileCoordIndex || ((std::numeric_limits<u32>::max() != maxLength) 
				&& (maxLength == currentLength)))
			{ 
				break; 
			}

			const auto currentConnectedNodeNumber = connectedNodes.connectedNodes[mostAlignedTileCoordIndex];

			if (false == VertexVisitedBefore(currentConnectedNodeNumber)) {
				_queue.Enqueue(currentConnectedNodeNumber);
				_tree.Add(TraversedNode{ currentConnectedNodeNumber, currentParentIndex });
				MarkVertexAsVisited(currentConnectedNodeNumber);
				++currentParentIndex;
				++currentLength;
			}
		}
		else { break; }

	}

	u32 tempNodeNumber = _tree[currentParentIndex].nodeNumber;
	u32 tempParentNodeNumber = _tree[currentParentIndex].parentNodeIndex;
	path.Add(GetTileCoordFromVertexNumber(tempNodeNumber));
	while (nodeNumberSource != tempNodeNumber) {

		if (std::numeric_limits<u32>::max() == tempParentNodeNumber) {
			break;
		}
		tempNodeNumber = _tree[tempParentNodeNumber].nodeNumber;
		tempParentNodeNumber = _tree[tempParentNodeNumber].parentNodeIndex;

		path.Add(GetTileCoordFromVertexNumber(tempNodeNumber));
	}

	Algo::Reverse(path);
	return path;
}

TArrayTilesInline16 MazeGraph::GeneratePathBetweenTwoTilesNotGoingThroughSpecifiedTile(const TileCoordinate source
	,const TileCoordinate target, const TileCoordinate tileToAvoid) const
{
	memcpy(_visited.GetData(), _defaultVisited.GetData(), _visited.Num() * sizeof(bool));
	_queue.Empty();
	_tree.SetNum(0U, EAllowShrinking::No);
	TArrayTilesInline16 path{};

	const auto nodeNumberSource = GetVertexNumberFromTileCoord(source);
	const auto nodeNumberTileToAvoid = GetVertexNumberFromTileCoord(tileToAvoid);
	const auto nodeNumberTarget = GetVertexNumberFromTileCoord(target);

	_queue.Enqueue(nodeNumberTarget);
	TraversedNode targetTraversedNode{};
	targetTraversedNode.nodeNumber = nodeNumberTarget;
	_tree.Add(targetTraversedNode);
	MarkVertexAsVisited(nodeNumberTarget);

	if (source == tileToAvoid || source == target || tileToAvoid == target) {
		path.Add(GetTileCoordFromVertexNumber(nodeNumberSource));
		return path;
	}

	u32 currentParentIndex{};
	while (false == _queue.IsEmpty()) {

		u32 currentNodeNumber{};
		_queue.Dequeue(currentNodeNumber);

		if (currentNodeNumber == nodeNumberSource) { break; }

		const auto& connectedNodes = GetConnectedNodeToRequestedVertexNumber(currentNodeNumber);
		for (size_t i = 0U; i < connectedNodes.size; ++i) {

			const auto currentConnectedNodeNumber = connectedNodes.connectedNodes[i];

			if (currentNodeNumber != nodeNumberTileToAvoid && false == VertexVisitedBefore(currentConnectedNodeNumber)) {
				_queue.Enqueue(currentConnectedNodeNumber);
				_tree.Add(TraversedNode{ currentConnectedNodeNumber, currentParentIndex });
				MarkVertexAsVisited(currentConnectedNodeNumber);
			}

		}
		++currentParentIndex;
	}

	if (_tree.Num() == currentParentIndex) {
		path.Add(GetTileCoordFromVertexNumber(nodeNumberSource));
		return path;
	}

	u32 tempNodeNumber = _tree[currentParentIndex].nodeNumber;
	u32 tempParentNodeNumber = _tree[currentParentIndex].parentNodeIndex;
	path.Add(GetTileCoordFromVertexNumber(tempNodeNumber));
	while (nodeNumberTarget != tempNodeNumber) {

		if (std::numeric_limits<u32>::max() == tempParentNodeNumber) [[unlikely]] {
			break;
		}

		tempNodeNumber = _tree[tempParentNodeNumber].nodeNumber;
		tempParentNodeNumber = _tree[tempParentNodeNumber].parentNodeIndex;

		path.Add(GetTileCoordFromVertexNumber(tempNodeNumber));
	}
	return path;
}


TArrayTilesInline16 MazeGraph::GenerateShortestPathFromSourceToTarget(const TileCoordinate source, const TileCoordinate target) const
{
	memcpy(_visited.GetData(), _defaultVisited.GetData(), _visited.Num() * sizeof(bool));
	_queue.Empty();
	_tree.SetNum(0U, EAllowShrinking::No);
	TArrayTilesInline16 path{};

	const auto nodeNumberSource = GetVertexNumberFromTileCoord(source);
	const auto nodeNumberTarget = GetVertexNumberFromTileCoord(target);

	_queue.Enqueue(nodeNumberTarget);
	TraversedNode targetTraversedNode{};
	targetTraversedNode.nodeNumber = nodeNumberTarget;
	_tree.Add(targetTraversedNode);
	MarkVertexAsVisited(nodeNumberTarget);

	u32 currentParentIndex{};
	while (false == _queue.IsEmpty()) {

		u32 currentNodeNumber{};
		_queue.Dequeue(currentNodeNumber);

		if (currentNodeNumber == nodeNumberSource) { break; }

		const auto& connectedNodes = GetConnectedNodeToRequestedVertexNumber(currentNodeNumber);
		for (size_t i = 0U; i < connectedNodes.size; ++i) {

			const auto currentConnectedNodeNumber = connectedNodes.connectedNodes[i];

			if (false == VertexVisitedBefore(currentConnectedNodeNumber)) {
				_queue.Enqueue(currentConnectedNodeNumber);
				_tree.Add(TraversedNode{ currentConnectedNodeNumber, currentParentIndex });
				MarkVertexAsVisited(currentConnectedNodeNumber);
			}

		}
		++currentParentIndex;
	}

	u32 tempNodeNumber = _tree[currentParentIndex].nodeNumber;
	u32 tempParentNodeNumber = _tree[currentParentIndex].parentNodeIndex;
	path.Add(GetTileCoordFromVertexNumber(tempNodeNumber));
	while (nodeNumberTarget != tempNodeNumber) {

		if (std::numeric_limits<u32>::max() == tempParentNodeNumber) [[unlikely]] {
			break;
		}

		tempNodeNumber = _tree[tempParentNodeNumber].nodeNumber;
		tempParentNodeNumber = _tree[tempParentNodeNumber].parentNodeIndex;

		path.Add(GetTileCoordFromVertexNumber(tempNodeNumber));
	}
	
	return path;
}

TArrayTilesInline16 MazeGraph::GenerateNonTrivialRandomPathStartingFromSource(const TileCoordinate source) const
{
	TileCoordinate target = source;
	while (target == source) {
		target = GetRandomTileCoordinate();
	}

	return GenerateShortestPathFromSourceToTarget(source, target);

}


TileCoordinate MazeGraph::GetTileCoordFromVertexNumber(const u32 nodeNumber) const
{
	const auto y = nodeNumber / tilemapWidth;
	const auto x = nodeNumber % tilemapWidth;

	return TileCoordinate{ x, y };
}


u32 MazeGraph::GetVertexNumberFromTileCoord(const TileCoordinate tileCoord) const
{
	return tileCoord.Y * tilemapWidth + tileCoord.X;
}

TileCoordinate MazeGraph::GetRandomTileCoordinate() const
{
	check(_adjList.Num() != 0);

	const auto randomIndex = FMath::RandRange(0, _adjList.Num() - 1U);

	return GetTileCoordFromVertexNumber(_adjList[randomIndex].connectedNodes[0]);
}

MazeGraph::ConnectedMazeNodesToSingleNode& MazeGraph::GetConnectedNodeToRequestedVertexNumber(const u32 vertexNumber)
{
	return _adjList[_indicesToVertices[vertexNumber]];
}

const MazeGraph::ConnectedMazeNodesToSingleNode& MazeGraph::GetConnectedNodeToRequestedVertexNumber(const u32 vertexNumber) const
{
	return _adjList[_indicesToVertices[vertexNumber]];
}

bool MazeGraph::VertexVisitedBefore(const u32 vertexNumber) const
{
	return _visited[_indicesToVertices[vertexNumber]];
}

void MazeGraph::MarkVertexAsVisited(const u32 vertexNumber) const
{
	_visited[_indicesToVertices[vertexNumber]] = true;
}

u32 MazeGraph::FindMostAlignedTileCoordIndexAlongDirection(const ConnectedMazeNodesToSingleNode& connectedNodes
	, const u32 parentVertexNumber, const FInt32Vector2 direction) const
{
	check(0U != connectedNodes.size);
	
	TStaticArray<int32, 4> dotProducts{};
	const auto parentTileCoord = GetTileCoordFromVertexNumber(parentVertexNumber);

	bool positiveDot{ false };
	for (size_t i = 0U; i < connectedNodes.size; ++i) {
		const auto adjacentTileCoord = GetTileCoordFromVertexNumber(connectedNodes.connectedNodes[i]);
		const FInt32Vector2 inducedDirection{(int32)adjacentTileCoord.X - (int32)parentTileCoord.X, -(int32)adjacentTileCoord.Y + (int32)parentTileCoord.Y};
		const int32 dotProduct = inducedDirection.X * direction.X + inducedDirection.Y * direction.Y;
		dotProducts[i] = dotProduct;
		if (0 < dotProduct) {
			positiveDot = true;
		}
	}

	if (true == positiveDot) {

		u32 maxDotProductsIndex = 0U;
		for (size_t i = 0U; i < connectedNodes.size; ++i) {
			if (dotProducts[maxDotProductsIndex] < dotProducts[i]) {
				maxDotProductsIndex = i;
			}
		}
		return maxDotProductsIndex;
	}
	else {
		return std::numeric_limits<u32>::max();
	}

}
