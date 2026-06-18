# AINavData

Dungeon AI navigation data (`ainavdata_XXXXX.dat` in `Data/navmesh`).

```cs
// Header
1   byte    version                 // expecting 1
4   uint    simpleDungeonDataOffset

// CRefDungeon
2   ushort  regionID
4   uint    blockCount
for (int i = 0; i < blockCount; i++)
{
    // CRefBlock
    4   uint    block.Index
    4   uint    block.CellCount
    4   uint    block.EdgeCount

    // CellLookupTable[CellCount * CellCount]
    for (goalCell = 0; goalCell < CellCount; goalCell++)
        for (startCell = 0; startCell < CellCount; startCell++)
        {
            2   short   refEdgeIndex0   // forward path
            2   short   refEdgeIndex1   // backward path
        }

    4   uint    linkCount
    for (int i = 0; i < linkCount; i++)
    {
        4   uint    link.ID
        2   ushort  link.CellID
        2   ushort  link.LinkedObjID
        2   ushort  link.LinkedObjRefEdgeIndex
    }   // 10 bytes per link (no padding)
}

// BlockLookupTable[BlockCount * BlockCount]
for (goalBlock = 0; goalBlock < blockCount; goalBlock++)
    for (startBlock = 0; startBlock < blockCount; startBlock++)
        2   short   refCellID

4   uint    int0    // expecting 0

// SimpleDungeonData @ simpleDungeonDataOffset
2   ushort  regionID
4   uint    blockCount
for (uint i = 0; i < blockCount; i++)
{
    4   uint    edgeCount
    for (uint j = 0; j < edgeCount; j++)
        12  Vector3 edgeCenter
}
4   uint    int1    // expecting 0
```

See also: [JMXVNVM](JMXVNVM.md) for terrain navmesh (`.nvm`).
