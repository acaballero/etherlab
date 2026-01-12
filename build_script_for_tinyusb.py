# extra_tinyusb.py
# Place in project root directory
# Forces TinyUSB source files to be compiled with correct CPU flags

Import("env")
import os

print("=" * 60)
print("TinyUSB Build Script - Adding source files...")
print("=" * 60)

# TinyUSB source files
tinyusb_sources = [
    "lib/tinyusb/src/tusb.c",
    "lib/tinyusb/src/common/tusb_fifo.c",
    "lib/tinyusb/src/device/usbd.c",
    "lib/tinyusb/src/device/usbd_control.c",
    "lib/tinyusb/src/class/audio/audio_device.c",
    "lib/tinyusb/src/class/cdc/cdc_device.c",
    "lib/tinyusb/src/class/msc/msc_device.c",
    "lib/tinyusb/src/portable/synopsys/dwc2/dcd_dwc2.c",
    "lib/tinyusb/src/portable/synopsys/dwc2/dwc2_common.c",
]

# Add include path
env.Append(CPPPATH=["lib/tinyusb/src"])

# Get flags but remove C++ standard
base_ccflags = [f for f in env.get("CCFLAGS", []) if "-std=gnu++" not in str(f) and "-std=c++" not in str(f)]

# CRITICAL: Add Cortex-M4 Thumb flags for TinyUSB
tinyusb_cflags = [
    "-std=c11",
    "-mthumb",  # Use Thumb mode (required for Cortex-M)
    "-mcpu=cortex-m4",  # Target Cortex-M4
    "-mfpu=fpv4-sp-d16",  # FPU support
    "-mfloat-abi=hard",  # Hard float ABI
    "-Og",
    "-ggdb3",
]

# Compile each file
for src in tinyusb_sources:
    if os.path.exists(src):
        obj = env.Object(
            target=os.path.join("$BUILD_DIR", "tinyusb", os.path.basename(src).replace(".c", ".o")),
            source=src,
            CCFLAGS=base_ccflags,
            CFLAGS=tinyusb_cflags,
        )
        env.Append(PIOBUILDFILES=[obj])
        print(f"  ✓ {src}")
    else:
        print(f"  ✗ NOT FOUND: {src}")

print("=" * 60)
print("TinyUSB: Configured with Cortex-M4 Thumb mode")
print("=" * 60)
