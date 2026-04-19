# Makefile for PixelOS Bootloader

ASM = nasm
CC = gcc
LD = ld
OBJCOPY = objcopy

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
APP_DIR = app

KERNEL_OBJS = \
	$(BUILD_DIR)/kernel_entry.o \
	$(BUILD_DIR)/kernel.o \
	$(BUILD_DIR)/heap.o \
	$(BUILD_DIR)/paging.o \
	$(BUILD_DIR)/font.o \
	$(BUILD_DIR)/idt.o \
	$(BUILD_DIR)/interrupts.o \
	$(BUILD_DIR)/event_queue.o \
	$(BUILD_DIR)/task.o \
	$(BUILD_DIR)/task_switch.o \
	$(BUILD_DIR)/timer.o \
	$(BUILD_DIR)/keyboard.o \
	$(BUILD_DIR)/mouse.o \
	$(BUILD_DIR)/cursor.o \
	$(BUILD_DIR)/graphics.o \
	$(BUILD_DIR)/window.o \
	$(BUILD_DIR)/layer_manager.o \
	$(BUILD_DIR)/compositor.o \
	$(BUILD_DIR)/flush_gdt.o \
	$(BUILD_DIR)/gdt.o \
	$(BUILD_DIR)/user_mode.o \
	$(BUILD_DIR)/game_window.o \
	$(BUILD_DIR)/pong.o

CFLAGS = -m32 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib -fno-pic
ASFLAGS_BIN = -f bin
ASFLAGS_ELF = -f elf32

define COMPILE_C_TEMPLATE
$(BUILD_DIR)/%.o: $(1)/%.c | $(BUILD_DIR)
	@echo "==> Compiling $$<..."
	$(CC) $(CFLAGS) -c $$< -o $$@
endef

define COMPILE_ASM_ELF_TEMPLATE
$(BUILD_DIR)/%.o: $(1)/%.asm | $(BUILD_DIR)
	@echo "==> Assembling $$<..."
	$(ASM) $$< $(ASFLAGS_ELF) -o $$@
endef

$(eval $(call COMPILE_C_TEMPLATE,$(KERNEL_DIR)))
$(eval $(call COMPILE_C_TEMPLATE,$(MEMORY_DIR)))
$(eval $(call COMPILE_C_TEMPLATE,$(FONT_DIR)))
$(eval $(call COMPILE_C_TEMPLATE,$(KEYBOARD_DIR)))
$(eval $(call COMPILE_C_TEMPLATE,$(MOUSE_DIR)))
$(eval $(call COMPILE_C_TEMPLATE,$(GRAPHICS_DIR)))
$(eval $(call COMPILE_C_TEMPLATE,$(WINDOW_DIR)))
$(eval $(call COMPILE_C_TEMPLATE,$(APP_DIR)))

$(eval $(call COMPILE_ASM_ELF_TEMPLATE,$(KERNEL_DIR)))

.PHONY: all run clean

# make all 명령
all: $(DISK_DIR)/hddisk.img

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(DISK_DIR):
	mkdir -p $(DISK_DIR)

$(BUILD_DIR)/boot.bin: $(BOOT_DIR)/boot.asm | $(BUILD_DIR)
	@echo "==> Assembling bootloader..."
	$(ASM) $< $(ASFLAGS_BIN) -o $@

# 커널 링크
$(BUILD_DIR)/kernel.bin: $(KERNEL_OBJS) | $(BUILD_DIR)
	@echo "==> Linking kernel..."
	$(LD) -m elf_i386 -T $(LINKER_DIR)/linker.ld \
	$(KERNEL_OBJS) \
	-o $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/kernel.bin

$(DISK_DIR)/hddisk.img: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin | $(DISK_DIR)
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