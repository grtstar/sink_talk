// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK_CSR867x.WIN. 4.4
//
// *****************************************************************************


#ifndef _STREAM_PWR_INCLUDED
#define _STREAM_PWR_INCLUDED

#include "stream_pwr.h"
#ifdef PATCH_LIBS
   #include "patch_library.h"
#endif
#include "cbuffer.h"


// *****************************************************************************
// MODULE:
//    $M.stream_pwr
//
// DESCRIPTION:
//    This routine iterates through a frame of samples and finds and stores the
//    power magnitude.
//
// INPUTS:
//    - r7 = Data object
//
// OUTPUTS:
//    The power value is written into the data object.
//
// TRASHED REGISTERS:
//    M1, I3, I0, L0, r0, r3, r4, r10
//
// *****************************************************************************
.MODULE $M.stream_pwr;

   .CODESEGMENT   PM;

func:
   push rLink;
   r0 = M[r7 + $M.stream_pwr.PTR_INPUT_BUFFER_FIELD];     
#ifdef BASE_REGISTER_MODE  
   call $frmbuffer.get_buffer_with_start_address;
   push r2;
   pop  B0;
#else
   call $frmbuffer.get_buffer;
#endif
   I0 = r0;
   L0 = r1;
   pop rLink;
   
   M1 = 1;
   // Set frame size from input.  First Value
   r10 = r3,    r4 = M[I0,M1]; 
   
   r3 = 0;                                   
   do lp_calc_pwr;
      // ABS
      r4 = ABS r4;
      r3 = r3 + r4;     
      r4 = M[I0, M1];
lp_calc_pwr:
   L0 = Null;
   M[r7 + $M.stream_pwr.POWER_VALUE] = r3;  
   
#ifdef BASE_REGISTER_MODE  
   push Null;
   pop B0;
#endif
   rts;
   
.ENDMODULE;

#endif //_STREAM_PWR_INCLUDED
