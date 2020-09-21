ifdef VITASDK
PREFIX = $(VITASDK)/bin/
endif

ARCH ?= $(PREFIX)arm-vita-eabi
AS = $(ARCH)-as
AR = $(ARCH)-ar
RANLIB = $(ARCH)-ranlib

TARGETS = libSceKbl_365_stub.a libSceKblForKernel_365_stub.a
TARGETS_WEAK = libSceKbl_365_stub_weak.a

SceKbl_365_OBJS =
SceKbl_365_weak_OBJS =
SceKblForKernel_365_OBJS = SceKbl_SceKblForKernel_KblDeinitComm_365.o SceKbl_SceKblForKernel_KblPrintf1_365.o SceKbl_SceKblForKernel_KblExecSysrootx360x20_365.o SceKbl_SceKblForKernel_KblExecSysrootx2d4_365.o SceKbl_SceKblForKernel_KblDeinitStorageHandlers_365.o SceKbl_SceKblForKernel_KblSyncCpuSysrootx2c_365.o SceKbl_SceKblForKernel_KblIsModelx102_365.o SceKbl_SceKblForKernel_KblLoadModulesFromList_365.o SceKbl_SceKblForKernel_KblPrintf3_365.o SceKbl_SceKblForKernel_KblStopKASM_365.o SceKbl_SceKblForKernel_KblCpuBarrierStart_365.o SceKbl_SceKblForKernel_KblIsNotDolce_365.o SceKbl_SceKblForKernel_KblIsCex_365.o SceKbl_SceKblForKernel_KblIsModelx100_365.o SceKbl_SceKblForKernel_KblStartModulesFromList_365.o SceKbl_SceKblForKernel_KblGetDevRegs_365.o SceKbl_SceKblForKernel_KblWriteModStartRegs_365.o SceKbl_SceKblForKernel_KblIsDolce_365.o SceKbl_SceKblForKernel_KblGetCpuId_365.o SceKbl_SceKblForKernel_KblIsModelx101_365.o SceKbl_SceKblForKernel_KblIsGenuineDolce_365.o SceKbl_SceKblForKernel_KblGetDipsw204And205Stat_365.o SceKbl_SceKblForKernel_KblIsDoubleModel_365.o SceKbl_SceKblForKernel_KblExecSysrootx2d8_365.o SceKbl_SceKblForKernel_KblCheckDipsw_365.o SceKbl_SceKblForKernel_KblParamx2dAnd1_365.o SceKbl_SceKblForKernel_KblGetHwConfig_365.o SceKbl_SceKblForKernel_KblDisableIrq_365.o SceKbl_SceKblForKernel_KblSetupStorages_365.o
SceKernel_OBJS = SceKbl_SceKblForKernel_KblDeinitComm_365.o SceKbl_SceKblForKernel_KblPrintf1_365.o SceKbl_SceKblForKernel_KblExecSysrootx360x20_365.o SceKbl_SceKblForKernel_KblExecSysrootx2d4_365.o SceKbl_SceKblForKernel_KblDeinitStorageHandlers_365.o SceKbl_SceKblForKernel_KblSyncCpuSysrootx2c_365.o SceKbl_SceKblForKernel_KblIsModelx102_365.o SceKbl_SceKblForKernel_KblLoadModulesFromList_365.o SceKbl_SceKblForKernel_KblPrintf3_365.o SceKbl_SceKblForKernel_KblStopKASM_365.o SceKbl_SceKblForKernel_KblCpuBarrierStart_365.o SceKbl_SceKblForKernel_KblIsNotDolce_365.o SceKbl_SceKblForKernel_KblIsCex_365.o SceKbl_SceKblForKernel_KblIsModelx100_365.o SceKbl_SceKblForKernel_KblStartModulesFromList_365.o SceKbl_SceKblForKernel_KblGetDevRegs_365.o SceKbl_SceKblForKernel_KblWriteModStartRegs_365.o SceKbl_SceKblForKernel_KblIsDolce_365.o SceKbl_SceKblForKernel_KblGetCpuId_365.o SceKbl_SceKblForKernel_KblIsModelx101_365.o SceKbl_SceKblForKernel_KblIsGenuineDolce_365.o SceKbl_SceKblForKernel_KblGetDipsw204And205Stat_365.o SceKbl_SceKblForKernel_KblIsDoubleModel_365.o SceKbl_SceKblForKernel_KblExecSysrootx2d8_365.o SceKbl_SceKblForKernel_KblCheckDipsw_365.o SceKbl_SceKblForKernel_KblParamx2dAnd1_365.o SceKbl_SceKblForKernel_KblGetHwConfig_365.o SceKbl_SceKblForKernel_KblDisableIrq_365.o SceKbl_SceKblForKernel_KblSetupStorages_365.o
ALL_OBJS=

all: $(TARGETS) $(TARGETS_WEAK)

define LIBRARY_template
 $(1): $$($(1:lib%_stub.a=%)_OBJS)
 ALL_OBJS += $$($(1:lib%_stub.a=%)_OBJS)
endef
define LIBRARY_WEAK_template
 $(1): $$($(1:lib%_stub_weak.a=%)_weak_OBJS)
 ALL_OBJS += $$($(1:lib%_stub_weak.a=%)_weak_OBJS)
endef

$(foreach library,$(TARGETS),$(eval $(call LIBRARY_template,$(library))))
$(foreach library,$(TARGETS_WEAK),$(eval $(call LIBRARY_WEAK_template,$(library))))

install: $(TARGETS) $(TARGETS_WEAK)
	cp $(TARGETS) $(VITASDK)/arm-vita-eabi/lib
	cp $(TARGETS_WEAK) $(VITASDK)/arm-vita-eabi/lib

clean:
	rm -f $(TARGETS) $(TARGETS_WEAK) $(ALL_OBJS)

$(TARGETS) $(TARGETS_WEAK):
	$(AR) cru $@ $?
	$(RANLIB) $@

%.o: %.S
	$(AS) --defsym GEN_WEAK_EXPORTS=0 $< -o $@

%.wo: %.S
	$(AS) --defsym GEN_WEAK_EXPORTS=1 $< -o $@
