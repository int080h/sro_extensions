```cs
12  string  signature  // "JMXVCPD 0101"
4   uint    header.CollisionOffset
4   uint    header.ResourceOffset
4   uint    header.unkUInt0
4   uint    header.unkUInt1
4   uint    header.unkUInt2
4   uint    header.unkUInt3
4   uint    header.unkUInt4

4   uint    objInfo.Type                    //see ObjectType
4   uint    objInfo.Name.Length
*   string  objInfo.Name
4   uint    objInfo.unkUInt5
4   uint    objInfo.unkUInt6

//CollisionOffset
4   uint    collisionResourcePath.Length
*   string  collisionResourcePath

//ResourceOffset
4   uint    resourceCnt
for (int i = 0; i < resourceCnt; i++)
{
    4   uint    resourcePath.Length
    *   string  resourcePath
}
```