#   make        - скачать QCustomPlot (если его нет) и собрать приложение
#   make run    - собрать и запустить; порт можно задать: make run PORT=/dev/ttys004
#   make clean  - удалить каталог сборки

QMAKE ?= qmake
BAUD ?= 19200

QCP_VERSION := 2.1.1
QCP_URL := https://www.qcustomplot.com/release/$(QCP_VERSION)/QCustomPlot-source.tar.gz
QCP_DIR := monitor/qcustomplot
QCP_FILES := $(QCP_DIR)/qcustomplot.h $(QCP_DIR)/qcustomplot.cpp

BUILD_DIR := monitor/build
ifeq ($(shell uname),Darwin)
APP := $(BUILD_DIR)/monitor.app/Contents/MacOS/monitor
else
APP := $(BUILD_DIR)/monitor
endif

.PHONY: all deps build run clean

all: build

deps: $(QCP_FILES)

# В архиве лежит каталог qcustomplot-source, из него нужны два файла и лицензия.
$(QCP_FILES):
	mkdir -p $(QCP_DIR)
	curl -fsSL $(QCP_URL) | tar xzf - -C $(QCP_DIR) --strip-components=1 \
		qcustomplot-source/qcustomplot.h \
		qcustomplot-source/qcustomplot.cpp \
		qcustomplot-source/GPL.txt

build: deps
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(QMAKE) ../monitor.pro
	$(MAKE) -C $(BUILD_DIR)

run: build
	$(APP) $(if $(PORT),--port $(PORT) --baud $(BAUD))

clean:
	rm -rf $(BUILD_DIR)
