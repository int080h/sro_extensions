[More about PK2 Internals by Drew "pushedx" Benton](http://www.stealthex.org/site/showthread.php?5440-More-about-PK2-Internals)

```csharp
public struct PackFileHeader
{
    30  char[]  Header          //JoyMax File Manager!
    4   uint    Version         //0x02000001
    1   byte    Encrypted
    16  byte[]  Verify          // Used to test the blowfish key
    205 byte[]  reserved        // Unused
}

public struct PackFileBlock
{
    2560  PackFileEntry[] Entries       //128 bytes per entry, a block stores 20 entries
}

public struct PackFileEntry
{
    1   byte    Entry.Type       //0 = Empty, 1 = Folder, 2  = File
    89  char[]  Entry.Name
    8   ulong   CreateTime      // Windows time format
    8   ulong   ModifyTime      // Windows time format
    8   ulong   Position        // Position of data for files, position of children for folders
    4   uint    Size            // Size of files
    8   ulong   NextChain       // Next chain in the current directory
    2   byte[]  Padding         // So blowfish can be used directly on the structure
}
```

### ImHex pattern
```hexpat
struct PackFileHeader
{
    char Signature[30];
    u32 Version;
    bool Encrypted;
    u8 Checksum[16];
    u8 Reserved[205];
};

struct PackFileEntry
{
    u8 Type;
    char Name[89];
    u64 CreateTime;
    u64 ModifyTime;
    u64 Position;
    u32 Size;
    u64 NextChain;
    u16 Padding;
};

struct PackFileBlock
{
    PackFileEntry Entries[20];    
};

PackFileHeader header @ 0;
PackFileBlock rootBlock @ 256;
```