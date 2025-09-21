#------------------------------------------------------------
# makefile.def : common template for makefile
# Author       : hhao020
# Date         : 2011-06-30
#------------------------------------------------------------


ifeq ($(PRJ_DIR), )
env_err:
	@echo Error: must define PRJ_DIR environment variable.
endif

ifeq ($(MAKE_RULE), )
env_err:
	@echo Error: must define MAKE_RULE environment variable.
endif


# source files' list
SOURCES = 

# private compiler flags and include file path
SELF_ASFLAGS  = 
SELF_CFLAGS	  =
SELF_CPPFLAGS = 
SELF_INCLUDE  = 



# private macro or compiler conditions definations
SELF_DEFINE = 
#-DNET_SELF_TEST

# private link definations
SELF_LINK_OPT = 



include $(PRJ_DIR)/build/$(MAKE_RULE)
