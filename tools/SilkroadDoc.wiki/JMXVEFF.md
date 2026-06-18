# EFStoredEffect
```cs
8   string  format  // "JMXVEFF "
4   string  version // "0011", "0012", "0013"
if(version == "0012" || version == "0013")
{
    4   float rootScale
}
if(version == "0013")
{
    4   int version13Int0
    4   int version13Int1
    4   int version13Int2
}
* EFStoredObject rootObject
// EOF
```

# EFStoredObject
```cs
4   int dataOffset
*   string  Name

// Controller
4   int     controllerCount
foreach(controller)
{
    *   string  controllerName
    *   EFController    controller  // based on controllerName
}

//EEGlobalData
*   EEGlobalData    globalData              // "BlendScaleGraph", "BlendDiffuseGraph", "BSAnimation"
*   EESourceList    emptySourceList0
*   EESourceList    emitterSourceList       // "StaticEmit"
*   EESourceList    emptySourceList2
*   EESource        lifeTimeSource          // "NeverExtinct", "NormalTimeExtinct", "NormalTimeLoop"
*   EESourceList    programSourceList       // "ProgramUpdate"

1   byte    byte0
1   byte    byte1
4   int     int0    // start?
4   int     int1    // end
4   int     int2

1   byte    byte2
4   int     int3    // AttachToParent?
1   byte    byte3

*   EESource        viewModeSource      // "ViewNone", "ViewBillboard", ViewVBillboard", "ViewYBillboard"
*   EEResource      resource
*   EESource        renderSource        // "RenderNone", "RenderPlate", RenderMesh", "RenderLinkDPipe", "RenderLinkPipe", 
*   EESourceList    emptySourceList3
*   EESourceList    renderSourceList

// LinkedObjects
4   childObjectCount
foreach(childObject)
{
    * EFStoredObject childObject
}
```

# EE Objects
## EEBlend\<TValue\>
```cs
4   float   begin
4   float   end
4   int     blendCount
foreach(blend)
{
    4   float   time
    *   TValue  value
}
```

## EEGlobalData
```cs
4   int Int0
4   int parameterCount
foreach(parameter)
{
    *   string  parameterName
    *   EEParameter parameter   // based on parameterName
}
```

## EESourceList
```cs
4   int sourceCount
foreach(source)
{
    *   EESourceData    sourceData
}
```

## EESource
```cs
*   EESourceData    sourceData
```

## EESourceData
```cs
1   bool    hasData
if(hasData)
{
    *   string      commandName
    1   byte        byte0
    1   byte        byte1
    4   float       start
    4   float       end
    4   float       float2
    *   EEParameter parameter   // see ParameterByCommandNameTable
}
```

## EEResource
```cs
4   int TwoSided // != 0
4   int SrcBlend // https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dblend
4   int DstBlend // https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dblend
4   int SrcTextureArg1 // https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dta
4   int SrcTextureArg2 // https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dta
4   int SrcTextureOP // https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dtextureop
4   int DstTextureArg1 // https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dta
4   int DstTextureArg2 // https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dta
4   int DstTextureOP    // https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dtextureop

4   int meshCount
foreach(mesh)
{
    *   string  meshPath    // *.bms
    4   int textureCount
    foreach(texture)
    {
        *   string  texturePath // *.ddj    
    }
}
```

# EFController
## EFCNormalTimeLife
```cs
// Empty
```

## EFCNormalTimeLoopLife
```cs
// Empty
```

## EFCStaticEmit
```cs
 // see EFStaticEmit
```

## EFCProgram
```cs
// see EEStaticProgram
```

## EFCLinkMode
```cs
4   int Int0
4   int Int1
4   int Int2
4   int Int3
```

## EFCBAN
```cs
// see BSAnimation
```

## EFCViewMode
```cs
*   string  viewMode    // "ViewNone", "ViewBillboard", ViewVBillboard", "ViewYBillboard"
```

## EFCShape
```cs
*   string      renderShape    // "RenderNone", "RenderPlate", RenderMesh", "RenderLinkDPipe", "RenderLinkPipe", "RenderLinkObj"
*   EEResource  resource
```

## EFCScaleGraph
```cs
*   EEBlend<float>  blendScaleX;
*   EEBlend<float>  blendScaleY;
*   EEBlend<float>  blendScaleZ;
4   float   float0
4   float   float1
```

