JMXVMAPT represents a baked terrain lightmap. It also includes a simplified per tile lightMap that is used to determine if dynamic objects are in shadow.

```cs
12      byte[]  signature            // "JMXVMAPT1001"
9216    byte[]  lightMap             // per tile lighting [96 * 96 = 9216 ::: 96 * 20 = 1920 units]
4       int     lightMapBufferSize   // in bytes
4       int     lightMapTextureType  // 3 = Texture, 4 = VolumeTexture, 5 = CubeTexture (see https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dresourcetype)
*       byte[]  lightMapBuffer       // TextureBuffer
```

# ImHex pattern
```hexpat
#pragma array_limit 4294967296
//#include "dds.hexpat"

struct JMXVMAPT
{
char Signature[12];
u8 lightMap[96*96];
u32 lightMapBufferSize;
u32 lightMapTextureType;
u8 lightMapBuffer[lightMapBufferSize]; //DDS
};

JMXVMAPT file @ 0;
```


### Remarks
lightMap is used to determine whether or not dynamic shadow casters should cast or be rendered with shadow tone.

![ShadowTexture](http://i.imgur.com/HJSJ6yA.png)
![Minimap_ShadowApplied](http://i.imgur.com/xh1p2zr.png)
> Probably flipped or rotated incorrectly in these images.