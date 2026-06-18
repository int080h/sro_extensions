### Header

```cs
//Header
12  char[]  header.Signature
4   uint    header.BlockOffset
4   uint    header.LinkOffset
4   uint    header.GridOffset
4   uint    header.GroupOffset
4   uint    header.LabelOffset
4   uint    header.Offset5                                    // Has been 0 in every file...
4   uint    header.Offset6                                    // Has been 0 in every file...
4   uint    header.BoundingBoxOffset  
```

### Info
```cs
//ObjGeneralInfo
4   uint    objInfo.Type
4   uint    objInfo.Name.Length
*   string  objInfo.Name
4   uint    objInfo.unkUInt0
4   uint    objInfo.unkUInt1

2  ushort   dungeon.RegionID
```

### BoundingBox
```cs
//BoundingBoxOffset
24  float[] dungeon.CollisionBox0
24  float[] dungeon.CollisionBox1 // not used in CRTNavMeshDungeon
```

### BlockList
```cs
//BlockOffset
4   uint    dunBlockCnt
for (int i = 0; i < dunBlockCnt; i++)
{
    4   uint    block.Path.Length
    *   string  block.Path
    
    4   uint    block.Name.Length
    *   string  block.Name    
    
    //field_17
    4   uint    block.unkUInt0
    
    12  Vector3 block.Position
    4   float   block.Yaw
    4   uint    block.IsEntrance            // obsolete?
    24  float[] block.CollisionBox0
    
    //field_18
    4   uint    block.unkUInt1
    
    //FogParamters
    4   uint    block.FogParam.Color
    4   float   block.FogParam.NearPlane
    4   float   block.FogParam.FarPlane
    4   float   block.FogParam.Intensity
    1   byte    block.FogParam.hasHeightFog
    if(block.FogParam.hasHeightFog)            // == 0x01
    {
        4   float   block.FogParam.unkFloat3
        4   float   block.FogParam.unkFloat4
        4   float   block.FogParam.unkFloat5
        4   float   block.FogParam.unkFloat6
    }
    
    1   byte   block.unkByte1
    if(block.unkByte1)                      // == 0x02?
    {
        12  Vector3 block.unkVector0
        12  Vector3 block.unkVector1
        4   uint    block.unkUInt2
    }
    
    4   uint    block.unkString.Length
    *   string  block.unkString             //""

    4   uint    block.RoomIndex
    4   uint    block.FloorIndex
    
    4   uint    iCount                      //iCount < 100
    *   uint[]  block.ConnectedBlockIndices // lists blocks that are connected (walkable) to this block
    
    4   uint    iCount
    *   uint[]  block.VisibleBlockIndices   // lists block that can be seen from this block. (portal based occlusion culling)
    
    4   uint    dwObjCount
    4   uint    dwColObjCount
    for (int ii = 0; ii < objCount; ii++)
    {
        4   uint    obj.Name.Length
        *   string  obj.Name
        
        4   uint    obj.Path.Length
        *   string  obj.Path                // *.bsr
        
        12  Vector3 obj.Position
        12  Vector3 obj.Rotation
        12  Vector3 obj.Scale
        
        4   uint    obj.Flag                //0 = None, 2 = ColObj, 4 = WaterObj
        4   uint    obj.Int0                //
        4   float   obj.RadiusSqrt
        if(obj.Flag & 4)
        {
            4   uint    obj.WaterColor
        }      
    }
    
    4   uint    lightCount
    for (int ii = 0; ii < lightCount; ii++)
    {
        4   uint    light.Name.Length
        *   string  light.Name 
        
        12  Vector3 light.Position
        12  Color3  light.Color0        // Diffuse?
        12  Color3  light.Color1        // Ambient?
        12  Color3  light.Color2        // Specular?

        4   float   light.Float0        // Attenuation?
        4   float   light.Float1
        4   float   light.Float2
    }
}
```

### 3D Block Lookup Grid

Acceleration structure for translating 3D position inside the dungeon to possible dungeon blocks to be tested for containment.
Represents a regular grid in three-dimensonal space. Every voxel of this grid is `200 x 200 x 200` units.
Only voxels with block indices are saved to file.

![image](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/assets/13951844/017e43df-10ec-4210-9c00-0a02439dd7e3)


```cs
//GridOffset
4   uint    gridWidth // Width in Voxels
4   uint    gridHeight // Height in Voxels
4   uint    gridLength // Length in Voxels
4   uint    gridVoxelCount
foreach(gridVoxelCount)
{
    // | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 09 | 08 | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00 |
    // | Reserv. |                     Y-Index                     |                     Z-Index                     |                     X-Index                     |
    4   uint    voxel.ID

    4   uint    voxel.blockCount
    *   uint[]  voxel.blockIndices
}
```

### Links
```cs
//LinkOffset (generate off-mesh links between blocks)
4   uint    linkCount
foreach(linkCount)
{
    4   uint    link.blockCount
    *   uint[]  link.blockIndices          // neighbor blocks
}
```

### Labels
```cs
//LabelOffset
if(LabelOffset != 0)
{
    4   uint    roomCounter
    for (int roomIndex = 0; roomIndex < roomCounter; roomIndex++)
    {
        4   uint    roomName.Length
        *   string  roomName
    }
    4   uint    floorCounter
    for (int floorIndex = 0; floorIndex < floorCounter; floorIndex++)
    {
        2   ushort  floorName.Length
        *   string  floorName
    }
}
```

### Groups
```cs
//GroupOffset
4   uint    blockGroupCount
for (int i = 0; i < blockGroupCount; i++)
{
    4   uint    group.Name.Length
    *   string  group.Name
    4   uint    group.Flag                                 //0 or 1 -> Service?
    4   uint    group.blockCount
    4   uint[]  group.BlockIndices
}

//EOF
```