#!/usr/bin/make 

# defining this here or with make SUBFILNG=-DH5_SUBFILING will cause code to be build with subfiling calls
SUBFILING:=-DH5_SUBFILING

# define any other features here... 

FEATURES:=$(SUBFILING) #...

