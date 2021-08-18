import os
import sys
import platform
import subprocess
import setuptools
import pathlib
import sysconfig
import copy
import distutils
from pathlib import Path
from pkg_resources import Distribution, get_distribution
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext, copy_file
from distutils import log

from distutils.version import LooseVersion


MIN_SETUPTOOLS_VERSION = "31.0.0"
assert (LooseVersion(setuptools.__version__) >= LooseVersion(MIN_SETUPTOOLS_VERSION)), "QBDL requires a setuptools version '{}' or higher (pip install setuptools --upgrade)".format(MIN_SETUPTOOLS_VERSION)

CURRENT_DIR = Path(__file__).parent
SOURCE_DIR  = CURRENT_DIR / ".." / ".."
PACKAGE_NAME = "pyqbdl"

class PyQbdlDistribution(setuptools.Distribution):
    global_options = setuptools.Distribution.global_options + [
        ('ninja', None, 'Use Ninja as build system'),
        ('lief-dir=', None, 'Path to the directory that contains LIEFConfig.cmake'),
        ('spdlog-dir=', None, 'Path to the directory that contains spdlogConfig.cmake'),
    ]

    def __init__(self, attrs=None):
        self.ninja = False
        self.lief_dir = None
        self.spdlog_dir = None
        super().__init__(attrs)


class Module(Extension):
    def __init__(self, name, sourcedir='', *args, **kwargs):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = SOURCE_DIR


class BuildLibrary(build_ext):
    def run(self):
        try:
            subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        for ext in self.extensions:
            self.build_extension(ext)
        self.copy_extensions_to_source()

    @staticmethod
    def has_ninja():
        try:
            subprocess.check_call(['ninja', '--version'])
            return True
        except Exception:
            return False

    def build_extension(self, ext):
        fullname = self.get_ext_fullname(ext.name)
        filename = self.get_ext_filename(fullname)

        cmake_args = []

        source_dir = ext.sourcedir
        build_temp = Path(self.build_temp).absolute()
        build_temp.mkdir(parents=True, exist_ok=True)

        extdir                         = Path(self.get_ext_fullpath(ext.name)).parent
        cmake_library_output_directory = build_temp.parent.absolute()
        cfg                            = 'Debug' if self.debug else 'Release'
        is64                           = sys.maxsize > 2**32

        # Ninja ?
        build_with_ninja = False
        if self.has_ninja() and self.distribution.ninja:
            build_with_ninja = True

        if build_with_ninja:
            cmake_args += ["-G", "Ninja"]

        cmake_args += [
            '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={}'.format(cmake_library_output_directory),
            '-DPython3_ROOT_DIR={}'.format(sys.prefix),
            '-DPython3_FIND_STRATEGY=LOCATION',
            '-DQBDL_BUILD_EXAMPLES=OFF',
            '-DQBDL_BUILD_DOCS=OFF',
            '-DQBDL_PYTHON_BINDING=on',
            '-DCMAKE_BUILD_TYPE={}'.format(cfg),
        ]
        if self.distribution.lief_dir is not None:
            cmake_args.append("-DLIEF_DIR={}".format(self.distribution.lief_dir))

        if self.distribution.spdlog_dir is not None:
            cmake_args.append("-Dspdlog_DIR={}".format(self.distribution.spdlog_dir))


        build_args = ['--config', cfg]

        env = os.environ

        if os.getenv("CXXFLAGS", None) is not None:
            cmake_args += [
                '-DCMAKE_CXX_FLAGS={}'.format(os.getenv("CXXFLAGS")),
            ]

        if os.getenv("CFLAGS", None) is not None:
            cmake_args += [
                '-DCMAKE_C_FLAGS={}'.format(os.getenv("CFLAGS")),
            ]



        log.info("Platform: %s", platform.system())
        log.info("Wheel library: %s", self.get_ext_fullname(ext.name))

        # 1. Configure
        configure_cmd = ['cmake', ext.sourcedir.absolute().as_posix()] + cmake_args
        log.info(" ".join(configure_cmd))
        subprocess.check_call(configure_cmd, cwd=self.build_temp, env=env)

        subprocess.check_call(['cmake', "--build", ".","--config",cfg], cwd=self.build_temp, env=env)

        if platform.system() == "Windows":
            cmake_library_output_directory /= cfg


        py_lib_dst  = Path(self.build_lib) / self.get_ext_filename(self.get_ext_fullname(ext.name))
        libsuffix = py_lib_dst.name.split(".")[-1]

        py_lib_path = cmake_library_output_directory / "{}.{}".format(PACKAGE_NAME, libsuffix)

        Path(self.build_lib).mkdir(exist_ok=True)

        log.info("Copying {} into {}".format(py_lib_path, py_lib_dst))
        copy_file(py_lib_path, py_lib_dst, verbose=self.verbose, dry_run=self.dry_run)

# From setuptools-git-version
command       = 'git describe --tags --long --dirty'
is_tagged_cmd = 'git tag --list --points-at=HEAD'
fmt_dev       = '{tag}.dev0'
fmt_tagged    = '{tag}'

def format_version(version: str, fmt: str = fmt_dev, is_dev: bool = False):
    parts = version.split('-')
    assert len(parts) in (3, 4)
    dirty = len(parts) == 4
    tag, count, sha = parts[:3]
    MA, MI, PA = map(int, tag.split(".")) # 0.9.0 -> (0, 9, 0)

    if is_dev:
        tag = "{}.{}.{}".format(MA, MI + 1, 0)

    if count == '0' and not dirty:
        return tag
    return fmt.format(tag=tag, gitsha=sha.lstrip('g'))


def get_git_version(is_tagged: bool) -> str:
    git_version = subprocess.check_output(command.split()).decode('utf-8').strip()
    if is_tagged:
        return format_version(version=git_version, fmt=fmt_tagged)
    return format_version(version=git_version, fmt=fmt_dev, is_dev=True)

def check_if_tagged() -> bool:
    output = subprocess.check_output(is_tagged_cmd.split()).decode('utf-8').strip()
    return output != ""

def get_pkg_info_version(pkg_info_file):
    pkg = get_distribution(PACKAGE_NAME)
    return pkg.version


def get_version() -> str:
    version   = "0.2.0"
    pkg_info  = os.path.join(CURRENT_DIR, "{}.egg-info".format(PACKAGE_NAME), "PKG-INFO")
    git_dir   = os.path.join(CURRENT_DIR, ".git")
    if os.path.isdir(git_dir):
        is_tagged = False
        try:
            is_tagged = check_if_tagged()
        except Exception:
            is_tagged = False

        try:
            return get_git_version(is_tagged)
        except Exception:
            pass

    if os.path.isfile(pkg_info):
        return get_pkg_info_version(pkg_info)

    return version

version = get_version()
cmdclass = {
    'build_ext': BuildLibrary,
}

LIEF_VERSION = "0.11.5"

setup(
    distclass=PyQbdlDistribution,
    ext_modules=[Module(PACKAGE_NAME)],
    cmdclass=cmdclass,
    version=version,
    install_requires=["lief=={}".format(LIEF_VERSION)],
)
