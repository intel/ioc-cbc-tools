OUT_DIR ?= .
DEBUG ?= 1

ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG
else
    CFLAGS += -DNDEBUG
endif
    
CFLAGS += -I$(CURDIR)/inc -I/usr/local/include -I/usr/local/include/dlt $(shell pkg-config --cflags automotive-dlt-c++) -Wall
LDFLAGS += -ldl -ldlt -lrt -lpthread -lz

$(OUT_DIR)/cbc_logging:
	g++ -c $(CFLAGS) $(CURDIR)/src/cbc_logging_service_options.cpp
	g++ -c $(CFLAGS) $(CURDIR)/src/cbc_logging_service_main.cpp
	g++ -c $(CFLAGS) $(CURDIR)/src/cbc_logging_service.cpp
	g++ cbc_logging_service_options.o cbc_logging_service_main.o cbc_logging_service.o -o $(OUT_DIR)/cbc_logging $(LDFLAGS)

clean:
	rm -rf *.o $(OUT_DIR)/cbc_logging

install: $(OUT_DIR)/cbc_logging
	install -d $(DESTDIR)/usr/bin
	install -t $(DESTDIR)/usr/bin $(OUT_DIR)/cbc_logging

