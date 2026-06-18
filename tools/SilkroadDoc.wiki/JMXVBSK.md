* [Credits to theonly112 for his initial release.](http://www.elitepvpers.com/forum/sro-coding-corner/1992824-wip-silkroad-file-formats-bsr-bms-bmt-bsk-ban.html)   
* [Credits to Cruor for his initial release.](http://www.silkroadforums.com/viewtopic.php?p=1157282#p1157282)

```cs
//CPrimBranch : CPrim, IPrim
12  string  Signature
4   uint    subPrimCount   //number of bones
for (int i = 0; i < subPrimCount; i++)
{
    //CPrimBone : CPrimNode, CSubPrim
    //BoneData
    1   byte    boneType            //0 = CPrimBone, 1 = CPrimDummy
    4   uint    boneName.Length 
    *   string  boneName
    4   uint    parentBoneName.Length
    *   string  parentBoneName

    // Transform relative to parent bone
    16  Quaternion  rotationToParent
    12  Vector3     translationToParent
    
    // Transform relative to world origin
    16  Quaternion  rotationToOrigin
    12  Vector3     translationToOrigin
    
    // Transform relative to local armature
    16  Quaternion  rotationToLocal
    12  Vector3     translationToLocal
    
    4   uint    childBoneCount
    for (int ii = 0; ii < childBoneCount; ii++)
    {
        4   uint    childBoneName.Length
        *   string  childBoneName
    }    
}
4   uint    unkUInt0    //ASSERT(nCount == 0)?
4   uint    unkUInt1    //ASSERT(nCount == 0)?
```

## Sample pictures
![ChinamanSkeleton](http://i.imgur.com/WMFbVfdl.png)
![ChinamanSkeletonHand](http://i.imgur.com/DaOyqpgl.png)
![RocSkeletonBlended](http://i.imgur.com/ggXclA7l.png)

---

## ImHex Pattern (jmxvbsk.hexpat)
Allows for crude editing capabilities using [ImHex](https://github.com/WerWolv/ImHex)
```c
#pragma array_limit 4294967296
#pragma pattern_limit 4294967296

enum BoneType : u8
{
    CPrimBone = 0,
    CPrimDummy = 1
};

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

struct ChildBone
{
    u32 nameLenght;
    char Name[nameLenght];
};

struct Bone
{
    u8 Type;
    u32 nameLenght;
    char Name[nameLenght];
    u32 parentNameLenght;
    char ParentName[parentNameLenght];
    Quaternion RotationFromParent;
    Vector3 TranslationFromParent;
    Quaternion RotationFromOrigin;
    Vector3 TranslationFromOrigin;
    Quaternion RotationFromLocalArmature;
    Vector3 TranslationFromLocalArmature;
    u32 childCount;
    ChildBone Children[childCount];
};

struct JMXVBSK
{
    char Signature[12];
    u32 boneCount;
    Bone Bones[boneCount];
};

JMXVBSK file @ 0;
```