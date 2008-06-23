#
# system.mk
#
# Copyright (C) 2005,2008 Robert William Fuller <hydrologiccycle@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.	If not, see <http://www.gnu.org/licenses/>.
#

BUILD ?= release
INSTALL_PREFIX ?= /usr/local
INSTALL_BIN_DIR ?= $(INSTALL_PREFIX)/bin

UNAME_SYSTEM		:= $(shell uname -s)
ifeq (SunOS, $(UNAME_SYSTEM))
	UNAME_PLATFORM	:= $(shell uname -p)
	BITS	 ?= $(shell isainfo -b)
else ifeq (Linux, $(UNAME_SYSTEM))
	UNAME_PLATFORM	:= $(shell uname -m)
	ifeq (64, $(findstring 64, $(UNAME_PLATFORM)))
		BITS ?= 64
	else ifeq (32, $(findstring 32, $(UNAME_PLATFORM)))
		BITS ?= 32
	endif
endif
BITS		 ?= 32

ifeq (SunOS, $(UNAME_SYSTEM))
	CC_BIN	?= /opt/SUNWspro/bin/cc
else ifeq (Linux, $(UNAME_SYSTEM))
	CC_BIN	?= gcc
else
	CC_BIN	?= cc
endif
LD_BIN		?= $(CC_BIN)

ifeq (gcc, $(notdir $(CC_BIN)))
	GCC	  := t
    #CC_FLAGS += -pedantic -std=gnu99
else ifeq (g++, $(notdir $(CC_BIN)))
	GCC	  := t
    #CC_FLAGS += -pedantic -std=gnu++98
else ifeq (SUNWspro, $(findstring SUNWspro, $(CC_BIN)))
	SUNWS := t
endif

ifdef SUNWS
	LD_FLAGS += $(CC_FLAGS) -xildoff
else
	LD_FLAGS += $(CC_FLAGS)
endif

AR_BIN	 ?= ar
AR_FLAGS += rcs


OBJ_DIR			:= obj/$(UNAME_SYSTEM)/$(UNAME_PLATFORM)/$(BITS)/$(BUILD)/
LIB_PROTO_DIR	:= lib/$(UNAME_SYSTEM)/$(UNAME_PLATFORM)/$(BITS)/$(BUILD)
ifdef BIN
	BIN_DIR		:= bin/$(UNAME_SYSTEM)/$(UNAME_PLATFORM)/$(BITS)/$(BUILD)/
	BIN_PATH	:= $(BIN_DIR)$(BIN)
endif
ifdef LIB
	LIB_DIR		:= $(LIB_PROTO_DIR)/
	LIB_PATH	:= $(LIB_DIR)lib$(LIB).a
endif
vpath %.o $(OBJ_DIR)
vpath %.a /usr/lib $(addsuffix /$(LIB_PROTO_DIR), $(LIBDIRS)) $(EXTLIBDIRS)
ifdef GCC
    vpath %.a $(dir $(shell $(CC_BIN) -print-file-name=libgcc.a))
endif

CC_FLAGS			+= $(CC_DEFINES)
ifdef GCC
	CC_FLAGS		+= -Wall -Wstrict-aliasing=2
	CC_DEP_FLAGS	+= -MM
	ifeq (SunOS, $(UNAME_SYSTEM))
		CC_DEFINES	+= -D__$(UNAME_SYSTEM)_$(subst ., _, $(shell uname -r))
	endif
else ifdef SUNWS
	CC_FLAGS		+= -xCC -Xa -v
	CC_DEP_FLAGS	+= -xM1
endif

ifeq (release, $(BUILD))
	CC_FLAGS		+= -g
	ifdef GCC
		CC_FLAGS	+= -O2
	else
		CC_FLAGS	+= -O
	endif
	#CC_DEFINES		 += -DNDEBUG
else ifeq (debug, $(BUILD))
	CC_FLAGS		+= -g
	#CC_DEFINES		 += -DDEBUG
endif

ifeq (32, $(BITS))
	ifdef GCC
		CC_FLAGS	+= -m32
	endif
else ifeq (64, $(BITS))
	ifdef GCC
		CC_FLAGS	+= -m64
		ifeq (sparc, $(UNAME_PLATFORM))
			#CC_FLAGS += -mcpu=v9
			CC_FLAGS += -march=v9
		endif
	else ifdef SUNWS
		ifeq (sparc, $(UNAME_PLATFORM))
			CC_FLAGS += -xarch=v9
		else ifeq (i386, $(UNAME_PLATFORM))
			CC_FLAGS += -xarch=amd64
		endif
	endif
endif

ifdef INCLUDE
	CC_FLAGS += $(addprefix -I, $(INCLUDE))
endif


.SUFFIXES :
.SUFFIXES : .c .o .m

define run-cc
	$(CC_BIN) $(CC_FLAGS) -c -o $(addprefix $(OBJ_DIR), $@) $<
endef

%.o : %.c
	$(run-cc)

%.o : %.m
	$(run-cc)

.PHONY : all clean depend dependclean distclean install


all : $(BIN_PATH) $(LIB_PATH)

.dependencies depend :
	$(CC_BIN) $(CC_FLAGS) $(CC_DEP_FLAGS) $(foreach src, $(SRC), $(or $(wildcard ${src}.c), $(wildcard ${src}.m))) >.dependencies

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
# this directive cannot be indented, or it will be ignored!
include .dependencies
endif

$(OBJ_DIR) $(BIN_DIR) $(LIB_DIR) :
	@-mkdir -p $@

$(BIN_PATH) :: $(OBJ_DIR) $(BIN_DIR) ;

$(BIN_PATH) :: $(addsuffix .o, $(SRC)) $(addprefix lib, $(addsuffix .a, $(LIBS)))
	$(LD_BIN) $(LD_FLAGS) -o $@ $(addprefix $(OBJ_DIR), $(addsuffix .o, $(SRC))) \
        $(addprefix -L, $(EXTLIBDIRS)) $(addprefix -L, $(addsuffix /$(LIB_PROTO_DIR),$(LIBDIRS))) $(addprefix -l, $(LIBS))

$(LIB_PATH) :: $(OBJ_DIR) $(LIB_DIR) ;

$(LIB_PATH) :: $(addsuffix .o, $(SRC))
	$(AR_BIN) $(AR_FLAGS) $@ $(foreach obj, $?, $(if $(findstring $(OBJ_DIR), $(obj)), $(obj), $(addprefix $(OBJ_DIR), $(obj))))

install : all
	$(if $(BIN_PATH),cp -p $(BIN_PATH) $(INSTALL_BIN_DIR))

dependclean :
	@-rm .dependencies 2>/dev/null

clean : dependclean
	@-rm $(addprefix $(OBJ_DIR), $(addsuffix .o, $(SRC))) 2>/dev/null
	@-rm $(BIN_PATH) $(LIB_PATH) 2>/dev/null
	@-rmdir -p $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR) 2>/dev/null

distclean : dependclean
	@-rm -rf bin lib obj *~ 2>/dev/null
