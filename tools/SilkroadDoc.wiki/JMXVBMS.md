## Signature
```cs
12  string  Signature                                   // "JMXVBMS 0110"
```

## Header
```cs
4   uint    header.VertexOffset
4   uint    header.SkinOffset
4   uint    header.FaceOffset
4   uint    header.ClothVertexOffset
4   uint    header.ClothEdgeOffset
4   uint    header.BoundingBoxOffset
4   uint    header.OcclusionPortals
4   uint    header.NavMeshOffset
4   uint    header.SkinedNavMeshOffset                  // NO SAMPLES (only reverse engineered code)
4   uint    header.Unknown9Offset                       // NO SAMPLES (offset leads to unk9Count)
4   uint    header.unkUInt0
4   uint    header.NavFlag                              // 0 = None, 1 = Edge, 2 = Cell, 4 = Event

4   uint    subPrimCount                                // ASSERT(subPrimCount == 1)
4   uint    VertexFlag                                  // Default = 0x000, LightMapUV = 0x400, MorphingData = 0x800
4   uint    unkUInt2                                    // 0 in in all files

4   uint    Name.Length
*   string  Name

4   uint    Material.Length
*   string  Material                                    // name of used Mtrl from active MtrlSet

4   uint    unkUInt3
```

## VertexBuffer (VB)
```cs
// VertexOffset
4   uint vertexCount
for (int i = 0; i < vertexCount; i++)
{
    12  Vector3 vertex.Position
    12  Vector3 vertex.Normal
    8   Vector2 vertex.UV0
    if(VertexFlag & 0x400)
    {
        8   Vector2 vertex.UV1
    }
    if(VertexFlag & 0x800)
    {
        36  byte[]  morphingData    // TODO: Need sample
    }
    
    // Could contains information to stich submeshes into single runtime skinned mesh.
    // Could contains information to create dynamic level of detail (LOD)
    //Outer vertices be : 0, -1, 0
    //Inner vertices be : 0,  X, 2
    4   float   vertex.Float0
    4   uint    vertex.Int0
    4   uint    vertex.Int1
}
if(VertexFlag & 0x400)
{    
    4   uint    LightmapPath.Length
    *   string  LightmapPath                                //.ddj
}
if(VertexFlag & 0x1000) // Found into new models from ISROR
{
    4   uint vertexDataCount
    for (int i = 0; i < vertexDataCount; i++)
    {
        12  Vector3 vertexData.Vector0
        12  Vector3 vertexData.Vector1
    }
}
```

## SkinningData
```cs
// SkinOffset
4  uint    boneCount
if(boneCount > 0)
{
    for (int i = 0; i < boneCount; i++)
    {
        4   uint    BoneName.Length
        *   string  BoneName
    }
    
    for (int i = 0; i < vertexCount; i++)
    {
        1   byte    boneIndex1                              // Index of bone list in .bms!
        2   ushort  boneWeight1                             // Used to weight the two transformation matricies
        1   byte    boneIndex2                              // Index of bone list in .bms!
        2   ushort  boneWeight2                             // Used to weight the two transformation matricies
    }
}
```

## IndexBuffer (IB)
```cs
// FaceOffset
4   uint    faceCount
for (int i = 0; i < faceCount; i++)
{
    2   ushort  face.A                                      // vertexIndex
    2   ushort  face.B                                      // vertexIndex
    2   ushort  face.C                                      // vertexIndex
}   
```

## DyVertexData
```cs
// ClothVertexOffset
4   uint    clothVertexCount                                // 0 or equal to vertexCount
for (int i = 0; i < clothVertexCount; i++)
{
    4   float   clothVertex.MaxDistance                     // maximum distance this vertex can be moved
    4   uint    clothVertex.IsPinned                        // vertex is pinned and no cloth deformation is animated
}

// ClothEdgeOffset
4   uint    clothEdgeCount
if(clothEdgeCount > 0)
{      
    for (int i = 0; i < clothEdgeCount; i++)
    {
        4   uint    clothEdge.A                             // vertexIndex
        4   uint    clothEdge.B                             // vertexIndex
        4   float   clothEdge.MaxDistance                   // maximum distance this edge can be streched or bend
    }
    
    for (int i = 0; i < clothEdgeCount; i++)
    {
        4   uint    clothEdgeIndex                          // index from clothEdgeList matching above for loop
    }
    
    // Cloth simulation parameters
    4   uint    deformationMode                             // 0 = Deformation to wind maybe, 1 = Deformation to ground
    4   float   animationOffsetX
    4   float   animationOffsetZ
    4   float   animationOffsetY
    4   float   clothFallingSpeed
    4   float   unkFloat6
    4   float   unkFloat7
    4   float   clothElasticity   
    4   uint    clothMovementFactor                         // zero is not allowed
}
```

## BoundingBox (AABB)
```cs
// BoundingBoxOffset
24  float[]  AxisAlignedBoundingBox
```

