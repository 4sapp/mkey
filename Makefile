CC ?= cc
PKG_CONFIG ?= pkg-config
PREFIX ?= $(HOME)/.local
BUILD_DIR ?= build

CPPFLAGS += -D_GNU_SOURCE -Isrc
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Werror -Wpedantic -pthread
GLIB_CFLAGS ?= $(patsubst -I%,-isystem%,$(shell $(PKG_CONFIG) --cflags glib-2.0))
GLIB_LIBS ?= $(shell $(PKG_CONFIG) --libs glib-2.0)
GTK_CFLAGS ?= $(patsubst -I%,-isystem%,$(shell $(PKG_CONFIG) --cflags 'gtk4 >= 4.12'))
GTK_LIBS ?= $(shell $(PKG_CONFIG) --libs 'gtk4 >= 4.12')
PULSE_CFLAGS ?= $(patsubst -I%,-isystem%,$(shell $(PKG_CONFIG) --cflags libpulse-simple))
PULSE_LIBS ?= $(shell $(PKG_CONFIG) --libs libpulse-simple)

COMMON_SOURCES := $(wildcard src/common/*.c)
DAEMON_SOURCES := $(wildcard src/daemon/*.c) $(COMMON_SOURCES)
UI_SOURCES := $(wildcard src/ui/*.c) $(COMMON_SOURCES)
TEST_SOURCES := tests/test_core.c src/daemon/mixer.c src/daemon/samples.c src/daemon/input.c $(COMMON_SOURCES)
HEADERS := $(wildcard src/common/*.h src/daemon/*.h src/ui/*.h)

.PHONY: all test test-ui install uninstall clean format format-check
all: $(BUILD_DIR)/mkey $(BUILD_DIR)/mkey-gui

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/mkey: $(DAEMON_SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(GLIB_CFLAGS) $(PULSE_CFLAGS) $(DAEMON_SOURCES) $(LDFLAGS) $(GLIB_LIBS) $(PULSE_LIBS) -o $@

$(BUILD_DIR)/mkey-gui: $(UI_SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(GTK_CFLAGS) $(UI_SOURCES) $(LDFLAGS) $(GTK_LIBS) -o $@

$(BUILD_DIR)/test-core: $(TEST_SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(GLIB_CFLAGS) $(TEST_SOURCES) $(LDFLAGS) $(GLIB_LIBS) -o $@

test: $(BUILD_DIR)/test-core
	$(BUILD_DIR)/test-core

test-ui: $(BUILD_DIR)/mkey-gui
	@config=$$(mktemp -d); trap 'rm -rf "$$config"' EXIT; \
	XDG_CONFIG_HOME="$$config" MKEY_DATA_DIR="$(CURDIR)/data" $(BUILD_DIR)/mkey-gui --smoke-test

install: all
	install -d "$(DESTDIR)$(PREFIX)/bin" "$(DESTDIR)$(PREFIX)/share/mkey/sounds/mkey2"
	install -m 755 $(BUILD_DIR)/mkey $(BUILD_DIR)/mkey-gui "$(DESTDIR)$(PREFIX)/bin/"
	install -m 644 data/style.css "$(DESTDIR)$(PREFIX)/share/mkey/"
	install -m 644 data/sounds/*.pcm data/sounds/SOURCE.txt "$(DESTDIR)$(PREFIX)/share/mkey/sounds/"
	install -m 644 data/sounds/mkey2/*.pcm data/sounds/mkey2/SOURCE.txt "$(DESTDIR)$(PREFIX)/share/mkey/sounds/mkey2/"
	install -d "$(DESTDIR)$(PREFIX)/share/applications" "$(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps"
	sed -e 's|@BINDIR@|$(PREFIX)/bin|g' -e 's|@DATADIR@|$(PREFIX)/share/mkey|g' data/local.davo.mkey.desktop > "$(DESTDIR)$(PREFIX)/share/applications/local.davo.mkey.desktop"
	install -m 644 data/mkey.svg "$(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/local.davo.mkey.svg"
	install -d "$(DESTDIR)$(PREFIX)/share/systemd/user"
	sed -e 's|@BINDIR@|$(PREFIX)/bin|g' -e 's|@DATADIR@|$(PREFIX)/share/mkey|g' data/mkey.service > "$(DESTDIR)$(PREFIX)/share/systemd/user/mkey.service"
	@echo 'Installed. Run: systemctl --user daemon-reload && systemctl --user enable --now mkey.service'

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/mkey" "$(DESTDIR)$(PREFIX)/bin/mkey-gui"
	rm -f "$(DESTDIR)$(PREFIX)/share/applications/local.davo.mkey.desktop"
	rm -f "$(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/local.davo.mkey.svg"
	rm -f "$(DESTDIR)$(PREFIX)/share/systemd/user/mkey.service"
	rm -f "$(DESTDIR)$(PREFIX)/share/mkey/style.css" "$(DESTDIR)$(PREFIX)/share/mkey/sounds/"*.pcm "$(DESTDIR)$(PREFIX)/share/mkey/sounds/mkey2/"*.pcm
	rm -f "$(DESTDIR)$(PREFIX)/share/mkey/sounds/SOURCE.txt" "$(DESTDIR)$(PREFIX)/share/mkey/sounds/mkey2/SOURCE.txt"

format:
	clang-format -i $(DAEMON_SOURCES) $(UI_SOURCES) $(HEADERS) tests/*.c

format-check:
	clang-format --dry-run --Werror $(DAEMON_SOURCES) $(UI_SOURCES) $(HEADERS) tests/*.c

clean:
	rm -rf $(BUILD_DIR)
