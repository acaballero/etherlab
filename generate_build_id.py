import subprocess
import os
from datetime import datetime

Import("env")


def get_git_version():
    try:
        version = subprocess.check_output(["git", "describe", "--tags", "--always"], stderr=subprocess.STDOUT).strip().decode("utf-8")
        return version
    except Exception:
        return "unknown"


def generate_version_file():
    # Generate version info
    build_date_time = datetime.utcnow().strftime("%Y%m%d%H%M%S")
    git_version = get_git_version()
    version_string = f"{build_date_time} - {git_version}"

    # Generate version.cpp content
    version_cpp_content = f"""// Auto-generated file - DO NOT EDIT
#include "version.h"

const char* get_build_version() {{
    return "{version_string}";
}}

const char* get_build_time() {{
    return __DATE__ " " __TIME__;
}}
"""

    # Write version.cpp
    version_cpp_path = "src/version.cpp"
    with open(version_cpp_path, "w") as f:
        f.write(version_cpp_content)

    print(f"Generated version.cpp with: {version_string}")
    return version_cpp_path


# Generate the version file before build
version_file = generate_version_file()

# Ensure version.cpp is always compiled
env.Depends("$BUILD_DIR/src/version.o", version_file)
