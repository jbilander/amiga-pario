#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>
#include <devices/parallel.h>
#include <resources/misc.h>
#include <clib/misc_protos.h>

#if DEBUG
#include <clib/debug_protos.h>
#endif

// Amiga register addresses
#define PARALLEL_DDR 0xBFE301  // System address of parallel port data direction register
#define PARALLEL_DATA 0xBFE101 // System address of paralled port data register
#define PAR_CTRL_DDR 0xBFD200  // System address of parallel port control line data direction register
#define PAR_CTRL_DATA 0xBFD000 // System address of parallel port control line data register

// Amiga register values
#define PARALLEL_DATA_OUTPUT 0xFF // Set parallel data direction to output
#define PARALLEL_DATA_INPUT 0x00  // Set parallel data direction to input
#define PARALLEL_CTRL_OUTPUT 0x07 // Set sel, pout, busy as outputs

#define PROGRAM_NAME "Pario"

int main(int argc, char *argv[])
{
    struct ExecBase *SysBase;
    struct Library *MiscBase;

    SysBase = *(struct ExecBase **)4UL;
    MiscBase = OpenResource((CONST_STRPTR)MISCNAME);

    if (MiscBase)
    {
        UBYTE *pport_bits_owner = AllocMiscResource(MR_PARALLELBITS, (CONST_STRPTR)PROGRAM_NAME);
        UBYTE *pport_port_owner = AllocMiscResource(MR_PARALLELPORT, (CONST_STRPTR)PROGRAM_NAME);

        if ((pport_bits_owner == NULL) && (pport_port_owner == NULL))
        {
            // Parallel port was successfully allocated
            Write(Output(), "Parallel port was successfully allocated\n", 41);

            //TODO: implement the file transfer program here.

            FreeMiscResource(MR_PARALLELBITS);
            FreeMiscResource(MR_PARALLELPORT);
        }
        else
        {
            // The parallel port is already taken
            Write(Output(), "Can't allocate parallel port resources!\n", 40);
            Write(Output(), "The port may be in use by another application.\n", 47);
        }
    }

    return 0;
}