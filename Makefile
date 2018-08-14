T := $(CURDIR)
OUT_DIR ?= $(CURDIR)/build

.PHONY: all cbc_lifecycle cbc_attach cbc_thermal
all: cbc_lifecycle cbc_attach cbc_thermal

cbc_thermal:
	mkdir -p $(OUT_DIR)
	make -C $(T)/cbc_thermal OUT_DIR=$(OUT_DIR)
cbc_lifecycle:
	mkdir -p $(OUT_DIR)
	make -C $(T)/cbc_lifecycle OUT_DIR=$(OUT_DIR)
cbc_attach:
	mkdir -p $(OUT_DIR)
	make -C $(T)/cbc_attach OUT_DIR=$(OUT_DIR)

.PHONY: clean
clean:
	make -C $(T)/cbc_lifecycle clean
	make -C $(T)/cbc_attach clean
	make -C $(T)/cbc_thermal clean
	rm -rf $(OUT_DIR)

.PHONY: install
install: cbc_lifecycle-install cbc_attach-install cbc_thermal-install

cbc_lifecycle-install:
	make -C $(T)/cbc_lifecycle OUT_DIR=$(OUT_DIR) install

cbc_attach-install:
	make -C $(T)/cbc_attach OUT_DIR=$(OUT_DIR) install

cbc_thermal-install:
	make -C $(T)/cbc_thermal OUT_DIR=$(OUT_DIR) install
