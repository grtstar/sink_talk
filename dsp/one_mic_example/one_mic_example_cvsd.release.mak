###########################################################
# Makefile generated by xIDE                               
#                                                          
# Project: one_mic_example_cvsd
# Configuration: Release
# Generated: ���� 9�� 24 16:26:17 2023
#                                                          
# WARNING: Do not edit this file. Any changes will be lost 
#          when the project is rebuilt.                    
#                                                          
###########################################################

OUTPUT=one_mic_multitalk_cvsd
OUTDIR=C:/ADK_CSR867x.WIN.4.4.0.21/apps/sink-talk
DEFS=-DKALASM3 -DFRAME_SYNC_DEBUG 
HARDWARE=gordon
BOOTSTRAP=1
LIBS=core frame_sync_debug cbops_multirate_debug plc100 cvc_system spi_comm 
ASMS=\
      one_mic_example_config.asm\
      stream_copy.asm\
      main.asm\
      one_mic_example_system.asm\
      sco_decode2.asm\
      stream_mix.asm
DEBUGTRANSPORT=SPITRANS=USB SPIPORT=0

# Project-specific options
LIB_SET=sdk
debugtransport=[SPITRANS=LPT SPIPORT=1]
separator_flags=0

-include one_mic_example_cvsd.mak
include $(BLUELAB)/Makefile.dsp
