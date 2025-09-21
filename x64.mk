# programs for cshell

MAKE_RULE := linux.i64

PRJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))


export PRJ_DIR MAKE_RULE


CC := gcc
LD := gcc

CFLAGS := -Wall -g -MD -m64 -c
SELF_CFLAGS :=
LDFLAGS :=

TARGET := target_b
SOURCES := $(wildcard $(TARGET)/*.s) $(wildcard $(TARGET)/*.c) $(wildcard $(TARGET)/*.cpp) 
BIN = $(PRJ_DIR)/bin/$(MAKE_RULE)
PATHS = $(addprefix $(BIN)/, $(dir $(SOURCES)))

OUT=$(PRJ_DIR)bin/$(TARGET).$(MAKE_RULE).exe

OBJECTS := $(addprefix $(BIN)/, $(addsuffix .o, $(basename $(SOURCES))))

#must keep cshell as the first lib
LIBDIRS := cshell userapp sal_linux zlib util


LDDIRS := -L$(BIN)

LDLIBS := $(addprefix -l, $(LIBDIRS))

LIBFILES := $(addprefix $(BIN)/lib, $(addsuffix .a, $(LIBDIRS)))

LDFLAGS += -m64 $(LDLIBS) $(LDDIRS) -lpthread 

INCLIBS =  -I$(PRJ_DIR)export $(addsuffix /export, $(addprefix -I$(PRJ_DIR), $(LIBDIRS)))
export INCLIBS

.PHONY: all clean rebuild veryclean

all: yacc $(BIN) exe
	
	
#$(OBJECTS): | $(BIN) yacc

$(BIN):
	mkdir -p $(PATHS)
	
yacc:
ifeq ($(target), )
	#( cd cshell && (perl $(PRJ_DIR)/tools/p_readelf.pl ))
	( cd cshell && (bison -d cshell.y && flex -ocshell.lex.c cshell.l && perl $(PRJ_DIR)/tools/p_readelf.pl ))
endif

-include $(OBJECTS:.o=.d)

exe: $(OBJECTS)
ifneq ($(target), )
	make -C $(target) TARGET=$(target)
else	
	for dir in $(LIBDIRS); do (make -C $$dir TARGET=$$dir || exit 1) || exit 1; done		
	(cd cshell && rm -rf c_sym_table.* bin/$(MAKE_RULE) && perl $(PRJ_DIR)tools/p_readelf.pl $(LIBFILES)  $(OBJECTS)) || exit 1;
	rm -rf $(BIN)/libcshell.a
	make -C cshell TARGET=cshell $(INCLIBS)
	$(LD) -o $(OUT) $^ $(LDFLAGS)
endif

$(BIN)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(INCLIBS)

clean:
ifneq ($(target), )
	make -C $(target) TARGET=$(target) clean
else
	rm -f $(BIN)/$(TARGET)/*.o $(BIN)/$(TARGET)/*.d $(BIN)/$(TARGET)/*.sym.txt
	rm -f $(BIN)/*.a $(BIN)/*.sym.txt  $(OUT) *.sym.txt   *.a
	for dir in $(LIBDIRS); do (make -C $$dir clean || exit 1) || exit 1; done
endif

rebuild: clean exe

veryclean:
	rm -rf bin
	for dir in $(LIBDIRS); do (cd $$dir && rm -rf bin) || exit 1; done
