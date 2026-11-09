import os


def increment_version(version_file):
    with open(version_file, "r") as file:
        version = file.read().strip()

    major, minor, patch = map(int, version.split("."))
    patch += 1
    new_version = f"{major}.{minor}.{patch}"

    with open(version_file, "w") as file:
        file.write(f"{new_version}\n")

    return new_version


def write_version_header(include_dir, version):
    version_header = os.path.join(include_dir, "version.h")
    with open(version_header, "w") as file:
        file.write("#pragma once\n")
        file.write(f'#define VERSION "{version}"\n')


def before_build(env):
    version_file = os.path.join(env["PROJECT_DIR"], "version.txt")
    include_dir = os.path.join(env["PROJECT_DIR"], "include")
    if os.path.exists(version_file):
        new_version = increment_version(version_file)
        write_version_header(include_dir, new_version)
        print(f"Updated firmware version to: {new_version}")


Import("env")
before_build(env)