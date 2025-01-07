import subprocess
from datetime import datetime

Import("env")


def get_git_version():
    try:
        version = (
            subprocess.check_output(
                ["git", "describe", "--tags", "--always"], stderr=subprocess.STDOUT
            )
            .strip()
            .decode("utf-8")
        )
        return version
    except Exception:
        return "unknown"


# Generate the current date-time in RFC3339 format
build_date_time = datetime.utcnow().strftime("%Y%m%d%H%M%S")

git_version = get_git_version()

version_string = f'\\"{build_date_time} - {git_version}\\"'

print(version_string)

# Add it as a build flag
env.Append(CPPDEFINES=[("BUILD_VERSION", version_string)])
