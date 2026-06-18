```csharp
12  string  Signature           //"JMXVBAN 0102"
4   int     Int0                //Introduced in JMXVBAN 0102, 0 in all files
4   int     Int1                //Introduced in JMXVBAN 0102, 0 in all files
4   int     Name.Length
*   string  Name
4   int     Duration            // in ms
4   int     FramesPerSecond
4   int     Type                // 0 = OneShot, 1 = Cyclic

//Timings of the keyframes, so you can interpolate between two poses.
4   uint    keyframeTimeCount
foreach(keyframeTime)
{
    4   uint    keyframeTime    // in ms
}

//Amount of bones that have transformations, that are different from their bind poses.
4   uint    animatedBoneCount
foreach(animatedBone)
{
    4   uint    boneName.Length
    *   string  boneName    
    4   uint    keyframeCount
    foreach(keyframe)
    {
        //These two together give you the transformation Matrix relative to it's parent bone/joint.
        16  Quaternion  Rotation
        12  Vector3     Translation
    }
}
//EOF
```

![Animation](http://i.imgur.com/UdjYFlt.gif)

---

## ImHex Pattern (jmxvban.hexpat)
Allows for crude editing capabilities using [ImHex](https://github.com/WerWolv/ImHex)
```c
#pragma array_limit 4294967296
#pragma pattern_limit 4294967296

struct Quaternion
{
    float X;
    float Y;
    float Z;
    float W;
};

struct Vector3
{
    float X;
    float Y;
    float Z;
};

struct Keyframe
{
    Quaternion Rotation;
    Vector3 Translation;
};

struct AnimatedBone
{
    u32 boneNameLenght;
    char BoneName[boneNameLenght];
    u32 keyframeIndices;
    Keyframe Keyframes[keyframeIndices];
};

struct JMXVBAN
{
    char Signature[12];
    u32 Int0;
    u32 Int1;
    u32 NameLenght;
    char Name[NameLenght];
    u32 Duration;
    u32 FramesPerSsecond;
    u32 Type;
    u32 keyframesCount;
    u32 KeyframesTimings[keyframesCount];
    u32 animatedBonesCount;
    AnimatedBone AnimatedBones[animatedBonesCount];
};

JMXVBAN file @ 0;
```

* [Credits to theonly112 for is initial release.](http://www.elitepvpers.com/forum/sro-coding-corner/1992824-wip-silkroad-file-formats-bsr-bms-bmt-bsk-ban.html)