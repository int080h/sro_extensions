import sys
from minidump.minidumpfile import MinidumpFile

def main():
    if len(sys.argv) < 2:
        print("Usage: python parse_with_minidump.py <dmp_file>")
        return

    dmp_path = sys.argv[1]
    
    # parse catches __parse_peb internally
    dmp = MinidumpFile.parse(dmp_path)

    print(f"--- Minidump Info: {dmp_path} ---")
    
    # System Info
    if dmp.sysinfo:
        print("\n[System Info]")
        print(dmp.sysinfo)

    # Exception Info
    if dmp.exception:
        print("\n[Exception Info]")
        for exc in dmp.exception.exception_records:
            print(f"Thread ID: 0x{exc.ThreadId:X} ({exc.ThreadId})")
            print("Exception Record Fields:")
            for k, v in exc.__dict__.items():
                if k == 'ExceptionAddress':
                    print(f"  {k}: 0x{v:08X}")
                elif k == 'ExceptionCode_raw':
                    print(f"  {k}: 0x{v:08X}")
                elif k == 'ExceptionInformation':
                    params = ", ".join(f"0x{x:08X}" for x in v)
                    print(f"  {k}: [{params}]")
                else:
                    print(f"  {k}: {v}")
            
            # Print faulting thread registers
            if dmp.threads:
                for thread in dmp.threads.threads:
                    if thread.ThreadId == exc.ThreadId:
                        print(f"\n[Registers for faulting thread 0x{thread.ThreadId:X}]")
                        ctx = thread.ContextObject
                        if ctx:
                            print(ctx)

    # Modules
    if dmp.modules:
        print("\n[Modules]")
        for module in dmp.modules.modules:
            print(f"0x{module.baseaddress:08X} - 0x{module.baseaddress + module.size:08X} : {module.name}")

if __name__ == "__main__":
    main()