## OcclusionCullingData
```cs
// OcclusionPortal
4   uint    hasOcclusionPortal
if(hasOcclusionPortal == 1)
{
    4   uint    portal.Name.Length
    *   string  portal.Name
    4   uint    vertexCount
    for (int ii = 0; ii < vertexCount; ii++)
    {
        12  Vector3 vertex
    }

    4   uint    faceCount
    for (int ii = 0; ii < faceCount; ii++)
    {
        //PrimMeshFaceTri
        2   ushort  face.A
        2   ushort  face.B
        2   ushort  face.C
    }
}
```

```cs
// Unknown9Offset
4   uint    unk9Count                   //0 in every file :/
```

## NavMeshObj
```cs
//NavMeshOffset
if(header.NavMeshOffset > 0)
{
    //NavVertices
    4   uint    vertexCount
    for (int i = 0; i < vertexCount; i++)
    {
        //PrimMeshNavVertex
        12  Vector3 vertex.Position
        1   byte    vertex.BisectorIndex        // index into bisector cache
    }

    //NavCells (ObjectGround)
    4   uint    collisionCellCount
    for (int i = 0; i < collisionCellCount; i++)
    {
        // RTNavMeshCellTri
        2   ushort  cell.Vertex0            // index into NavVertices
        2   ushort  cell.Vertex1            // index into NavVertices
        2   ushort  cell.Vertex2            // index into NavVertices
        2   ushort  cell.Flag
        
        if(header.NavFlag & 2)
        {
            // (MSB)                               (LSB)
            // | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00 |
            // |   Flag  |         EventIndex          |
            // Flag:
            // 1 = Bit0 -> TriggerOnEnter?
            // 2 = Bit1 -> TriggerOnExit?
            1   byte    cell.EventZoneData            
        }            
    }

    //NavOutlineEdges
    4   uint    outlineLinkCount
    for (int i = 0; i < outlineLinkCount; i++)
    {
        //PrimMeshNavEdge
        2   ushort  edge.SrcVertex
        2   ushort  edge.DstVertex
        2   ushort  edge.SrcCell
        2   ushort  edge.DstCell                          // None = -1
        1   byte    edge.Flag                             //PrimMeshNavEdgeFlag
        if(header.NavFlag & 1)
        {
            // (MSB)                               (LSB)
            // | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00 |
            // |   Flag  |         EventIndex          |
            // Flag:
            // 1 = Bit0 -> TriggerOnEnter?
            // 2 = Bit1 -> TriggerOnExit?
            1   byte    edge.EventZoneData
        }
    }

    //NavInlineEdges
    4   uint    inlineLinkCount
    for (int i = 0; i < inlineLinkCount; i++)
    {
        //PrimMeshNavEdge
        2   ushort  edge.SrcVertex
        2   ushort  edge.DstVertex
        2   ushort  edge.SrcCell
        2   ushort  edge.DstCell
        1   byte    edge.Flag
                
        if(header.NavFlag & 1)
        {
            // (MSB)                               (LSB)
            // | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00 |
            // |   Flag  |         EventIndex          |
            // Flag:
            // 1 = Bit0 -> TriggerOnEnter?
            // 2 = Bit1 -> TriggerOnExit?
            1   byte    edge.EventZoneData
        }
    }

    // Events    
    if(header.NavFlag & 4)
    {
        4   uint    eventCount
        for (int i = 0; i < eventCount; i++)
        {
            //NavCallBack2 related
            4   uint    event.Name.Length
            *   string  event.Name
        }
    }

    // OutlineLookupGrid
    8   Vector2 grid.Origin
    4   uint    grid.Width
    4   uint    grid.Height
    4   uint    grid.CellCount
    for (int i = 0; i < CellCount; i++)
    {        
        4   uint    gridCellOutlineCount
        for (int ii = 0; ii < gridCellOutlineCount; ii++)    
            2   ushort  outlineIndex
    }
}
```

## SkinedNavMesh -> SkinnedNavMesh?!
The client reads this in the the visual section for some reason.
```cs
//SkinnedNavMeshOffset is unknown location within file.
4   int    skinnedCount0
for (int i = 0; i < skinnedCount0; i++)
{
    12  byte[]  skinnedStructure0 // probably a Vector3
}

4   int    skinnedCount1
for (int i = 0; i < skinnedCount1; i++)
{
    2  byte[]  skinnedStructure1 // probably bone weight (byte bone0, byte bone1)
}

4   int    skinnedCount2
for (int i = 0; i < skinnedCount2; i++)
{
    6  byte[]  skinnedStructure2 // probably triangle indices (ushort a, ushort b, ushort c)
}
//EOF
```
* [Credits to theonly112 for is initial release.](http://www.elitepvpers.com/forum/sro-coding-corner/1992824-wip-silkroad-file-formats-bsr-bms-bmt-bsk-ban.html)    
* [Credits to illstar (keinplan1337) for his initial hitbox parsing](http://www.stealthex.org/site/showthread.php?5178-VB.NET-Silkroad-*.bms-hitbox)


# Converted into Wavefront OBJ (*.obj)
![Sample](http://i.imgur.com/8vE36nU.png)

# Collision
![Collision](http://i.imgur.com/136Yz77.png)