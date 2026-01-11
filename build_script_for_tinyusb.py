# extra_tinyusb.py
# Place this file in your project root directory
# Add to platformio.ini: extra_scripts = pre:extra_tinyusb.py

Import("env")


# Force TinyUSB library files to compile as C, not C++
def force_c_compilation(node):
    """Force .c files in TinyUSB to compile as C"""
    path = str(node)

    # Check if this is a TinyUSB source file
    if "tinyusb" in path.lower() and path.endswith(".c"):
        # Get the current flags
        flags = env.get("CCFLAGS", [])
        cflags = env.get("CFLAGS", [])

        # Remove C++ flags
        clean_flags = [f for f in flags if not any(x in str(f) for x in ["-std=gnu++", "-std=c++"])]

        # Compile as C (C11)
        env.Object(node, CCFLAGS=clean_flags, CFLAGS=["-std=c11"] + cflags)
        return None  # Prevent default compilation

    return node


# Add the callback
env.AddBuildMiddleware(force_c_compilation)

print("TinyUSB build script loaded: Forcing .c files to compile as C")
