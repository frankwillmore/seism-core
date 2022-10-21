#!/usr/bin/make 

# defining this here or with make SUBFILNG=-DH5_SUBFILING will cause code to be build with subfiling calls
#SUBFILING:=-DH5_SUBFILING

# if INCLUDE_ZFP is defined, codes will be built assuming ZFP support. Flags should be declared so it links
#ZFP:=-DINCLUDE_ZFP
#ZFP_FLAGS=-lh5zzfp -lzfp 
ZFP_FLAGS=

FEATURES:=$(SUBFILING) $(ZFP)#...

