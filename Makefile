# Makefile for PixelOS Bootloader

ASM = nasm

BOOT_DIR = boot
BUILD_DIR = build
DISK_DIR = disk

.PHONY: all run clean

all: $(DISK_DIR)/hddisk.img

$(BUILD_DIR)/boot.bin: $(BOOT_DIR)/boot.asm
	@echo "==> Assembling bootloader..."
	$(ASM) $(BOOT_DIR)/boot.asm -f bin -o $(BUILD_DIR)/boot.bin

$(DISK_DIR)/hddisk.img: $(BUILD_DIR)/boot.bin
	@echo "==> Creating HDD image..."
	dd if=/dev/zero of=$(DISK_DIR)/hddisk.img bs=512 count=20480
	dd if=$(BUILD_DIR)/boot.bin of=$(DISK_DIR)/hddisk.img bs=512 count=1 conv=notrunc

run:
	qemu-system-i386 -hda $(DISK_DIR)/hddisk.img

clean:
	rm -f $(BUILD_DIR)/*.bin
	rm -f $(DISK_DIR)/hddisk.img