## EFCDiffuseGraph
```cs
*   EEBlend<byte>   blendByte0;
*   EEBlend<Color>  blendColor; // ARGB32
4   float           float0
4   float           float1
```

# EEParameter

## float
```cs
4   float   value
```

## Vector
```cs
12  Vector3 value
```

## Matrix
```cs
64  Matrix4x4   value
```

## EFStaticEmit
```cs
4   int     Min
4   int     Max
4   int     BurstRate
4   float   SpawnRate
```

## AxisVector4
https://en.wikipedia.org/wiki/Axis%E2%80%93angle_representation
```cs
//ConvertParameter
16  Vector4     left    // XYZ = Unit vector; W = Angle in degrees
64  Matrix4x4   right   // left represented as Matrix.
```

## RotVector
https://en.wikipedia.org/wiki/Euler_angles
```cs
//ConvertParameter
12  Vector3     left    // X = Pitch, Y = Yaw, Z = Roll all in degrees
64  Matrix4x4   right   // left represented as Matrix.
```

## AngleVector1
```cs
//ConvertParameter
12  Vector3     left    // XY = Unit vector; Z = Angle in degrees
12  Vector3     right   // XY = Unit vector; Z = Angle in radians
```

## FrameScale
```cs
4   int scaleCount
foreach(scale)
{
    12  Vector3 scale
}
```

## BlendScaleGraph
```cs
*   EEBlend<Vector3> value;
```

## BlendScaleGraphPointer
```cs
4   int   value  // eh? CEEBlend<Vector,defblend<Vector>> *
```

## FrameDiffuse
```cs
4   int colorCount
foreach(color)
{
    4   Color   color   //ARGB32
}
```

## BlendDiffuseGraph
```cs
*   EEBlend<Color> value;  
```

## FrameBANRotation
```cs
4   int matrixCount
foreach(matrix)
{
    64   Matrix4x4   matrix
}
```

## FrameBANPosition
```cs
4   int vectorCount
foreach(vector)
{
    12   Vector3   vector
}
```

## BSAnimation
```cs
4   int primAnimationCount
foreach(primAnimation)
{
    *   string primAnimationPath    
}
```

## FrameTextureSlide
```cs
12  Vector3 Left
4   rightCount
foreach(rightValue)
{
    16  Vector4 rightValue
}
```

# Commands
## **Basic**
#### NormalTimeLife
#### NormalTimeLoopLife
#### StaticEmit
#
## **Position**
#### LinkMode
#### BAN
#
## **Decoration**
#### ViewMode
#### Shape
#### ScaleGraph
#### DiffuseGraph	
#
## **Program**
### **ProgramPosition**
#### Attraction
#### ConeForce
#### Force
#### SetConeVel
#### SetVelocity
#### SetSpherePos
#### SetBANPos
#### SetConePos
#### SetPosition
#### SetRVelocity
#### SetBANRot
#### SetRotation
#
### **ProgramDecoration**
#### SetShapeRotVel
#### SetShapeRot
#### TextureSlide
#### SetGraphDiffuse
#### SetGraphRandomScale
#### SetGraphScale

---

## ParameterByCommandName
| Command | Parameter
| --------| --------- 
NeverExtinct | -
NormalTimeExtinct | -
NormalTimeLoop | -
StaticEmit | EFStaticEmit
ProgramUpdate | -
SetRotationMat | Matrix
SetRVelocityMat | Matrix
SetRotation | RotVector
SetBANRot | FrameBANRotation
SetRVelocity | RotVector
SetRotationAxis | AxisVector4
SetRVelocityAxis | AxisVector4
SetPosition | Vector
SetConePos | AngleVector1
SetBANPos |  FrameBANPosition
SetSpherePos | Vector
SetVelocity | Vector
SetConeVel | AngleVector1
Force | Vector
ConeForce | AngleVector1
Attraction | float
TextureSlide | FrameTextureSlide
SetGraphScale | FrameScale
SetGraphRandomScale | BlendScaleGraphPointer
SetGraphDiffuse | FrameDiffuse
SetShapeRot | AxisVector4
SetShapeRotVel | AxisVector4
ViewNone | -
ViewBillboard | -
ViewYBillboard | -
ViewVBillboard | -
RenderNone | -
RenderPlate | -
RenderMesh | -
RenderLinkDPipe | -
RenderLinkPipe | -
RenderLinkObj | -

---

Credits to [Aurus77](https://www.elitepvpers.com/forum/members/8066198-aurus77.html) for providing his findings to me.