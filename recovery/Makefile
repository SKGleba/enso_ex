BLOB_DIR=blob
BLOB_FILE=$(BLOB_DIR)/rblob.e2xp
BOOTMGR_FILE=$(BLOB_DIR)/bootmgr.e2xp
LIB_FILE=$(BLOB_DIR)/libe2xr.a

BOTSTRAP_DIR=bootstrap
BOTSTRAP_FILE=$(BOTSTRAP_DIR)/rconfig.e2xr

SECOND_DIR=second
SECOND_FILE=$(SECOND_DIR)/second.bin

OUTPUT_DIR=output
OUTPUT_FILES=$(OUTPUT_DIR)/rconfig.e2xr $(OUTPUT_DIR)/rblob.e2xp $(OUTPUT_DIR)/sd_recovery.img $(OUTPUT_DIR)/bootmgr.e2xp $(OUTPUT_DIR)/second.e2xp $(OUTPUT_DIR)/libe2xr.a

all: $(OUTPUT_FILES)
	@echo "Build complete. Output files are in $(OUTPUT_DIR)."

$(OUTPUT_DIR)/sd_recovery.img: $(BOTSTRAP_FILE) $(OUTPUT_DIR)/rconfig.e2xr $(OUTPUT_DIR)/rblob.e2xp $(OUTPUT_DIR)/second.e2xp
	@mkdir -p $(OUTPUT_DIR)
	mv $(BOTSTRAP_FILE) $@
	truncate -s 2048 $@
	cat $(OUTPUT_DIR)/rconfig.e2xr >> $@
	truncate -s 2560 $@
	cat $(OUTPUT_DIR)/second.e2xp >> $@
	truncate -s 24576 $@
	cat $(OUTPUT_DIR)/rblob.e2xp >> $@
	truncate -s 234496 $@

$(OUTPUT_DIR)/rconfig.e2xr: $(BOTSTRAP_FILE)
	@mkdir -p $(OUTPUT_DIR)
	cp $< $@

$(BOTSTRAP_FILE):
	$(MAKE) -C $(BOTSTRAP_DIR)

$(OUTPUT_DIR)/libe2xr.a: $(BLOB_FILE)
	@mkdir -p $(OUTPUT_DIR)
	mv $(LIB_FILE) $@

$(OUTPUT_DIR)/bootmgr.e2xp: $(BLOB_FILE)
	@mkdir -p $(OUTPUT_DIR)
	mv $(BOOTMGR_FILE) $@

$(OUTPUT_DIR)/rblob.e2xp: $(BLOB_FILE)
	@mkdir -p $(OUTPUT_DIR)
	mv $< $@

$(BLOB_FILE):
	$(MAKE) -C $(BLOB_DIR)

$(OUTPUT_DIR)/second.e2xp: $(SECOND_FILE)
	@mkdir -p $(OUTPUT_DIR)
	mv $< $@

$(SECOND_FILE):
	$(MAKE) -C $(SECOND_DIR)

clean:
	rm -rf $(OUTPUT_DIR)
	$(MAKE) -C $(BLOB_DIR) clean
	$(MAKE) -C $(BOTSTRAP_DIR) clean
	$(MAKE) -C $(SECOND_DIR) clean