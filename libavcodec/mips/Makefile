MIPSFPU-OBJS-$(CONFIG_AMRNB_DECODER)      += mips/acelp_filters_mips.o     \
                                             mips/celp_filters_mips.o      \
                                             mips/celp_math_mips.o         \
                                             mips/acelp_vectors_mips.o
MIPSFPU-OBJS-$(CONFIG_AMRWB_DECODER)      += mips/acelp_filters_mips.o     \
                                             mips/celp_filters_mips.o      \
                                             mips/amrwbdec_mips.o          \
                                             mips/celp_math_mips.o         \
                                             mips/acelp_vectors_mips.o
MIPSFPU-OBJS-$(CONFIG_MPEGAUDIODSP)       += mips/mpegaudiodsp_mips_float.o
MIPSDSPR1-OBJS-$(CONFIG_MPEGAUDIODSP)     += mips/mpegaudiodsp_mips_fixed.o
MIPSFPU-OBJS-$(CONFIG_FFT)                += mips/fft_float_mips.o
OBJS-$(CONFIG_FFT)                        += mips/fft_fixed32_mips.o
OBJS                                      += mips/fmtconvert_mips.o
OBJS-$(CONFIG_AC3DSP)                     += mips/ac3dsp_mips.o
OBJS-$(CONFIG_AAC_DECODER)                += mips/aacsbr_mips.o            \
                                             mips/sbrdsp_mips.o            \
                                             mips/aacpsdsp_mips.o          \
                                             mips/aacdec_mips.o            \
                                             mips/aacdec_fixed_mips.o
MIPSDSPR1-OBJS-$(CONFIG_AAC_ENCODER)      += mips/aaccoder_mips.o
MIPSFPU-OBJS-$(CONFIG_AAC_ENCODER)        += mips/iirfilter_mips.o

ifdef CONFIG_HARDCODED_TABLES
$(SUBDIR)mips/aacdec_mips.o: $(SUBDIR)cbrt_fixed_tables.h
$(SUBDIR)mips/aacdec_fixed_mips.o: $(SUBDIR)cbrt_fixed_tables.h
endif
