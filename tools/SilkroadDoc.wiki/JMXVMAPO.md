The JMXVMAPO1001 file format describes placement of object instances in a world region.

## *.o
```cs
12  char[]  Signature       //JMXVMAPO1001

for (int zBlock = 0; zBlock < 6; zBlock++)
{
    for (int xBlock = 0; xBlock < 6; xBlock++)
    {
        2   ushort  mapObjInfoCount
        foreach(mapObjInfo)
        {
            4   uint    mapObjInfo.ObjID            // from m_sObjectInfo (object.ifo)
            12  Vector3 mapObjInfo.LocalPosition    // relative to the region
            2   short   mapObjInfo.IsStatic         //0 = No, 0xFFFF = Yes
            4   float   mapObjInfo.Yaw
            2   short   mapObjInfo.UID              // Unique Id to track the same objects on multiple blocks
            2   short   mapObjInfo.Short0
            1   byte    mapObjInfo.IsBig            // exceeds region size
            1   byte    mapObjInfo.IsStruct         // has "objectstring.ifo"
        }
        
    }
}
```

## *.o2
```cs
12  char[]  Signature       //JMXVMAPO1001

for (int zBlock = 0; zBlock < 6; zBlock++)
{
    for (int xBlock = 0; xBlock < 6; xBlock++)
    {
        // Objects can appear in multiple blocks but should be unique per LoD (Level of Details) group.
        // More level means fading quickly on distance.
        for (int lodGroupIndex = 0; lodGroupIndex < 4; lodGroupIndex++)
        {
            2   ushort  mapObjInfoCount
            foreach(mapObjInfo)
            {
                4   uint    mapObjInfo.ObjID            // from m_sObjectInfo (object.ifo)
                12  Vector3 mapObjInfo.LocalPosition    // relative to the region
                2   short   mapObjInfo.IsStatic         //0 = No, 0xFFFF = Yes
                4   float   mapObjInfo.Yaw
                2   short   mapObjInfo.UID              // Unique Id to track the same objects on multiple blocks
                2   short   mapObjInfo.Short0
                1   byte    mapObjInfo.IsBig            // exceeds region size
                1   byte    mapObjInfo.IsStruct         // has "objectstring.ifo"

                // (MSB)                                                                       (LSB)
                // | 15 | 14 | 13 | 12 | 11 | 10 | 09 | 08 | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00 |
                // | DB |                                                                          |
                // |               Z (0-255)               |              X (0 - 255)              |
                // DB = DungeonBit
                2   ushort  mapObjInfo.RegionID
            }
        }
    }
}
```

[Credits to "The other place" for is initial release.](http://said.thathat.net/2009/07/silkroad-maps-smviewer-project.html)