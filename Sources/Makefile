# SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
# Copyright 2025 NXP

PROJ_DIR ?= ${CURDIR}/..
VSPA_TOOL ?= /opt/VSPA_Tools_vbeta_14_00_781
FSVSPAIncludes ?= $(VSPA_TOOL)/include
DEST_DIR ?=  ${PROJ_DIR}/install
FIRM_DIR ?= ${DEST_DIR}/lib/firmware
VSPA_SDK ?= ${PROJ_DIR}/vspa-sdk
VSPA_LIB ?= ${PROJ_DIR}/vspa-lib

VCC = $(VSPA_TOOL)/bin/fsvspacc
VAR = $(VSPA_TOOL)/bin/vspa-ar
VLD = $(VSPA_TOOL)/bin/fsvspacc

VCFLAGS_COMMON = -arch vspa2 -au_count 16 -core_type sp -ansi off -D__LA9310__ -g -O3 -Os -opt=noalias_by_type -mvcpu -msgstyle gcc -env "$(VSPA_TOOL)"
VLDFLAGS_COMMON = -arch vspa2 -au_count 16 -core_type sp -no-startup-files -ansi off -g -cwd source -O3 -Os -msgstyle gcc -env "$(VSPA_TOOL)"
INCLUDES += -I${CURDIR}/inc -I${VSPA_SDK}/inc -I${VSPA_LIB}/inc -I${PROJ_DIR}/include
LDLIBS = -l../vspa-lib/vspa-kernel-lib.a -l../vspa-sdk/vspa-sdk-lib.a

CONFIGS = 0T1R 1T0R 1T1R 1T2R 1T4R

_OBJS_0T1R = dc_cal.o dfe.o iqmod_rx.o l1-trace.o _main.o ovly_init.o signal.o crt0.obj
_OBJS_1T0R = dc_cal.o dfe.o iqmod_tx.o l1-trace.o _main.o ovly_init.o signal.o crt0.obj
_OBJS_1T1R = dc_cal.o dfe.o iqmod_rx.o iqmod_tx.o l1-trace.o _main.o ovly_init.o signal.o crt0.obj
_OBJS_1T2R = dc_cal.o dfe.o iqmod_rx_2R_decx2x4.o iqmod_tx_2R.o l1-trace.o _main.o ovly_init.o signal.o crt0.obj
_OBJS_1T4R = dc_cal.o dfe.o iqmod_rx_2R_decx2x4.o iqmod_tx_2R.o l1-trace.o _main.o ovly_init.o signal.o crt0.obj

ALL_TARGETS = $(foreach cfg,$(CONFIGS),$(PROJ_DIR)/Debug_IQPLAYER_$(cfg)/apm-iqplayer-$(cfg).eld)
.PHONY: all
all: $(ALL_TARGETS)

define BUILD_template
ODIR_$1 = $(PROJ_DIR)/Debug_IQPLAYER_$1
VCFLAGS_$1 = $(VCFLAGS_COMMON) -DIQMOD_RX_$1
VLDFLAGS_$1 = $(VLDFLAGS_COMMON) -Xlnk "-Map $$(ODIR_$1)/la9310_cal.map" -mem "$$(PROJ_DIR)/LCF/la9310_iqplayer.lcf"
OBJS_$1 = $(patsubst %,$$(ODIR_$1)/%,$(_OBJS_$1))

$$(ODIR_$1)/apm-iqplayer-$1.eld: $$(OBJS_$1)
	$$(VLD) -o $$@ $$(VLDFLAGS_$1) $(LDLIBS) $$^

$$(ODIR_$1)/%.o: %.c
	echo $$(ALL_TARGETS)
	mkdir -p $$(ODIR_$1)
	$$(VCC) $$(VCFLAGS_$1) $(INCLUDES) -c $$< -o $$@

$$(ODIR_$1)/%.obj: %.s
	mkdir -p $$(ODIR_$1)
	$$(VCC) $$(VCFLAGS_$1) $(INCLUDES) -c $$< -o $$@
endef

$(foreach cfg,$(CONFIGS),$(eval $(call BUILD_template,$(cfg))))

clean:
	rm -rf $(PROJ_DIR)/Debug_IQPLAYER_*

install:
	mkdir -p ${FIRM_DIR} 
	cp $(PROJ_DIR)/Debug_IQPLAYER_*/apm-iqplayer*.eld ${FIRM_DIR}/
	cp $(PROJ_DIR)/Debug_IQPLAYER_1T1R/apm-iqplayer-1T1R.eld ${FIRM_DIR}/apm-iqplayer.eld

