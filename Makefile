# Makefile for PixelOS Bootloader

ASM = nasm
CC = gcc
LD = ld

BOOT_DIR = boot
BUILD_DIR = build
DISK_DIR = disk
KERNEL_DIR = kernel
LINKER_DIR = linker

FONT_DIR = font
KEYBOARD_DIR = keyboard
MOUSE_DIR = mouse

GRAPHICS_DIR = graphics
WINDOW_DIR = window

MEMORY_DIR = memory

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

$(BUILD_DIR)/heap.o: $(MEMORY_DIR)/heap.c
	@echo "==> Compiling heap..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(MEMORY_DIR)/heap.c \
	-o $(BUILD_DIR)/heap.o

$(BUILD_DIR)/paging.o: $(MEMORY_DIR)/paging.c
	@echo "==> Compiling paging module..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(MEMORY_DIR)/paging.c \
	-o $(BUILD_DIR)/paging.o

$(BUILD_DIR)/font.o: $(FONT_DIR)/font.c
	@echo "==> Compiling font..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(FONT_DIR)/font.c \
	-o $(BUILD_DIR)/font.o

$(BUILD_DIR)/idt.o: $(KERNEL_DIR)/idt.asm
	@echo "==> Assembling IDT..."
	$(ASM) $(KERNEL_DIR)/idt.asm -f elf32 -o $(BUILD_DIR)/idt.o

$(BUILD_DIR)/interrupts.o: $(KERNEL_DIR)/interrupts.c
	@echo "==> Compiling interrupts..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(KERNEL_DIR)/interrupts.c \
	-o $(BUILD_DIR)/interrupts.o

$(BUILD_DIR)/event_queue.o: $(KERNEL_DIR)/event_queue.c
	@echo "==> Compiling event queue..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(KERNEL_DIR)/event_queue.c \
	-o $(BUILD_DIR)/event_queue.o

$(BUILD_DIR)/task_switch.o: $(KERNEL_DIR)/task_switch.asm
	@echo "==> Assembling task switch..."
	$(ASM) $(KERNEL_DIR)/task_switch.asm -f elf32 -o $(BUILD_DIR)/task_switch.o

$(BUILD_DIR)/task.o: $(KERNEL_DIR)/task.c
	@echo "==> Compiling task..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(KERNEL_DIR)/task.c \
	-o $(BUILD_DIR)/task.o

$(BUILD_DIR)/timer.o: $(KERNEL_DIR)/timer.c
	@echo "==> Compiling timer..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(KERNEL_DIR)/timer.c \
	-o $(BUILD_DIR)/timer.o

$(BUILD_DIR)/keyboard.o: $(KEYBOARD_DIR)/keyboard.c
	@echo "==> Compiling keyboard input dirver..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(KEYBOARD_DIR)/keyboard.c \
	-o $(BUILD_DIR)/keyboard.o

$(BUILD_DIR)/mouse.o: $(MOUSE_DIR)/mouse.c
	@echo "==> Compiling mouse input dirver..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(MOUSE_DIR)/mouse.c \
	-o $(BUILD_DIR)/mouse.o

$(BUILD_DIR)/cursor.o: $(MOUSE_DIR)/cursor.c
	@echo "==> Compiling cursor..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(MOUSE_DIR)/cursor.c \
	-o $(BUILD_DIR)/cursor.o

$(BUILD_DIR)/graphics.o: $(GRAPHICS_DIR)/graphics.c
	@echo "==> Compiling graphics module..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(GRAPHICS_DIR)/graphics.c \
	-o $(BUILD_DIR)/graphics.o

$(BUILD_DIR)/layer_manager.o: $(GRAPHICS_DIR)/layer_manager.c
	@echo "==> Compiling layer manager..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(GRAPHICS_DIR)/layer_manager.c \
	-o $(BUILD_DIR)/layer_manager.o

$(BUILD_DIR)/compositor.o: $(GRAPHICS_DIR)/compositor.c
	@echo "==> Compiling composite module..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(GRAPHICS_DIR)/compositor.c \
	-o $(BUILD_DIR)/compositor.o

$(BUILD_DIR)/window.o: $(WINDOW_DIR)/window.c
	@echo "==> Compiling window manager..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(WINDOW_DIR)/window.c \
	-o $(BUILD_DIR)/window.o

$(BUILD_DIR)/flush_gdt.o: $(KERNEL_DIR)/flush_gdt.asm
	@echo "==> Assembling GDT/TSS helpers..."
	$(ASM) $(KERNEL_DIR)/flush_gdt.asm -f elf32 -o $(BUILD_DIR)/flush_gdt.o

$(BUILD_DIR)/gdt.o: $(KERNEL_DIR)/gdt.c
	@echo "==> Compiling GDT module..."
	$(CC) -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
	-c $(KERNEL_DIR)/gdt.c \
	-o $(BUILD_DIR)/gdt.o

$(BUILD_DIR)/user_mode.o: $(KERNEL_DIR)/user_mode.asm
	@echo "==> Assembling user mode helper..."
	$(ASM) $(KERNEL_DIR)/user_mode.asm -f elf32 -o $(BUILD_DIR)/user_mode.o

# 커널 링크
$(BUILD_DIR)/kernel.bin: \
    $(BUILD_DIR)/kernel_entry.o $(BUILD_DIR)/kernel.o \
	$(BUILD_DIR)/heap.o $(BUILD_DIR)/paging.o \
    $(BUILD_DIR)/font.o \
    $(BUILD_DIR)/idt.o $(BUILD_DIR)/interrupts.o $(BUILD_DIR)/event_queue.o $(BUILD_DIR)/task.o $(BUILD_DIR)/task_switch.o \
	$(BUILD_DIR)/timer.o \
    $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/mouse.o $(BUILD_DIR)/cursor.o \
	$(BUILD_DIR)/graphics.o $(BUILD_DIR)/window.o $(BUILD_DIR)/layer_manager.o $(BUILD_DIR)/compositor.o \
	$(BUILD_DIR)/flush_gdt.o $(BUILD_DIR)/gdt.o \
	$(BUILD_DIR)/user_mode.o
	@echo "==> Linking kernel..."
	$(LD) -m elf_i386 -T $(LINKER_DIR)/linker.ld \
	$(BUILD_DIR)/kernel_entry.o $(BUILD_DIR)/kernel.o \
	$(BUILD_DIR)/heap.o $(BUILD_DIR)/paging.o \
	$(BUILD_DIR)/task.o $(BUILD_DIR)/task_switch.o \
	$(BUILD_DIR)/timer.o \
	$(BUILD_DIR)/font.o \
	$(BUILD_DIR)/idt.o $(BUILD_DIR)/interrupts.o $(BUILD_DIR)/event_queue.o \
	$(BUILD_DIR)/keyboard.o $(BUILD_DIR)/mouse.o $(BUILD_DIR)/cursor.o \
	$(BUILD_DIR)/graphics.o $(BUILD_DIR)/window.o $(BUILD_DIR)/layer_manager.o $(BUILD_DIR)/compositor.o \
	$(BUILD_DIR)/flush_gdt.o $(BUILD_DIR)/gdt.o \
	$(BUILD_DIR)/user_mode.o \
	-o $(BUILD_DIR)/kernel.elf
	objcopy -O binary $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/kernel.bin


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
	rm -f $(BUILD_DIR)/*.elf
	rm -f $(BUILD_DIR)/*.o
	rm -f $(DISK_DIR)/hddisk.img
