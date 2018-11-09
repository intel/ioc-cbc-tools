OUT_DIR ?= .
DEBUG ?= 1

ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG
else
    CFLAGS += -DNDEBUG
endif

CFLAGS += -I$(CURDIR)/inc -Wall

$(OUT_DIR)/cbc_diagnostic:
	gcc -o $(OUT_DIR)/cbc_diagnostic $(CFLAGS) $(LDFLAGS) $(CURDIR)/src/cbc_diagnostic_control_frame_handler.c $(CURDIR)/src/cbc_diagnostic_control_main.c $(CURDIR)/src/cbc_diagnostic_control_options.c 

clean:
	rm -rf $(OUT_DIR)/cbc_diagnostic

install: $(OUT_DIR)/cbc_diagnostic
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/share/ioc-cbc-tools
	install -t $(DESTDIR)/usr/bin $(OUT_DIR)/cbc_diagnostic
