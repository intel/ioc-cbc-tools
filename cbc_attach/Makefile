OUT_DIR ?= .

all:
	gcc -o $(OUT_DIR)/u_cbc_attach cbc_attach.c

clean:
	rm -rf $(OUT_DIR)/u_cbc_attach

install: $(OUT_DIR)/u_cbc_attach u_cbc_attach.service cbc_match.txt
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/share/ioc-cbc-tools
	install -t $(DESTDIR)/usr/bin $(OUT_DIR)/u_cbc_attach
	install -t $(DESTDIR)/usr/share/ioc-cbc-tools cbc_match.txt
	install -d $(DESTDIR)/usr/lib/systemd/system/
	install -p -m 0644 u_cbc_attach.service $(DESTDIR)/usr/lib/systemd/system/
