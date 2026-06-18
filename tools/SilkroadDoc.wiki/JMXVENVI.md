JMXVENVI1003 represents the color grading / post-processing effects over the course of the day.

```cs
12  char[]  Signature                       // "JMXVENVI1003"

2   short   profileCount
4   uint    environmentSetName.Length
*   string  environmentSetName              //""

for (int i = 0; i < profileCount; i++)
{
    2   ushort      profile.Id
    4   uint        profile.Name.Length
    *   string      profile.Name
    
    4   uint        profile.DayBGM.Length
    *   string      profile.DayBGM                 // obsolete; handled by regioninfo.txt & effectenvsnd.txt
    4   uint        profile.NightBGM.Length
    *   string      profile.NightBGM               // obsolete; handled by regioninfo.txt & effectenvsnd.txt
    
    *   ColorGraph  profile.SunColor
    *   ColorGraph  profile.SkyTopColor
    *   ColorGraph  profile.DiffuseColor
    *   ColorGraph  profile.ObjectAmbientColor
    *   ColorGraph  profile.Graph4
    *   ColorGraph  profile.TerrainAmbientColor
    *   ColorGraph  profile.TerrainShadowColor  //added in fileVersion 1003
    *   FloatGraph  profile.FogNearPlane
    *   FloatGraph  profile.FogFarPlane
    *   ColorGraph  profile.FogColor
    *   FloatGraph  profile.Graph10
    *   FloatGraph  profile.Graph11
    *   FloatGraph  profile.Graph12         //added in fileVersion 1001
    *   ColorGraph  profile.SkyBottomColor  //added in fileVersion 1002
    *   ColorGraph  profile.WaterColor      //added in fileVersion 1002
    *   FloatGraph  profile.Graph15         //added in fileVersion 1002
}
*   Environment environmentRoot             //client reads this hardcoded as 3 nested loops
//EOF
```

```cs
struct Environment
{
    4   uint    childCount
    
    4   uint    Name.Length
    *   string  Name

    // Client reads this as an int tho Short0 is not 0.
    2   short   ProfileId
    2   short   Short0

    4   int     Int0
    4   int     int1

    *   Environment[] Children
}
```

```cs
struct ColorGraph
{
    4   int        keyCount
    *   Vector4[]   keys                    //xyz = Color (RGB), w = time
}
```

```cs
struct FloatGraph
{
    4   int        keyCount
    *   Vector2[]   keys                    //x = Value, y = time
}
```

![Visualization](https://i.imgur.com/bw69qjo.png)
> Some of the colors could be missing in this preview. Unity Gradient only supports 8 color keys.