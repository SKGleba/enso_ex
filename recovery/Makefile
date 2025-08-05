BLOB_DIR=blob
BLOB_FILE=$(BLOB_DIR)/rblob.e2xp

BOTSTRAP_DIR=bootstrap
BOTSTRAP_FILE=$(BOTSTRAP_DIR)/rconfig.e2xr

SECOND_DIR=second
SECOND_FILE=$(SECOND_DIR)/second.bin

OUTPUT_DIR=output
OUTPUT_FILES=$(OUTPUT_DIR)/rconfig.e2xr $(OUTPUT_DIR)/rblob.e2xp $(OUTPUT_DIR)/sd_recovery.img

all: $(OUTPUT_FILES)
	@echo "Build complete. Output files are in $(OUTPUT_DIR)."

$(OUTPUT_DIR)/sd_recovery.img: $(BOTSTRAP_SD_FILE) $(OUTPUT_DIR)/rconfig.e2xr $(OUTPUT_DIR)/rblob.e2xp $(SECOND_FILE)
	@mkdir -p $(OUTPUT_DIR)
	mv $(BOTSTRAP_FILE) $@
	truncate -s 2048 $@
	cat $(OUTPUT_DIR)/rconfig.e2xr >> $@
	truncate -s 2560 $@
	cat $(SECOND_FILE) >> $@
	truncate -s 24576 $@
	cat $(OUTPUT_DIR)/rblob.e2xp >> $@
	truncate -s 234496 $@

$(OUTPUT_DIR)/rconfig.e2xr: $(BOTSTRAP_FILE)
	@mkdir -p $(OUTPUT_DIR)
	cp $< $@

$(BOTSTRAP_FILE):
	$(MAKE) -C $(BOTSTRAP_DIR)

$(OUTPUT_DIR)/rblob.e2xp: $(BLOB_FILE)
	@mkdir -p $(OUTPUT_DIR)
	mv $< $@

$(BLOB_FILE):
	$(MAKE) -C $(BLOB_DIR)

$(SECOND_FILE):
	$(MAKE) -C $(SECOND_DIR)

clean:
	rm -rf $(OUTPUT_DIR)