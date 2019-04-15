T := $(CURDIR)
OUT_DIR ?= $(CURDIR)/build
CFLAGS += -fstack-protector-strong -fPIE -fPIC -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security
CFLAGS += -I$(CURDIR)
LDFLAGS += -z noexecstack -z relro -z now -pie
export CFLAGS
export LDFLAGS

.PHONY: all cbc_lifecycle cbc_attach cbc_thermal cbc_diagnostic_control cbc_logging_service
all: cbc_lifecycle cbc_attach cbc_thermal cbc_diagnostic_control cbc_logging_service

cbc_thermal:
	mkdir -p $(OUT_DIR)
	make -C $(T)/cbc_thermal OUT_DIR=$(OUT_DIR)
cbc_lifecycle:
	mkdir -p $(OUT_DIR)
	make -C $(T)/cbc_lifecycle OUT_DIR=$(OUT_DIR)
cbc_attach:
	mkdir -p $(OUT_DIR)
	make -C $(T)/cbc_attach OUT_DIR=$(OUT_DIR)
cbc_diagnostic_control:
	mkdir -p $(OUT_DIR)
	make -C $(T)/cbc_diagnostic_control OUT_DIR=$(OUT_DIR)
cbc_logging_service:
	mkdir -p $(OUT_DIR)
	make -C $(T)/cbc_logging_service OUT_DIR=$(OUT_DIR)

.PHONY: clean
clean:
	make -C $(T)/cbc_lifecycle clean OUT_DIR=$(OUT_DIR)
	make -C $(T)/cbc_attach clean OUT_DIR=$(OUT_DIR)
	make -C $(T)/cbc_thermal clean OUT_DIR=$(OUT_DIR)
	rm -rf $(OUT_DIR)

.PHONY: install
install: cbc_lifecycle-install cbc_attach-install cbc_thermal-install cbc_diagnostic_control-install cbc_logging_service-install

cbc_lifecycle-install:
	make -C $(T)/cbc_lifecycle OUT_DIR=$(OUT_DIR) install

cbc_attach-install:
	make -C $(T)/cbc_attach OUT_DIR=$(OUT_DIR) install

cbc_thermal-install:
	make -C $(T)/cbc_thermal OUT_DIR=$(OUT_DIR) install

cbc_diagnostic_control-install:
	make -C $(T)/cbc_diagnostic_control OUT_DIR=$(OUT_DIR) install

cbc_logging_service-install:
	make -C $(T)/cbc_logging_service OUT_DIR=$(OUT_DIR) install
