# SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
#Copyright 2025 NXP


VSPA_TOOL ?= /opt/VSPA_Tools_vbeta_14_00_781
FSVSPAIncludes ?= $(VSPA_TOOL)/include
PROJ_DIR ?= ${CURDIR}/..
DEST_DIR ?= ${PROJ_DIR}/install
LIB_DIR ?= ${DEST_DIR}/usr/lib

VCC = $(VSPA_TOOL)/bin/fsvspacc
VAR = $(VSPA_TOOL)/bin/vspa-ar
VLD = $(VSPA_TOOL)/bin/fsvspacc

VCFLAGS = -arch vspa2 -au_count 16 -core_type sp -ansi off -D__LA9310__
VCFLAGS += -g -O3 -Os -opt=noalias_by_type -mvcpu -msgstyle gcc
VCFLAGS += -env "$(VSPA_TOOL)"

VLDFLAGS = -arch vspa2 -au_count 16 -core_type sp
VLDFLAGS += -no-startup-files -ansi off -g -cwd source -O3 -Os -msgstyle gcc
VLDFLAGS += -env "$(VSPA_TOOL)"

INCLUDES = -I${CURDIR}/inc 

SRCS =  $(wildcard src/*.sx) 
SRCS1 = $(wildcard src/*.c)

OBJS = ${SRCS1:.c=.o} ${SRCS:.sx=.obj}

all: vspa-sdk-lib.a 

vspa-sdk-lib.a: ${OBJS}
	${VAR} -cr vspa-sdk-lib.a ${OBJS}

%.obj: %.sx
	${VCC} ${VCFLAGS} ${INCLUDES} -c  $< -o $@
%.o: %.c
	${VCC} ${VCFLAGS} ${INCLUDES} -c  $< -o $@

clean:
	rm -rf src/*.o src/*.obj vspa-sdk-lib.a 

install:
	mkdir -p ${LIB_DIR}
	cp vspa-sdk-lib.a ${LIB_DIR}/



