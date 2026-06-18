The JMXVMAPM1000 file format describes the map mesh of a world region.

### File structure
```cs
12  char[]  Signature      //JMXVMAPM1000

for (int zBlock = 0; zBlock < 6; zBlock++)
{
    for (int xBlock = 0; xBlock < 6; xBlock++)
    {
        4   uint    block.Flag              //0 = None, 1 = Culled
        2   ushort  block.EnvironmentID     //see "environment.ifo"

        //every block has 17 * 17 MapMeshVerticies
        for (int z = 0; z < 17; z++)
        {
            for (int x = 0; x < 17; x++)
            {
                4   float   vertex.Height
                
                //    (MSB)                                                                        (LSB)
                //bit | 15  | 14 | 13 | 12 | 11 | 10 | 09 | 08 | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00 |
                //    |             Scale            |                   TextureID                     |
                // TextureID corresponds to ID from tile2d.ifo
                2   ushort  vertex.Texture
                1   byte    vertex.Brightness     //lighting direction indicator?
            }
        }

        1   sbyte   block.WaterType           //-1 = None, 0 = Water, 1 = Ice
        1   byte    block.WaterWaveType       //See Water folder?
        4   float   block.WaterHeight
        
        // every block has 16 * 16 MapMeshTiles
        for (int z = 0; z < 16; z++)
        {
            for (int x = 0; x < 16; x++)
            {
                2   ushort    tile.Flag        // 1 = Blocked manually
            }
        }

        4   float   block.HeightMax           // heighest point including objects
        4   float   block.HeightMin           // lowest point including objects
        20  byte[]  block.reserved            //reserved[0] = 1 in some blocks
    }
}
```

### [ImHex](https://imhex.werwolv.net/) pattern
```hexpat
struct MapVertex
{
    float Height;
    u16 TextureData;
    u8 Brightness;
};

struct MapBlock
{
    u32 Flag;
    u16 EnvironmentID;
    MapVertex VertexMap[17*17];
    u8 WaterType;
    u8 WaterWaveType;
    float WaterHeight;   
    u16 TileMap[16*16];
     
    float HeightMax;
    float HeightMin;
    u8 reserved[20]; 
};

struct JMXVMAPM
{
    char Signature[12];
    MapBlock Blocks[6*6];     
};

JMXVMAPM file @ 0;
```

* [Credits to "The other place" for is initial release.](http://said.thathat.net/2009/07/silkroad-maps-smviewer-project.html)