all: libossoproductinfo.so.0.7.3 osso-product-info

libossoproductinfo.so.0.7.3: libossoproductinfo.c libossoproductinfo.h
	$(CC) $(CFLAGS) $(shell pkg-config --cflags --libs libcal sysinfo dbus-1) -shared -Wl,-soname=libossoproductinfo.so.0 $^ -o $@

osso-product-info: osso-product-info.c libossoproductinfo.so.0.7.3
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(RM) libossoproductinfo.so.0.7.3 osso-product-info

install:
	install -d "$(DESTDIR)/usr/include/"
	install -d "$(DESTDIR)/usr/lib/"
	install -d "$(DESTDIR)/usr/bin/"
	install -m 644 libossoproductinfo.h "$(DESTDIR)/usr/include/"
	install -m 755 osso-product-info "$(DESTDIR)/usr/bin/"
	install -m 755 libossoproductinfo.so.0.7.3 "$(DESTDIR)/usr/lib/"
	ln -s libossoproductinfo.so.0.7.3 "$(DESTDIR)/usr/lib/libossoproductinfo.so.0"
	ln -s libossoproductinfo.so.0 "$(DESTDIR)/usr/lib/libossoproductinfo.so"
