


#include "Path.h"



TileCoordinate Path::GetNextTile() const
{
    check(_currentIndex != _path.Num());
    const auto nextTileCoord = (_path)[_currentIndex];
    ++_currentIndex;
    return nextTileCoord;
}

bool Path::IsPathFullyTraversed() const
{
    check(0 != _path.Num());
    return (_path.Num() <= _currentIndex);
}

bool Path::IsEmpty() const
{
    check(0 != _path.Num());
    return (1 == _path.Num());
}

u32 Path::Length() const
{
    check(0 != _path.Num());
    return (u32)(_path.Num() - 1);
}


TileCoordinate Path::PeekAhead(const u32 tileNumber) const
{
    check(0 != _path.Num());
    check((int32)tileNumber < _path.Num());
    return (_path)[tileNumber];
}

u32 Path::TotalNumEdgesTraversedUntilNow() const
{
    check(0 != _path.Num());
    return _currentIndex - 1U;
}

TileCoordinate Path::GetLastTileInPath() const
{
    check(0 != _path.Num());
    return (_path)[_path.Num() - 1];
}


TArrayTilesInline16& Path::operator=(TArrayTilesInline16&& newPath)
{
    _path = std::move(newPath);
    _currentIndex = 1U;
    check(_path.Num() != 0);

    return _path;
}