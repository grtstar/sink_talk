// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK_CSR867x.WIN. 4.4
//
// *****************************************************************************


#ifndef _STREAM_VAD_INCLUDED
#define _STREAM_VAD_INCLUDED

#include "stream_vad.h"
#ifdef PATCH_LIBS
   #include "patch_library.h"
#endif
#include "cbuffer.h"


// *****************************************************************************
// MODULE:
//    $M.stream_vad
//
// DESCRIPTION:
//    This routine iterates through a frame of samples and detect if exist a voice.
//
// INPUTS:
//    - r7 = Data object
//
// OUTPUTS:
//    if no voice is detected, mute the frame
//
// TRASHED REGISTERS:
//    M1, I3, I0, L0, r0, r3, r4, r10
//
// *****************************************************************************
.MODULE $M.stream_vad;
   .CODESEGMENT   PM;
$stream_vad:
   push rLink;
   r0 = M[r7 + $M.stream_vad.PTR_INPUT_BUFFER_FIELD];     
#ifdef BASE_REGISTER_MODE  
   call $frmbuffer.get_buffer_with_start_address;
   push r2;
   pop  B0;
#else
   call $frmbuffer.get_buffer;
#endif
   I0 = r0;
   L0 = r1;
   I2 = r7;
   
   // Update output frame size from input
   r0 = M[r7 + $M.stream_vad.OFFSET_OUTPUT_PTR];
   call $frmbuffer.set_frame_size;

   // Get output buffer
#ifdef BASE_REGISTER_MODE
   call $frmbuffer.get_buffer_with_start_address;
   push r2;
   pop  B4;
#else
   call $frmbuffer.get_buffer;
#endif
   I4 = r0;
   L4 = r1;
   
  // calc power
  r5 = I0;
  M1 = 1;
  // Set frame size from input.  First Value
  r10 = r3,    r4 = M[I0,M1];   
  r0 = 0;
  do lp_calc_pwr;
    // ABS
    r4 = ABS r4;
    r0 = r0 + r4;     
    r4 = M[I0, M1];
lp_calc_pwr:
   L0 = Null;
   r7 = I2;
   r4 = M[r7 + $M.stream_vad.OFFSET_PTR_POWER];
   null = r0 - r4;
   if NEG jump detect_cnt;
   r7 = I2;
   r4 = M[r7 + $M.stream_vad.OFFSET_PTR_DELAY];
   r6 = M[r4];
   r2 = M[r7 + $M.stream_vad.OFFSET_MESSAGE];
   null = r2 - 0;
   if Z jump set_cnt;
   null = r6 - 0;
   if POS jump set_cnt;
   r7 = I2;
   r6 = M[r7 + $M.stream_vad.OFFSET_POWER_CNT];
   r6 = r6 + 1;
   M[r7 + $M.stream_vad.OFFSET_POWER_CNT] = r6;
   null = r6 - 3;
   if NEG jump detect_cnt;
   r3 = 1;
   call $message.send_short;
set_cnt:
   r0 = 200;
   r7 = I2;
   r4 = M[r7 + $M.stream_vad.OFFSET_PTR_DELAY];
   M[r4] = r0;
   jump copy_out;
detect_cnt:
   r7 = I2;
   r4 = M[r7 + $M.stream_vad.OFFSET_PTR_DELAY];
   r0 = M[r4];
   null = r0;
   if POS jump dec_cnt;
   jump copy_zero;
dec_cnt:   
   r0 = r0 - 1;
   M[r4] = r0;
   r2 = M[r7 + $M.stream_vad.OFFSET_MESSAGE];
   null = r2 - 0;
   if Z jump copy_out;
   null = r0 - 0;
   if NZ jump copy_out;
   r3 = 0;
   call $message.send_short;
   M[r7 + $M.stream_vad.OFFSET_POWER_CNT] = 0;
   jump copy_out;
   
// set output zero
copy_zero:
  r10 = r3;
  do lp_zero;
    r0 = 0;
    M[I4, 1]=r0;
lp_zero:
  jump end;
  
copy_out:
//  r10 = r3;
//  I0 = r5;
//  do lp_copy;
//    r0 = M[I0, 1];
//    M[I4, 1] = r0;
// lp_copy:
   
end:   
#ifdef BASE_REGISTER_MODE  
   push Null;
   pop B0;
#endif
   pop rLink;
   rts;
   
.ENDMODULE;

#endif //_STREAM_VAD_INCLUDED
