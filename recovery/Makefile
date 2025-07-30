BLOB_DIR=blob
BLOB_FILE=$(BLOB_DIR)/rblob.e2xp

BOTSTRAP_EMMC_DIR=bootstrap-emmc
BOTSTRAP_EMMC_FILE=$(BOTSTRAP_EMMC_DIR)/rconfig.e2xp

BOTSTRAP_SD_DIR=bootstrap-sd
BOTSTRAP_SD_FILE=$(BOTSTRAP_SD_DIR)/rconfig.e2xr

OUTPUT_DIR=output
OUTPUT_FILES=$(OUTPUT_DIR)/rconfig.e2xp $(OUTPUT_DIR)/rblob.e2xp $(OUTPUT_DIR)/sd_recovery.img

all: $(OUTPUT_FILES)
	@echo "Build complete. Output files are in $(OUTPUT_DIR)."

$(OUTPUT_DIR)/rconfig.e2xp: $(BOTSTRAP_EMMC_FILE)
	@mkdir -p $(OUTPUT_DIR)
	mv $< $@

$(BOTSTRAP_EMMC_FILE):
	$(MAKE) -C $(BOTSTRAP_EMMC_DIR)

$(OUTPUT_DIR)/sd_recovery.img: $(BOTSTRAP_SD_FILE) $(OUTPUT_DIR)/rconfig.e2xp $(OUTPUT_DIR)/rblob.e2xp
	@mkdir -p $(OUTPUT_DIR)
	mv $(BOTSTRAP_SD_FILE) $@
	truncate -s 512 $@
	cat $(OUTPUT_DIR)/rconfig.e2xp >> $@
	truncate -s 1024 $@
	cat $(OUTPUT_DIR)/rblob.e2xp >> $@
	truncate -s 234496 $@

$(BOTSTRAP_SD_FILE):
	$(MAKE) -C $(BOTSTRAP_SD_DIR)

$(OUTPUT_DIR)/rblob.e2xp: $(BLOB_FILE)
	@mkdir -p $(OUTPUT_DIR)
	mv $< $@

$(BLOB_FILE):
	$(MAKE) -C $(BLOB_DIR)

clean:
	rm -rf $(OUTPUT_DIR)