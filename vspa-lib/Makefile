# SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
#Copyright 2025 NXP

VSPA_TOOL ?= /opt/VSPA_Tools_vbeta_14_00_781
FSVSPAIncludes ?= $(VSPA_TOOL)/include
PROJ_DIR ?= ${PWD}/..
DEST_DIR ?= ${PWD}/../install
LIB_DIR ?= ${DEST_DIR}/usr/lib

VSPA_SDK ?= ${CURDIR}/../vspa-sdk

VCC = $(VSPA_TOOL)/bin/fsvspacc
VAR = $(VSPA_TOOL)/bin/vspa-ar
VLD = $(VSPA_TOOL)/bin/fsvspacc

VCFLAGS = -arch vspa2 -au_count 16 -core_type sp -ansi off -D__LA9310__
VCFLAGS += -g -O3 -Os -opt=noalias_by_type -mvcpu -msgstyle gcc
VCFLAGS += -env "$(VSPA_TOOL)"

VLDFLAGS = -arch vspa2 -au_count 16 -core_type sp
VLDFLAGS += -no-startup-files -ansi off -g -cwd source -O3 -Os -msgstyle gcc
VLDFLAGS += -env "$(VSPA_TOOL)"

INCLUDES = -I${CURDIR}/inc -I${VSPA_SDK}/inc 

SRCS = $(wildcard src/*.sx) 
SRCS1 = $(wildcard src/*.c)
OBJS = ${SRCS:.sx=.obj}
OBJS1 = ${SRCS1:.c=.o}

all: vspa-kernel-lib.a 

vspa-kernel-lib.a: ${OBJS} ${OBJS1}
	${VAR} -cr vspa-kernel-lib.a ${OBJS}

%.obj: %.sx
	${VCC} ${VCFLAGS} ${INCLUDES} -c  $< -o $@

%.o: %.c
	${VCC} ${VCFLAGS} ${INCLUDES} -c  $< -o $@

clean:
	rm -rf src/*.o src/*.obj vspa-kernel-lib.a 

install:
	mkdir -p ${LIB_DIR};
	cp vspa-kernel-lib.a ${LIB_DIR}/



