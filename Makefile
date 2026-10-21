CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share

PKG_CONFIG ?= pkg-config
PKGS = gtk+-3.0 gtk-layer-shell-0 webkit2gtk-4.1 glib-2.0

CFLAGS += $(shell $(PKG_CONFIG) --cflags $(PKGS))
LIBS = $(shell $(PKG_CONFIG) --libs $(PKGS))

SRCS = src/main.c src/app.c src/config.c src/monitor.c src/webview.c src/ipc.c
OBJS = $(SRCS:.c=.o)
TARGET = duckai-sidebar

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -d $(DESTDIR)$(DATADIR)/applications
	install -m 644 data/duckai-sidebar.desktop $(DESTDIR)$(DATADIR)/applications/
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps
	install -m 644 data/duckai-sidebar.svg $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps/
	install -d $(DESTDIR)$(DATADIR)/duckai-sidebar
	install -m 644 data/duckai-sidebar.css $(DESTDIR)$(DATADIR)/duckai-sidebar/
	install -m 644 data/duckai-sidebar.conf $(DESTDIR)$(DATADIR)/duckai-sidebar/
	install -d $(DESTDIR)$(DATADIR)/duckai-sidebar/hyprland
	install -m 644 hyprland/duckai-sidebar.conf $(DESTDIR)$(DATADIR)/duckai-sidebar/hyprland/

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(DATADIR)/applications/duckai-sidebar.desktop
	rm -f $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps/duckai-sidebar.svg
	rm -rf $(DESTDIR)$(DATADIR)/duckai-sidebar

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf build
