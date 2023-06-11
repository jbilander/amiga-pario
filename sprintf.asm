;
; Simple version of the C "sprintf" function.  Assumes C-style
; stack-based function conventions.
;

section  code       ,code

_AbsExecBase     EQU $0004
_LVOOpenLibrary  EQU -552
_LVOCloseLibrary EQU -414
_LVORawDoFmt     EQU -522

_sprintf:                              ;( ostring, format, {values} )
         movem.l    a2/a3/a6,-(sp)
         move.l     4*4(sp),a3          ;Get the output string pointer
         move.l     5*4(sp),a0          ;Get the FormatString pointer
         lea.l      6*4(sp),a1          ;Get the pointer to the DataStream
         lea.l      stuffChar(pc),a2
         move.l     _AbsExecBase,a6
         jsr        _LVORawDoFmt(a6)
         movem.l    (sp)+,a2/a3/a6
         rts
;------ PutChProc function used by RawDoFmt -----------
stuffChar:
         move.b     d0,(a3)+
         rts
