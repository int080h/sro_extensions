# RTNavMeshTerrain
The RTNavMeshTerrain is a type of RTNavMesh that is used per non-dungeon-region in the World.

```cs
// RTNavMeshTerrain
12  char[]  Signature                          // JMXVNVM 1000
```

## ObjectList
The Object list describes an instance of an `RTNavMeshObj` and where it is placed in the World.
The AssetID references an `RTNavMeshObj` through the use of the [MapObjectIndex](JMXVOBJI1000.md).
When an object instance is placed a running unique ID (LocalUID) is created. The ID is only unique to all other objects within the same region (as per RegionID not RTNavMeshTerrain file). An object instance can be contained in the object list of multiple `RTNavMeshTerrain`s depending on placement and size. A WorldUID is formed on runtime to make an object instance globally identifable and to avoid duplicates when streaming in neighboring `RTNavMeshTerrain`s.
An object instance is therefore owned by a `RTNavMeshTerrain` when it's RegionID (file name) matches the instance's RegionID.

```cs
//ObjectList (Object instances)
2   short  objCount
for (int objIndex = 0; objIndex < objCount; objIndex++)
{   
    // MapObject
    {
        4   int    obj.AssetID              // used to look up the asset (.bsr/.cpd) from MapObjectIndex (object.ifo)
        12  Vector3 obj.LocalPosition       // position local to the region
        2   short   obj.Type                // -1 = Static, 0 = SkinedNavMesh?
        4   float   obj.Yaw                 // Rotation around the Y axis (height)
        2   short   obj.LocalUID            // UID within region
        // WorldUID = (obj->RegionID << 16) | obj->LocalUID

        2   short   obj.Short0              // ?
        1   bool    obj.IsBig               // Object is big, used bypass distance culling? to render landmarks (gates, bridges, etc...)
        1   bool    obj.IsStruct            // Object is a structure and requires an objectstring.ifo entry with additional info
        2   ushort  obj.RID                 // RegionID this object belongs to (origin region)
    }

    // links a GlobalEdge of `this` object instance to a GlobalEdge of another object instance avaiable within the same `RTNavMeshTerrain`. 
    2   ushort  linkEdgeCount
    for (int i = 0; i < linkEdgeCount; i++)
    {
        // Values of `-1` here are temporarily invalid. Picture 3 regions (left, center, right) with an object owned by them each. The left and right 
        // object are connected to the center object. All 3 objects will be included in every regions object list because of the linkage regardless of 
        // bounding box intersection with the region.
        2   short  linkEdge.LinkedObjID         // index from ObjectList
        2   short  linkEdge.LinkedObjEdgeID     // other object's edge
        2   short  linkEdge.EdgeID              // this object's edge
    }
}
```

## RTNavMeshCellQuad
![image](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/assets/13951844/60e9d851-976a-4756-9f28-f569707e363c)

An RTNavMeshCellQuad is a type of **RTNavMeshCell** that describes a Quadratic (2D Rectangle) subsection of the region.
It contains a list of indices to instances that are considered *inside* this cell.
RTNavMeshCellQuad are generated from the NavMeshTile to form the biggest possible Quad.
The minimum size of a cell is 20x20 units, the maximum size is 960x960 units (2x2 RTNavMeshCellQuads per region).

```cs
//RTNavMeshCellQuad (Cells/Nodes)
4   int    totalCellCount      // number of cells
4   int    openCellCount       // number of cells considered walkable
for (int cellIndex = 0; cellIndex < totalCellCount; cellIndex++)
{
    // NavRect
    8   Vector2 cell.Min
    8   Vector2 cell.Max 
    
    1   byte    cellObjCount        // number of object instances within this cell
    for (int i = 0; i < cellObjCount; i++)
        2   ushort  objIndex        // index from ObjectList
}
```

## RTNavMeshEdgeGlobal

An RTNavMeshEdgeGlobal, also known as a GlobalLinker, describes a line and a link between two border RTNavMeshCellQuads from **neighboring** RTNavMeshTerrains.
The line may be the entire side of the cells rectangle or sub-segment of it.
The link is bidirectional.

![image](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/assets/13951844/b0ddd016-d71a-487b-ad89-2415f0d715a5)

```cs
//RTNavMeshEdgeGlobal (GlobalEdges, OutlineEdges)
4   int    globalEdgeCount
for (int edgeIndex = 0; edgeIndex < globalEdgeCount; edgeIndex++)
{
    // NavLine
    8   Vector2 edge.Min
    8   Vector2 edge.Max

    1   byte    edge.Flag               // see EdgeFlag
    1   byte    edge.AssocDirection[0]  // see EdgeDirection -> index for m_EdgeList
    1   byte    edge.AssocDirection[1]  // see EdgeDirection -> index for m_EdgeList (-1 if Blocked)

    2   short  edge.AssocCell[0]       // QuadCell index -> index for pCell.m_CellList
    2   short  edge.AssocCell[1]       // QuadCell index -> index for pCell.m_CellList (-1 if Blocked)
    
    2   short  edge.AssocRegion[0]     // RegionID
    2   short  edge.AssocRegion[1]     // RegionID (-1 if Blocked)
}
```

