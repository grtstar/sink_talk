// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK_CSR867x.WIN. 4.4
//
// *****************************************************************************


// *****************************************************************************
// MODULE:
//    $M.stream_mix 
//
// DESCRIPTION:
//    This function mixes two streams together using a weighted sum.
//
// INPUTS:
//    r7 - Pointer to the stream mixer data structure
//
// OUTPUTS:
//    none

// *****************************************************************************

#include "stream_mix.h"

.MODULE $M.stream_pwr;
    .CODESEGMENT PM;
    $stream_pwr:
    //push rLink;
//    push I0;
    // r0 = buf ptr
    // r1 = length
//    r10 = r1;
//    I0 = r0;
//    do calc_pwr;
//    r0 = M[I0, 1];
//    r7 = ABS r0;
//    r8 = r8 + r7;
//calc_pwr:
//    null = r8 - 20000;
//    r0 = 0;
//    if NEG jump pwr_get;
    r0 = 1;
//pwr_get:
//    pop I0;
    //pop rLink;
    rts;
.ENDMODULE;

.MODULE $M.stream_mix;
    .DATASEGMENT DM;
    .VAR $in1_count = 0;
    .VAR $in2_count = 0;
    .CODESEGMENT PM;

$stream_mix:
   // Pointer to the stream copy data struc
   I2 = r7;    
   M1 = 1;
   
   push rLink;  
   // Get Input Buffers
   r0 = M[I2,M1];                   // OFFSET_INPUT_CH1_PTR
#ifdef BASE_REGISTER_MODE  
   call $frmbuffer.get_buffer_with_start_address;
   push r2;
   pop  B0;
#else
   call $frmbuffer.get_buffer;
#endif
   // r0 = buf ptr
   // r1 = circ buf length
   // r2 = bbuffer base address <base variant only>
   // r3 = frame size
   I0  = r0;
   L0  = r1,    r0 = M[I2,M1];       // OFFSET_INPUT_CH2_PTR
#ifdef BASE_REGISTER_MODE  
   call $frmbuffer.get_buffer_with_start_address;
   push r2;
   pop  B1;
#else
   call $frmbuffer.get_buffer;
#endif
   I1  = r0;
   L1  = r1;
   
   // Use input frame size
   r10 = r3,    r0 = M[I2,M1];       // OFFSET_OUTPUT_PTR
   // Update output frame size from input
   call $frmbuffer.set_frame_size;
   
   // Get output buffer
#ifdef BASE_REGISTER_MODE  
   call $frmbuffer.get_buffer_with_start_address;
   push r2;
   pop  B4;
#else
   call $frmbuffer.get_buffer;
#endif
   I4 = r0,    r4 = M[I2,M1];       // OFFSET_PTR_CH1_MANTISSA
   L4 = r1,    r5 = M[I2,M1];       // OFFSET_PTR_CH2_MANTISSA
   pop rLink;
                                   
   r4 = M[r4], r2 = M[I2,M1];       // OFFSET_PTR_EXPONENT                   
   // r0 = first input ch1                      
   r5 = M[r5] ;            
   // r1 = first input ch2
   r6 = M[r2];
   
//   r7 = ABS r0; 
//   null = r7 - 100;
//   if POS jump copy;
//   r0 = 0;
   // Sum & Copy
   r9 = I0;
   r8 = 0;
   r10 = r3;
   do calc_pwr1;
   r0 = M[I0, 1];
   r7 = ABS r0;
   r8 = r8 + r7;
calc_pwr1:  
   I0 = r9;
   null = r8 - 2500000;
   if NEG jump pwr2;
   r0 = 500;
   M[$in1_count] = r0;
pwr2:   
   r9 = I1;
   r8 = 0;
   r10 = r3;
   do calc_pwr2;
   r0 = M[I1, 1];
   r7 = ABS r0;
   r8 = r8 + r7;
calc_pwr2:
   I1 = r9;
   null = r8 - 2500000;
   if NEG jump check_cnt1;
   r0 = 500;
   M[$in2_count] = r0;   
check_cnt1:
   null = M[$in1_count];
   if POS jump dec_cnt1;
   r4 = 0;
   jump check_cnt2;
dec_cnt1:
   r0 = M[$in1_count];
   r0 = r0 - 1;
   M[$in1_count] = r0;
check_cnt2:
   null = M[$in2_count];
   if POS jump dec_cnt2;
   r5 = 0;
   jump copy;
dec_cnt2:
   r0 = M[$in2_count];
   r0 = r0 - 1;
   M[$in2_count] = r0; 
copy:
   r10 = r3;
   do lp_stream_copy;
      // CH1 * CH1_Mantisa   
      r0 = M[I0,M1];
      rMAC = r0 * r4;   
      // CH2 * CH2_Mantisa 
      r1 = M[I1,M1];
      rMAC = rMAC + r1 * r5;  
      // Shift for exponent
      r2 = rMAC ASHIFT r6;             
      // Save Output (r2)
      M[I4,1] = r2;                     
lp_stream_copy:
              
   L0 = 0;
   L1 = 0;
   L4 = 0;  
#ifdef BASE_REGISTER_MODE  
   push Null;
   B4 = M[SP-1];
   B1 = M[SP-1];
   pop  B0;
#endif 
   rts;
.ENDMODULE;

