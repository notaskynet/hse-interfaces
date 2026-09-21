#   make         собрать
#   make run     собрать и запустить
#   make ports   поднять виртуальную пару портов для проверки без платы
#   make clean   удалить сборку

QMAKE     ?= qmake
BUILD_DIR := terminal/build

ifeq ($(shell uname),Darwin)
APP := $(BUILD_DIR)/terminal.app/Contents/MacOS/terminal
else
APP := $(BUILD_DIR)/terminal
endif

.PHONY: all build run ports clean

all: build

$(BUILD_DIR)/Makefile: terminal/terminal.pro
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(QMAKE) ../terminal.pro

build: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)

run: build
	$(APP)

# Печатает два пути /dev/ttys00N: один вписать в поле "Порт", второй читать/писать из консоли.
ports:
	socat -d -d pty,raw,echo=0 pty,raw,echo=0

clean:
	rm -rf $(BUILD_DIR)
