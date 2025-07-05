#!/usr/bin/env python3
"""
Delta upload script for STM32F427 with PlatformIO
"""

import os
import sys
import subprocess
import hashlib
import json
import tempfile
from pathlib import Path

# Try to import bsdiff4
try:
    import bsdiff4

    HAS_BSDIFF = True
except ImportError:
    HAS_BSDIFF = False
    print("Installing bsdiff4...")
    subprocess.run([sys.executable, "-m", "pip", "install", "bsdiff4"])
    try:
        import bsdiff4

        HAS_BSDIFF = True
    except ImportError:
        HAS_BSDIFF = False


class STM32DeltaUploader:
    def __init__(self, project_dir):
        self.project_dir = Path(project_dir)
        self.cache_dir = self.project_dir / ".pio_cache"
        self.cache_dir.mkdir(exist_ok=True)

        # STM32F427 specific settings
        self.flash_base = 0x08000000
        self.sector_size = 128 * 1024  # 128KB sectors for most of flash
        self.block_size = 4096  # 4KB blocks for comparison

    def get_firmware_info(self, elf_path):
        """Extract firmware info using objcopy"""
        bin_path = elf_path.replace(".elf", ".bin")
        hex_path = elf_path.replace(".elf", ".hex")

        # Convert ELF to BIN
        subprocess.run(["arm-none-eabi-objcopy", "-O", "binary", elf_path, bin_path], check=True)

        # Get size info
        result = subprocess.run(["arm-none-eabi-size", elf_path], capture_output=True, text=True)

        return bin_path, hex_path, result.stdout

    def calculate_hash(self, file_path):
        """Calculate SHA256 hash"""
        sha256_hash = hashlib.sha256()
        with open(file_path, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                sha256_hash.update(chunk)
        return sha256_hash.hexdigest()

    def create_block_diff(self, old_bin, new_bin):
        """Create block-based diff for STM32"""
        with open(old_bin, "rb") as f:
            old_data = f.read()
        with open(new_bin, "rb") as f:
            new_data = f.read()

        # Pad to block size
        max_size = max(len(old_data), len(new_data))
        padded_size = ((max_size + self.block_size - 1) // self.block_size) * self.block_size

        old_padded = old_data + b"\xFF" * (padded_size - len(old_data))
        new_padded = new_data + b"\xFF" * (padded_size - len(new_data))

        changed_blocks = []
        total_changed_size = 0

        for i in range(0, padded_size, self.block_size):
            old_block = old_padded[i : i + self.block_size]
            new_block = new_padded[i : i + self.block_size]

            if old_block != new_block:
                changed_blocks.append({"offset": i, "address": self.flash_base + i, "size": self.block_size, "data": new_block})
                total_changed_size += self.block_size

        return changed_blocks, total_changed_size

    def create_binary_diff(self, old_bin, new_bin):
        """Create binary diff using bsdiff"""
        if not HAS_BSDIFF:
            return None, 0

        try:
            with open(old_bin, "rb") as f:
                old_data = f.read()
            with open(new_bin, "rb") as f:
                new_data = f.read()

            delta = bsdiff4.diff(old_data, new_data)
            return delta, len(delta)

        except Exception as e:
            print(f"Binary diff failed: {e}")
            return None, 0

    def choose_best_update_method(self, old_bin, new_bin, custom_bootloader=False):
        """Choose the most efficient update method"""
        with open(new_bin, "rb") as f:
            new_size = len(f.read())

        # Try block diff
        changed_blocks, block_size = self.create_block_diff(old_bin, new_bin)

        # Try binary diff only if custom bootloader is available
        binary_diff = None
        diff_size = 0
        if custom_bootloader:
            binary_diff, diff_size = self.create_binary_diff(old_bin, new_bin)

        print(f"Update analysis:")
        print(f"  Full firmware: {new_size} bytes")
        print(f"  Block update: {block_size} bytes ({len(changed_blocks)} blocks)")
        if custom_bootloader and binary_diff:
            print(f"  Binary diff: {diff_size} bytes")
        else:
            print(f"  Binary diff: Not available (needs custom bootloader)")

        # Choose best method - never use binary diff without custom bootloader
        if custom_bootloader and binary_diff and diff_size < min(block_size, new_size * 0.8):
            return "binary_diff", binary_diff, diff_size
        elif block_size < new_size * 0.6:
            return "block_update", changed_blocks, block_size
        else:
            return "full_update", new_bin, new_size

    def upload_with_openocd(self, method, data, bin_path=None, custom_bootloader=False, upload_interface="stlink"):
        """Upload using OpenOCD"""

        # Create OpenOCD config
        openocd_cfg = f"""
# STM32F427 OpenOCD configuration
source [find interface/{upload_interface}.cfg]
source [find target/stm32f4x.cfg]

init
reset halt
"""

        if method == "full_update":
            # Standard full flash
            openocd_cfg += f"""
flash write_image erase {data} 0x08000000
reset run
shutdown
"""
        elif method == "block_update":
            # Block-based update
            temp_dir = tempfile.mkdtemp()

            for i, block in enumerate(data):
                block_file = Path(temp_dir) / f"block_{i:04d}.bin"
                with open(block_file, "wb") as f:
                    f.write(block["data"])

                openocd_cfg += f"""
flash write_image {block_file} 0x{block['address']:08x}
"""

            openocd_cfg += """
reset run
shutdown
"""
        elif method == "binary_diff":
            # Binary diff - send to custom bootloader
            if not custom_bootloader:
                print("Error: Binary diff selected but custom bootloader not enabled!")
                return False
            return self.send_delta_to_bootloader(data)

        # Write OpenOCD script
        script_file = self.cache_dir / "upload_script.cfg"
        with open(script_file, "w") as f:
            f.write(openocd_cfg)

        # Run OpenOCD
        try:
            cmd = ["openocd", "-f", str(script_file)]
            result = subprocess.run(cmd, check=True, capture_output=True, text=True)
            print("Upload successful!")
            return True
        except subprocess.CalledProcessError as e:
            print(f"OpenOCD upload failed: {e}")
            print(f"Output: {e.stdout}")
            print(f"Error: {e.stderr}")
            return False

    def upload_with_stlink(self, method, data, bin_path=None, custom_bootloader=False):
        """Upload using st-flash"""

        if method == "full_update":
            try:
                cmd = ["st-flash", "write", data, "0x08000000"]
                result = subprocess.run(cmd, check=True, capture_output=True, text=True)
                print("Upload successful!")
                return True
            except subprocess.CalledProcessError as e:
                print(f"st-flash upload failed: {e}")
                return False

        elif method == "block_update":
            # st-flash doesn't support block updates efficiently
            print("Block updates not supported with st-flash, using OpenOCD")
            return self.upload_with_openocd(method, data, bin_path, custom_bootloader)

        elif method == "binary_diff":
            # Binary diff - send to custom bootloader
            if not custom_bootloader:
                print("Error: Binary diff selected but custom bootloader not enabled!")
                return False
            return self.send_delta_to_bootloader(data)

        return False

    def upload_firmware(self, elf_path, upload_method="openocd", custom_bootloader=False):
        """Main upload function"""

        # Convert ELF to BIN
        bin_path, hex_path, size_info = self.get_firmware_info(elf_path)
        print(f"Firmware size info:\n{size_info}")

        # Check if firmware changed
        firmware_hash = self.calculate_hash(bin_path)
        env_name = os.getenv("PIOENV", "default")
        hash_cache_file = self.cache_dir / f"{env_name}_hash.txt"

        if hash_cache_file.exists():
            previous_hash = hash_cache_file.read_text().strip()
            if firmware_hash == previous_hash:
                print("Firmware unchanged - skipping upload")
                return True

        # Look for previous firmware
        previous_bin = self.cache_dir / f"{env_name}_previous.bin"

        if previous_bin.exists():
            # Determine best update method
            method, data, size = self.choose_best_update_method(str(previous_bin), bin_path, custom_bootloader)
            print(f"Using {method} ({size} bytes)")
        else:
            method, data, size = "full_update", bin_path, os.path.getsize(bin_path)
            print(f"First upload - using full update ({size} bytes)")

        # Perform upload
        success = False
        if upload_method == "openocd":
            success = self.upload_with_openocd(method, data, bin_path, custom_bootloader)
        elif upload_method == "stlink":
            success = self.upload_with_stlink(method, data, bin_path, custom_bootloader)

        if success:
            # Save current firmware for next comparison
            import shutil

            shutil.copy(bin_path, previous_bin)
            hash_cache_file.write_text(firmware_hash)

        return success

    def send_delta_to_bootloader(self, delta_data):
        """Send delta data to custom bootloader via UART"""
        try:
            # This would implement your communication protocol
            # Example for UART communication with the bootloader
            import serial

            # Try to find the bootloader UART port
            uart_port = "/dev/ttyACM0"  # Adjust as needed

            print(f"Connecting to bootloader on {uart_port}...")
            ser = serial.Serial(uart_port, 115200, timeout=5)

            # Send delta update command
            ser.write(b"\xAA\x11\x00\x55")  # CMD_DELTA_UPDATE packet
            response = ser.read(100)

            if b"READY" in response:
                print("Bootloader ready for delta update")

                # Send delta size
                delta_size = len(delta_data)
                size_packet = b"\xAA\x12\x04" + delta_size.to_bytes(4, "little") + b"\x55"
                ser.write(size_packet)

                # Send delta data in chunks
                chunk_size = 128
                for i in range(0, len(delta_data), chunk_size):
                    chunk = delta_data[i : i + chunk_size]
                    data_packet = b"\xAA\x12" + len(chunk).to_bytes(1, "little") + chunk + b"\x55"
                    ser.write(data_packet)

                    # Simple progress indicator
                    progress = (i + len(chunk)) * 100 // len(delta_data)
                    print(f"Uploading delta: {progress}%", end="\r")

                print("\nDelta upload complete, waiting for response...")

                # Wait for completion
                result = ser.read(100)
                ser.close()

                if b"SUCCESS" in result:
                    print("Delta update successful!")
                    return True
                else:
                    print("Delta update failed!")
                    return False
            else:
                print("Bootloader not responding")
                ser.close()
                return False

        except ImportError:
            print("pyserial not installed. Install with: pip install pyserial")
            return False
        except Exception as e:
            print(f"Bootloader communication failed: {e}")
            return False

    def _upload_delta(self, delta_file, upload_port, upload_speed):
        """Upload delta file (requires custom bootloader support)"""
        # Read delta file
        try:
            with open(delta_file, "rb") as f:
                delta_data = f.read()
            return self.send_delta_to_bootloader(delta_data)
        except Exception as e:
            print(f"Failed to read delta file: {e}")
            return False


def main():
    if len(sys.argv) < 2:
        print("Usage: stm32_delta_upload.py <firmware.elf> [upload_method] [custom_bootloader]")
        print("  upload_method: openocd or stlink (default: openocd)")
        print("  custom_bootloader: true or false (default: false)")
        print()
        print("Examples:")
        print("  python stm32_delta_upload.py firmware.elf                    # Standard block updates")
        print("  python stm32_delta_upload.py firmware.elf openocd true       # Enable delta updates")
        sys.exit(1)

    elf_path = sys.argv[1]
    upload_method = sys.argv[2] if len(sys.argv) > 2 else "openocd"
    custom_bootloader = sys.argv[3].lower() == "true" if len(sys.argv) > 3 else False

    if custom_bootloader:
        print("Custom bootloader mode: Delta updates enabled")
    else:
        print("Standard mode: Block updates only")

    uploader = STM32DeltaUploader(os.getcwd())
    success = uploader.upload_firmware(elf_path, upload_method, custom_bootloader)

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
