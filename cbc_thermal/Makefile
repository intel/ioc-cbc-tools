OUT_DIR ?= .

CFLAGS += `pkg-config --cflags fuse`
LDFLAGS += -pthread
LDFLAGS += `pkg-config --libs fuse`

$(OUT_DIR)/cbc_thermal: cbc_thermal.c
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm $(OUT_DIR)/cbc_thermal

install: $(OUT_DIR)/cbc_thermal cbc_thermal_chart.py cbc_thermald.service thermal-conf.xml cbc_thermald_start
	install -d $(DESTDIR)/usr/bin
	install -t $(DESTDIR)/usr/bin $<
	install -t $(DESTDIR)/usr/bin cbc_thermal_chart.py
	install -t $(DESTDIR)/usr/bin cbc_thermald_start
	install -d $(DESTDIR)/usr/lib/systemd/system/
	install -d $(DESTDIR)/usr/share/ioc-cbc-tools/
	install -p -m 0644 cbc_thermald.service $(DESTDIR)/usr/lib/systemd/system/
	install -p -m 0644 thermal-conf.xml $(DESTDIR)/usr/share/ioc-cbc-tools/
