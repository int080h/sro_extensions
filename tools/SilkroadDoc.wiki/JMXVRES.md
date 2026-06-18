```cs
12  byte[]  signature                       // JMXVRES 0109

//ResHeader
4   uint    header.MaterialOffset
4   uint    header.MeshOffset
4   uint    header.SkeletonOffset
4   uint    header.AnimationOffset
4   uint    header.PrimMeshGroupOffset
4   uint    header.PrimAniGroupOffset
4   uint    header.ModPaletteOffset
4   uint    header.CollisionOffset
4   uint    header.PrimMeshFlag
4   uint    header.ModDataFlag
4   uint    header.Int2
4   uint    header.Int3
4   uint    header.Int4

//ObjectGeneralInfo
2   ushort  objInfo.Type                    //see ResourceType.cs
2   ushort  objInfo.Category                //see ObjectGeneralCategory.cs
4   uint    objInfo.Name.Length
*   string  objInfo.Name
4   uint    objInfo.unkUInt0
4   uint    objInfo.unkUInt1

40  byte[]  unkBuffer0                      //reserved

//CollisionOffset
4   uint    collisionMesh.Length
*   string  collisionMesh
24  float[] collisionBox0
24  float[] collisionBox1
4   uint    requireCollisionMatrix
if(requireCollisionMatrix)
    64  byte[] collisionMatrix

//MaterialOffset
4   uint    mtrlSetCnt                      //MATERIALSET_MAXCOUNTMAX = 5
for (int i = 0; i < mtrlSetCnt; i++)
{
    //CPrimMtrlSet
    4   uint    iMtrlID // Must be 0-4, matches to SRO_VT_SHARD.dbo._RefObjChar.TextureType
    4   uint    szMtrlPath.Length
    *   string  szMtrlPath
}

//MeshOffset
4   uint    meshCnt
for (int i = 0; i < meshCnt; i++)
{       
    //CPrimMesh
    4   uint    meshPath.Length
    *   string  meshPath
    if(header.PrimMeshFlag & 1)
        4   uint    meshUnkUInt0            // this value is read into memory of CPrimMesh and CResObj remembers that a mesh has this set.
}

//AnimationOffset
4   uint    animationTypeVersion            //ANIMATION_TOOL_VERSION = 0x1000, "Animation Type의 Version이 다릅니다."
4   uint    animationTypeUserDefine         //0, "User Define Animation Type을 사용 하였습니다."
4   uint    animationCnt
for (int i = 0; i < animationCnt; i++)
{       
    //CPrimAnimation
    4   uint    animationPath.Length
    *   string  animationPath
}

//SkeletonOffset (Branch)
4   uint    hasSkeleton
if(hasSkeleton == 1)
{    
    //CPrimBranch
    4   uint    skeletonPath.Length
    *   string  skeletonPath
    
    //CPrimBone
    4   uint    attachmentBone.Length
    *   string  attachmentBone
}

//PrimGroupOffset
4   uint    meshGroupCnt
for (int i = 0; i < meshGroupCnt; i++)
{       
    4   uint    meshGroupName.Length
    *   string  meshGroupName
    4   uint    meshFileCnt
    for (int ii = 0; ii < meshFileCnt; ii++)
        4   uint    meshFileIdx             //from pMeshFileList
}

//PrimAniGroupOffset
4   uint    aniGroupCnt
for (int i = 0; i < aniGroupCnt; i++)
{       
    4   uint    groupName.Length
    *   string  groupName

    //PrimAniTypeData
    4   uint    aniCount
    for (int ii = 0; ii < aniCount; ii++)
    {                
        4   uint    animation.Type          //see ResourceAnimationType
        4   uint    animation.FileIndex

        4   uint    eventCnt
        for (int iii = 0; iii < eventCnt; iii++)
        {
            4   uint    event.KeyTime
            4   uint    event.Type
            4   uint    event.Index
            4   uint    event.unkValue1
        }

        4   uint    walkPointCnt
        4   float   walkLength
        for (int iii = 0; iii < walkPointCnt; iii++)
            8   Vector2 walkGraphPoint
    }
}

//ModPaletteOffset (CModPalette)
//-> SystemModSets
4   uint    modSetCnt
for (int i = 0; i < modSetCnt; i++)
{
    //CModDataSet
    4   uint    modSet.Type                 // Locomotion = 0, Simple = 1, Ambient = 2,
    4   uint    modSet.AnimationType       // from PrimAnimationType
    4   uint    modSet.Name.Length
    *   string  modSet.Name
    
    4   uint    modSetDataCnt
    for (int i = 0; i < modSetCnt; i++)
    {
        4   uint    modDataType             // see ModDataType
        *   byte[]  modData                 // read ModData base on type...
    }
}

//-> AniModSets
4   uint    modSetCnt
for (int i = 0; i < modSetCnt; i++)
{
    //[see above...]
}

//CResAttachable : CResObject, IResObject;
if(objInfo.Type == ObjectType.Character || objInfo.Type == ObjectType.Item) 
{
    4   uint    unkUInt0 //0 = CHAR, 1 = ITEM
    4   uint    unkUInt1 //see below
    4   uint    attachMethod //0 = BASE, 1 = REPLACE, 2 = ADD
    4   uint    slotCount
    for (int i = 0; i < slotCount; i++)
    {
        4   uint    slotId //see below
        4   uint    slotMeshIdx //PrimMeshIdx
    }
    
    //CResChar: CResAttachable, CResObject, IResObject;
    if(objInfo.Type == ObjectType.Character)
        4   uint    nComboNum               //0, "ASSERT(nComboNum == 0)"
}

//CResAttachable.unkUInt1:
//-1 = NONE/Invalid? (for an arrow?!)
//00 = _ha
//01 = _ba (also for avatars?)
//02 = _la
//03 = _fa
//04 = _sa
//05 = _aa
//06 = Left Hand (shield, bow)
//07 = Right Hand (spear, tblade, blade, sword)
//08 = 
//09 =
//10 =
//11 = 
//12 =
//13 = char
//14 =
//15 = 
//16 = attach

//CResAttachable.SlotId:
//00 = Hair
//01 = Face
//02 = torso_upper
//03 = torso_lower
//04 = ??? (override in avatars but never set?)
//05 = arm_upper
//06 = arm_lower
//07 = Left hand (Shield, Bow, Dagger, ...)
//08 =
//09 = Right hand (Blade, TBlade, Crossbow, Axe, ...)
//10 = Spear 
//11 = pelvis
//12 = thigh
//13 = calf
//14 = attach/cape (on the back)?
```

Related pages:
[[ModData]]