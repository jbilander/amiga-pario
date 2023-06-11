#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>
#include <devices/parallel.h>
#include <resources/misc.h>

#if DEBUG
#include <clib/debug_protos.h>
#endif

// Amiga register addresses
#define PARALLEL_DDR 0xBFE301  // System address of parallel port data direction register
#define PARALLEL_DATA 0xBFE101 // System address of paralled port data register
#define PAR_CTRL_DDR 0xBFD200  // System address of parallel port control line data direction register
#define PAR_CTRL_DATA 0xBFD000 // System address of parallel port control line data register

/*
   CIA-A (ODD) BFEx01 D7-D0 (A12*) (int2)    CIA-B (EVEN) BFDx00 D15-D8 (A13*) (int6)  Name    Function
   ---------------------------------------------------------------------------------------------------------------------
0  $BFE001 : 1011 1111 1110 0000 0000 0001   $BFD000 : 1011 1111 1101 0000 0000 0000   PA      Port register A
1  $BFE101 : 1011 1111 1110 0001 0000 0001   $BFD100 : 1011 1111 1101 0001 0000 0000   PB      Port register B
2  $BFE201 : 1011 1111 1110 0010 0000 0001   $BFD200 : 1011 1111 1101 0010 0000 0000   DDRA    Data direction register A
3  $BFE301 : 1011 1111 1110 0011 0000 0001   $BFD300 : 1011 1111 1101 0011 0000 0000   DDRB    Data direction register B

PB7-PB0 D7-D0   Centronics parallel interface data
PC... strobe*   Centronics control
F.... ack*

PA2.. SEL       Select
PA1.. POUT      Paper out
PA0.. BUSY      Busy

*/

// Amiga register values
#define PARALLEL_DATA_OUTPUT 0xFF // Set parallel data direction to output
#define PARALLEL_DATA_INPUT 0x00  // Set parallel data direction to input
#define PARALLEL_CTRL_OUTPUT 0x07 // Set sel, pout, busy as outputs

#define READ_BUFFER_SIZE 256

extern LONG length(UBYTE *arr, LONG size);   // Returns the array's number of elements until first NULL.
extern void sprintf(UBYTE *a, char *b, ...); // Our own simple sprintf in asm, (string, format, {values}
extern int strcmp(char *a, char *b);         // Our own string compare method

int main(int argc, char *argv[])
{
    struct ExecBase *SysBase;
    struct Library *DOSBase;
    
    struct MsgPort *ParMP;                 // Pointer to reply port
    struct IOExtPar *ParIO;                // Pointer to I/O request
    ULONG waitMask;                        // Collect all signals here
    ULONG temp;                            // Hey, we all need pockets
    UBYTE parReadBuffer[READ_BUFFER_SIZE]; // We buffer the incoming data in this one

#if DEBUG
    char *ddrb = (char *)PARALLEL_DDR; // Pointer to data direction register
    char *ddra = (char *)PAR_CTRL_DDR; // Pointer to control line data direction register
#endif

    SysBase = *(struct ExecBase **)4UL;
    DOSBase = OpenLibrary((CONST_STRPTR) "dos.library", 0);

    if (DOSBase)
    {
        if ((ParMP = CreatePort(0, 0)))
        {
            if ((ParIO = (struct IOExtPar *)CreateExtIO(ParMP, sizeof(struct IOExtPar))))
            {

                UBYTE string[256]; // string used for msg to stdout
                UWORD size = sizeof(string);

                if (OpenDevice((CONST_STRPTR)PARALLELNAME, 0L, (struct IORequest *)ParIO, 0))
                {
                    sprintf(string, "%s did not open\n\0", PARALLELNAME);
                    Write(Output(), string, length(string, size));
                }
                else // Device is opened
                {
                    sprintf(string, "Opened %s\n\0", PARALLELNAME);
                    Write(Output(), string, length(string, size));

                    waitMask = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | 1L << ParMP->mp_SigBit;
                    ParIO->io_ParFlags |= PARF_FASTMODE;

                    if (strcmp("write", argv[1]))
                    {
                        ParIO->IOPar.io_Command = CMD_WRITE;
                        ParIO->IOPar.io_Length = length((UBYTE *)argv[2], size);
                        ParIO->IOPar.io_Data = (APTR)argv[2];
                        SendIO((struct IORequest *)ParIO); // execute write
                    }
                    else if (strcmp("read", argv[1]))
                    {
                        ParIO->IOPar.io_Command = CMD_READ;
                        ParIO->IOPar.io_Length = READ_BUFFER_SIZE;
                        ParIO->IOPar.io_Data = (APTR)&parReadBuffer[0];
                        SendIO((struct IORequest *)ParIO); // execute read
                    }

#if DEBUG
                    KPrintF((CONST_STRPTR) "DDRA: %04lx\n", ddra[0]);
                    KPrintF((CONST_STRPTR) "DDRB: %04lx\n", ddrb[0]);
#endif

                    Write(Output(), "Sleeping until CTRL-C, CTRL-F, or write finish\n", 47);
                    while (1)
                    {
                        temp = Wait(waitMask);
                        Write(Output(), "Just woke up (YAWN!)\n", 21);

                        if (SIGBREAKF_CTRL_C & temp)
                            break;

                        if (CheckIO((struct IORequest *)ParIO)) // If request is complete...
                        {
                            WaitIO((struct IORequest *)ParIO); // clean up and remove reply

                            if (strcmp("write", argv[1]))
                            {
                                sprintf(string, "%ld bytes sent\n\0", ParIO->IOPar.io_Actual);
                                Write(Output(), string, length(string, size));
                            }
                            else if (strcmp("read", argv[1]))
                            {
                                for (UBYTE count = 0; count < READ_BUFFER_SIZE; count++)
                                {
                                    sprintf(string, "%lc [i=%ld]\n\0", parReadBuffer[count], count);
                                    Write(Output(), string, length(string, size));
                                }
                            }

                            break;
                        }
                    }
                    AbortIO((struct IORequest *)ParIO); /* Ask device to abort request, if pending */
                    WaitIO((struct IORequest *)ParIO);  /* Wait for abort, then clean up */
                    CloseDevice((struct IORequest *)ParIO);
                }
                DeleteExtIO((struct IORequest *)ParIO);
            }
            DeletePort(ParMP);
        }
        CloseLibrary(DOSBase);
    }
    return 0;
}

LONG length(UBYTE *arr, LONG size)
{
    for (LONG i = 0; i < size; i++)
    {
        if (arr[i] == NULL)
        {
            return i;
        }
    }
    return size;
}

int strcmp(char *a, char *b)
{
    while (*a != '\0' && *b != '\0')
    {
        if (*a != *b)
        {
            return 0;
        }
        a++;
        b++;
    }
    return (*a == '\0') && (*b == '\0');
}