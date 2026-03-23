INSTALLER_DIR := installer
CORE_DIR := core
RECOVERY_DIR := recovery
PLUGINS_DIR := plugins

P_CKLDR_DIR := $(PLUGINS_DIR)/loader
P_HENCFG_DIR := $(PLUGINS_DIR)/hencfg
P_CULOGO_DIR := $(PLUGINS_DIR)/culogo
KBL_STUBS_DIR := $(PLUGINS_DIR)/out_kbl_stubs

FIN_INSTALLER := $(INSTALLER_DIR)/build/enso_installer.vpk
FIN_CORE := $(CORE_DIR)/fat.bin
FIN_RECOVERY := $(RECOVERY_DIR)/output/rblob.e2xp
FIN_P_CKLDR := $(P_CKLDR_DIR)/e2x_ckldr.skprx
FIN_P_HENCFG := $(P_HENCFG_DIR)/e2xhencfg.skprx
FIN_P_CULOGO := $(P_CULOGO_DIR)/e2xculogo.skprx
FIN_KBL_STUBS := $(VITASDK)/arm-vita-eabi/lib/libSceKblForKernel_365_stub.a

all: enso_ex.vpk

enso_ex.vpk: $(FIN_INSTALLER)
	cp $< $@

$(FIN_INSTALLER): $(FIN_CORE) $(FIN_RECOVERY) $(FIN_P_CKLDR) $(FIN_P_HENCFG) $(FIN_P_CULOGO)
	mkdir $(INSTALLER_DIR)/res_ext
	cp $(CORE_DIR)/fat.bin $(INSTALLER_DIR)/res_ext/fat.bin
	echo "#define FATCHECK 0x$$(crc32 $(FIN_CORE))" > $(INSTALLER_DIR)/src/fatcheck.h
	cp $(RECOVERY_DIR)/output/rconfig.e2xr $(INSTALLER_DIR)/res_ext/rconfig.e2xr
	cp $(RECOVERY_DIR)/output/rblob.e2xp $(INSTALLER_DIR)/res_ext/rblob.e2xp
	cp $(P_CKLDR_DIR)/e2x_ckldr.skprx $(INSTALLER_DIR)/res_ext/e2x_ckldr.skprx
	cp $(P_CKLDR_DIR)/example_list.txt $(INSTALLER_DIR)/res_ext/boot_list.txt
	cp $(P_HENCFG_DIR)/e2xhencfg.skprx $(INSTALLER_DIR)/res_ext/e2xhencfg.skprx
	cp $(P_CULOGO_DIR)/e2xculogo.skprx $(INSTALLER_DIR)/res_ext/e2xculogo.skprx
	cp $(P_CULOGO_DIR)/example_logo.raw $(INSTALLER_DIR)/res_ext/bootlogo.raw
	mkdir $(INSTALLER_DIR)/build
	cmake -S $(INSTALLER_DIR) -B $(INSTALLER_DIR)/build
	$(MAKE) -C $(INSTALLER_DIR)/build

$(FIN_P_CULOGO): $(FIN_KBL_STUBS)
	$(MAKE) -C $(P_CULOGO_DIR)

$(FIN_P_HENCFG): $(FIN_KBL_STUBS)
	$(MAKE) -C $(P_HENCFG_DIR)

$(FIN_P_CKLDR): $(FIN_KBL_STUBS)
	$(MAKE) -C $(P_CKLDR_DIR)

$(FIN_RECOVERY):
	$(MAKE) -C $(RECOVERY_DIR)

$(FIN_CORE):
	$(MAKE) -C $(CORE_DIR)

$(FIN_KBL_STUBS):
	vita-libs-gen $(PLUGINS_DIR)/SceKbl.yml $(KBL_STUBS_DIR)
	$(MAKE) -C $(KBL_STUBS_DIR)
	$(MAKE) -C $(KBL_STUBS_DIR) install

clean:
	rm -rf $(INSTALLER_DIR)/build
	rm -rf $(INSTALLER_DIR)/res_ext
	$(MAKE) -C $(CORE_DIR) clean
	$(MAKE) -C $(RECOVERY_DIR) clean
	$(MAKE) -C $(P_CKLDR_DIR) clean
	$(MAKE) -C $(P_HENCFG_DIR) clean
	$(MAKE) -C $(P_CULOGO_DIR) clean
	rm -rf $(KBL_STUBS_DIR)