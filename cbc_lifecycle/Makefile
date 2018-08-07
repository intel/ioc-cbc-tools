
OUT_DIR ?= .

$(OUT_DIR)/u_cbc_lifecycle: cbc_lifecycle.c
	gcc -o $@ cbc_lifecycle.c -pthread -lacrn-mngr

clean:
	rm $(OUT_DIR)/u_cbc_lifecycle

install: $(OUT_DIR)/u_cbc_lifecycle u_cbc_lifecycle.service
	install -d $(DESTDIR)/usr/bin
	install -t $(DESTDIR)/usr/bin $<
	install -d $(DESTDIR)/usr/lib/systemd/system/
	install -p -m 0644 u_cbc_lifecycle.service $(DESTDIR)/usr/lib/systemd/system/
