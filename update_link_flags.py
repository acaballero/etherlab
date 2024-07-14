# Custom settings, as referred to as "extra_script" in platformio.ini
#
# See http://docs.platformio.org/en/latest/projectconf.html#extra-script

from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

env.Append(
    LINKFLAGS=[
        "-mcpu=cortex-m4",
        "-mthumb",
        "-mthumb-interwork",
        "-mfloat-abi=hard",
        "-mfpu=fpv4-sp-d16",
        "-ffast-math",
        "-fno-math-errno"


    ]
)