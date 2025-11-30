# Makefile for PixelOS Bootloader

ASM = nasm
CC = gcc
LD = ld

BOOT_DIR = boot
BUILD_DIR = build
DISK_DIR = disk
KERNEL_DIR = kernel
LINKER_DIR = linker

.PHONY: all run clean

# make all 명령
all: $(DISK_DIR)/hddisk.img

$(BUILD_DIR)/boot.bin: $(BOOT_DIR)/boot.asm
	@echo "==> Assembling bootloader..."
	$(ASM) $(BOOT_DIR)/boot.asm -f bin -o $(BUILD_DIR)/boot.bin

$(BUILD_DIR)/kernel_entry.o: $(KERNEL_DIR)/kernel_entry.asm
	@echo "==> Assembling kernel entry..."
	$(ASM) $(KERNEL_DIR)/kernel_entry.asm -f elf32 -o $(BUILD_DIR)/kernel_entry.o

$(BUILD_DIR)/kernel.o: $(KERNEL_DIR)/kernel.c
	@echo "==> Compiling kernel..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(KERNEL_DIR)/kernel.c \
	-o $(BUILD_DIR)/kernel.o

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel_entry.o $(BUILD_DIR)/kernel.o
	@echo "==> Linking kernel..."
	$(LD) -m elf_i386 -T $(LINKER_DIR)/linker.ld \
	$(BUILD_DIR)/kernel_entry.o \
	$(BUILD_DIR)/kernel.o \
	-o $(BUILD_DIR)/kernel.bin

$(DISK_DIR)/hddisk.img: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin
	@echo "==> Creating HDD image..."
	dd if=/dev/zero of=$(DISK_DIR)/hddisk.img bs=512 count=20480
	dd if=$(BUILD_DIR)/boot.bin of=$(DISK_DIR)/hddisk.img bs=512 count=1 conv=notrunc
	dd if=$(BUILD_DIR)/kernel.bin of=$(DISK_DIR)/hddisk.img bs=512 seek=1 conv=notrunc

# make run 명령
run:
	qemu-system-i386 -hda $(DISK_DIR)/hddisk.img

# make clean 명령
clean:
	rm -f $(BUILD_DIR)/*.bin
	rm -f $(BUILD_DIR)/*.o
	rm -f $(DISK_DIR)/hddisk.img
