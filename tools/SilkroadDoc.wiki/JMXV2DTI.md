# 2D Texture Index (2DTI)

_This file is not binary, it's read line by line (like a CSV list)!_

1. Header
2. TileCount
3. TileEntry

**TileEntry**   
> This tables shows the formats of each column.

TileIndex | TileType | TileCategory | TileFile | 3D-Grass
--------- | -------- | ------------ | -------- | -------- 
{0:00000} | 0x{1:X8} | "{2}"        | "{3}"    | {{0},{1}}


***



**3D-Grass:**
> Example: {1159,2} {1816,36}   
> First value indicates the model, the number is an index from object.ifo   
> Second value indicates the amount of grass which is randomly placed on the tile.   
> As seen in this example, multiple grass per tile is possible.

***

**TileType:**
```csharp
public enum TileType: int
{
    Dirt = 0,
    Sand = 1,
    Ashfield = 2,
    Stone = 3,
    Metal = 4,
    Wood = 5,
    Mud = 6,
    Water = 7,
    DeepWater = 8,
    Snow = 9,
    Grass = 10,
    LongGrass = 11,
    Forest = 12,
    Cloud = 13
}
```
