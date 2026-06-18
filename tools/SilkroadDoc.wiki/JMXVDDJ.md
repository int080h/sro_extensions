The JMXVDDJ 1000 file describes a container for textures.

```cs
12  string  Signature           //JMXVDDJ 1000
4   int     TextureBufferSize   // in bytes
4   int     TextureType         // 3 = Texture, 4 = VolumeTexture, 5 = CubeTexture (see https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dresourcetype)
*   byte[]  TextureBuffer
```