## RTNavMeshEdgeInternal
An RTNavMeshEdgeInternal, also known as a LocalLinker, describes a line and a link between two RTNavMeshCellQuads **within** the same RTNavMeshTerrain.
The line may be the entire side of the cell's rectangle or sub-segment of it.
The link is bidirectional.

![image](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/assets/13951844/1f8aa543-d682-40f9-8b39-9a230bdaa103)

```cs
//RTNavMeshEdgeInternal (InlineEdges)
4   int    internalEdgeCount
for (int edgeIndex = 0; edgeIndex < internalEdgeCount; edgeIndex++)
{
    // NavLine
    8   Vector2 edge.Min
    8   Vector2 edge.Max 

    1   byte    edge.Flag               // see EdgeFlag
    1   byte    edge.AssocDirection[0]  // see EdgeDirection -> index for pCell.m_EdgeList
    1   byte    edge.AssocDirection[1]  // see EdgeDirection -> index for pCell.m_EdgeList (-1 if Blocked)

    2   short  edge.AssocCell[0]       // QuadCell index -> index for pCell.m_CellList
    2   short  edge.AssocCell[1]       // QuadCell index -> index for pCell.m_CellList (-1 if Blocked)
}
```

## TileMap
The TileMap describes a two-dimensional array of NavMeshTiles.
A NavMeshTile is 20x20 units in size.

![image](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/assets/13951844/65dbc3be-be55-47b6-9906-7087fd7f1aed)

```cs
//TileMap (96 * 96)
for (int i= 0; i < 96 * 96; i++)
{
    4   int     tile.CellID         // The `QuadCell` this `Tile` belongs to
    2   ushort  tile.Flag           // 1 = Blocked, everything else is kinda unknown. Split into 2 bytes?
    2   ushort  tile.TextureID      // ID from TextureIndex (Tile2D.ifo) (replaced at runtime with SoundFlag for foot-step sounds)
}
```

## HeightMap
The HeightMap describes a two-dimensional array of heights (vertex y-coordinates).

```cs
//HeightMap (97 * 97)
for (int i = 0; i < 97 * 97; i++)
{
    4   float   height
}
```

## PlaneMap
The PlaneMap describes a two-dimensional array of PlaneType and PlaneHeight.
A NavMeshPlane is a flat and 320x320 units in size.
A NavMeshPlane with PlaneType set to Ice is used when determining height above terrain.

```cs
//PlaneTypeMap
for (int i = 0; i < 6 * 6; i++)
{
    1   byte    planeType        //0 = None, 1 = Water, 2 = Ice
}

//PlaneHeightMap 
for (int i = 0; i < 6 * 6; i++)
{
    4   float   planeHeight
}
//EOF
```
## ImHex Pattern (JMXNVM.hexpat)

Allows for crude editing capabilities using [ImHex](https://github.com/WerWolv/ImHex)

```
#pragma array_limit 4294967296

struct Vector2
{
    float X;
    float Y;
};

struct Vector3
{
    float X;
    float Y;
    float Z;
};

struct MapObject
{
    u32 AssetID;
    Vector3 LocalPosition;
    u16 Type;
    float LocalYaw;
    u16 LocalUID;
    u16 Short0;
    bool IsBig;
    bool IsStruct;
    u16 RID;
};

struct LinkEdge
{
   u16 LinkedObjID;
   u16 LinkedObjEdgeID;
   u16 EdgeID;
};

struct NavMeshObjInst
{
   MapObject Object;
   u16 LinkEdgeCount;
   LinkEdge LinkEdges[LinkEdgeCount];
};

struct NavRect
{
    Vector2 Min;
    Vector2 Max;
};

struct NavCellQuad
{
    NavRect Rectangle;    
    u8 ObjectIndexCount;
    u16 ObjectIndices[ObjectIndexCount];
};

struct NavLine
{
    Vector2 Min;
    Vector2 Max;
};

struct NavEdgeGlobal
{
    NavLine Line;    
    u8 Flag;
    u8 AssocDirection[2];    
    u16 AssocCell[2];
    u16 AssocRegion[2];
};

struct NavEdgeInternal
{
    NavLine Line;    
    u8 Flag;
    u8 AssocDirection[2];    
    u16 AssocCell[2];
};

struct Tile
{
    u32 CellID;
    u16 Flag;
    u16 TextureID;
};

struct JMXNVM
{
    char Signature[12];
    u16 ObjectCount;
    NavMeshObjInst ObjectList[ObjectCount];
    
    u32 TotalCellCount;
    u32 OpenCellCount;
    NavCellQuad QuadCellList[TotalCellCount]; 
    
    u32 GlobalEdgeCount;
    NavEdgeGlobal GlobalEdgeList[GlobalEdgeCount]; 
        
    u32 InternalEdgeCount;
    NavEdgeInternal InternalEdgeList[InternalEdgeCount]; 
    
    Tile TileMap[96*96];
    float HeightMap[97*97];
    u8 PlaneTypeMap[6*6];
    float PlaneHeightMap[6*6];
};

JMXNVM NavMesh @ 0;
```

## See also:
* [[EdgeFlag]]
* [[EdgeDirection]]