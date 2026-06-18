```csharp
12    char[]  signature            // "JMXVMFO 1000"
2     short   MapWidth             // 256 at max. (8 bits from RegionID)
2     short   MapHeight            // 128 at max. (7 bits from RegionID, 1 bit is used as dungeon indicator)
2     short   Short0
2     short   Short1
2     short   Short2
2     short   Short3
8192  byte[]  RegionData     //BitArray: 256 * 256 = 65536 bits / 8 = 8192 bytes (1 byte has 8 region indicators),
                             //Sorted at the same direction from world map.
```

* [Credits to Synx7 for releasing his SRO Mapinfo Editor](http://www.elitepvpers.com/forum/private-sro-exploits-hacks-bots-guides/3417445-release-sro-mapinfo-editor.html)