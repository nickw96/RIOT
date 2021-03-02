#!/usr/bin/python3
"""
Command line utility to generate compile_commands.json for RIOT applications
"""
import argparse
import os
import shlex
import subprocess
import sys
import re

TEMPLATE_COMMAND = """  {{
    "arguments": [
      "{compiler}",
      "-DRIOT_FILE_RELATIVE=\\\"{directory}/{source}\\\"",
      "-DRIOT_FILE_NOPATH=\\\"{source}\\\"",
{flags}      "-MQ",
      "{output}",
      "-MD",
      "-MP",
      "-c",
      "-o",
      "{output}",
      "{source}"
    ],
    "directory": "{directory}",
    "file": "{directory}/{source}",
    "output": "{bin_directory}/{output}"
  }}"""

TEMPLATE_FLAG = "      \"{flag}\",\n"

REGEX_VERSION = re.compile(r"\ngcc version ([^ ]+)")
REGEX_INCLUDES = r"^#include <\.\.\.> search starts here:$((?:\n|\r|.)*?)^End of search list\.$"
REGEX_INCLUDES = re.compile(REGEX_INCLUDES, re.MULTILINE)

def detect_includes_and_version_gcc(compiler):
    """
    Runs the given compiler with -v -E on an no-op compilation unit and parses the built-in
    include search directories and the GCC version from the output

    :param compiler: name / path of the compiler to run
    :type compiler: str

    :return: (list_of_include_paths, version)
    :rtype: tuple
    """
    try:
        with subprocess.Popen([compiler, "-v", "-E", "-"],
                stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as proc:
            inputdata = b"typedef int dont_be_pedantic;"
            _, stderrdata = proc.communicate(input=inputdata)
    except FileNotFoundError:
        msg = "Compiler {} not found, not adding system include paths\n".format(compiler)
        sys.stderr.write(msg)
        return []

    stderrdata = stderrdata.decode("utf-8")
    version = REGEX_VERSION.search(stderrdata).group(1)
    includes = [ os.path.abspath(p) for p in REGEX_INCLUDES.search(stderrdata).group(1).split() ]

    return (includes, version)

def detect_libstdcxx_includes(compiler, includes, version):
    """
    Tries to detect the g++ libstdc++ built-in include search directories using black magic and
    adds them to the list given in includes

    :param compiler: Name or path of the compiler
    :type compiler: str
    :param includes: List of include directories
    :type includes: list of str
    :param version: Version of g++
    :type version: str
    """
    for path in includes:
        cxx_lib = os.path.join(path, "c++", version)
        if os.path.exists(cxx_lib):
            includes.append(cxx_lib)
            triple = os.path.basename(compiler)[0:-4]
            cxx_extra = os.path.join(cxx_lib, triple)
            if os.path.exists(cxx_extra):
                includes.append(cxx_extra)
            break

def detect_built_in_includes(compiler, args):
    """
    Tries to detect the built-in include search directories of the given compiler

    :param compiler: Name or path of the compiler
    :type compiler: str
    :param args: Command line arguments

    :return: List of built-in include directories
    :rtype: list of str
    """
    if compiler.endswith('-gcc'):
        includes, version = detect_includes_and_version_gcc(compiler)
    elif compiler.endswith('-g++'):
        includes, version = detect_includes_and_version_gcc(compiler)
        if args.add_libstdcxx_includes:
            detect_libstdcxx_includes(compiler, includes, version)
    elif compiler in ('clang', 'clang++'):
        # clang / clang++ doesn't have any magic include search dirs built in, so we don't need
        # to detect them.
        includes = []
    else:
        msg = "Warning: Cannot detect default include search paths for {}\n".format(compiler)
        sys.stderr.write(msg)
        includes = []
    return includes

class CompilationDetails: # pylint: disable=too-few-public-methods
    """
    Representation of the compilation details stored by RIOT's build system.

    :param path: Path of the module to read compilation details from
    :type path: str
    """
    def __init__(self, path):
        with open(os.path.join(path, "src_c.txt"), "r") as file:
            self.src_c = file.read().split()
        with open(os.path.join(path, "src_cxx.txt"), "r") as file:
            self.src_cxx = file.read().split()
        with open(os.path.join(path, "cflags.txt"), "r") as file:
            self.cflags = shlex.split(file.read())
        with open(os.path.join(path, "cxxflags.txt"), "r") as file:
            self.cxxflags = shlex.split(file.read())
        with open(os.path.join(path, "directory.txt"), "r") as file:
            self.dir = file.read().strip()
        with open(os.path.join(path, "cc.txt"), "r") as file:
            self.cc = file.read().strip() # pylint: disable=invalid-name
        with open(os.path.join(path, "cxx.txt"), "r") as file:
            self.cxx = file.read().strip()

class State: # pylint: disable=too-few-public-methods
    """
    Entity to store the current programs state
    """
    def __init__(self):
        self.def_includes = dict()
        self.flags_to_remove = set()
        self.is_first = True

def get_built_in_include_flags(compiler, state, args):
    """
    Get built-in include search directories as parameter list in JSON format.

    :param compiler: Name or path of the compiler to get the include search dirs from
    :type compiler: str
    :param state: state of the program
    :param args: command line arguments

    :return: The -I<...> compiler flags for the build-in include search dirs in JSON format
    :rtype: str
    """
    result = ""

    if not compiler in state.def_includes:
        state.def_includes[compiler] = detect_built_in_includes(compiler, args)

    for include in state.def_includes[compiler]:
        result += TEMPLATE_FLAG.format(flag='-isystem')
        result += TEMPLATE_FLAG.format(flag=include)

    return result

def generate_module_compile_commands(path, state, args):
    """
    Generate section of compile_commands.json for the module in path and write it to stdout.

    :param path: path of the module's bin folder to emit the compile_commands.json chunk for
    :type path: str
    :param state: state of the program
    :param args: command line arguments
    """
    cdetails = CompilationDetails(path)

    cflags = ""
    cxxflags = ""
    for flag in cdetails.cflags:
        if not flag in state.flags_to_remove:
            cflags += TEMPLATE_FLAG.format(flag=flag.replace("\\", "\\\\").replace("\"", "\\\""))
    for flag in cdetails.cxxflags:
        if not flag in state.flags_to_remove:
            cxxflags += TEMPLATE_FLAG.format(flag=flag.replace("\\", "\\\\").replace("\"", "\\\""))

    if args.add_built_in_includes:
        cflags += get_built_in_include_flags(cdetails.cc, state, args)
        cxxflags += get_built_in_include_flags(cdetails.cxx, state, args)

    for src in cdetails.src_c:
        if state.is_first:
            state.is_first = False
        else:
            sys.stdout.write(",\n")
        obj = os.path.splitext(src)[0] + ".o"
        sys.stdout.write(TEMPLATE_COMMAND.format(
                compiler=cdetails.cc, directory=cdetails.dir, output=obj,
                source=src, flags=cflags, bin_directory=path))

    for src in cdetails.src_cxx:
        if state.is_first:
            state.is_first = False
        else:
            sys.stdout.write(",\n")
        obj = os.path.splitext(src)[0] + ".o"
        sys.stdout.write(TEMPLATE_COMMAND.format(
                compiler=cdetails.cxx, directory=cdetails.dir, output=obj,
                source=src, flags=cxxflags, bin_directory=path))

def generate_compile_commands(args):
    """
    Generate the compile_commands.json content and write them to stdout

    :param args: command line arguments
    """
    state = State()
    for flag in args.filter_out:
        state.flags_to_remove.add(flag)

    sys.stdout.write("[\n")

    for module in os.scandir(args.path):
        if module.is_dir() and module.name != 'riotbuild':
            generate_module_compile_commands(module.path, state, args)
    sys.stdout.write("\n]\n")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate compile_commands.json for RIOT apps')
    parser.add_argument('path', metavar='PATH', type=str,
            help='Bin path, usually \'<APP>/bin/<BOARD>\'')
    parser.add_argument('--add-built-in-includes', default=False, action='store_const', const=True,
            help='Explicitly add built in include search directories with -I<PATH> options')
    parser.add_argument('--add-libstdcxx-includes', default=False, action='store_const', const=True,
            help='Explicitly add libstdc++ include search directories with -I<PATH> options')
    parser.add_argument('--filter-out', type=str, default=[], action='append',
            help='Drop the given flag, if present (repeatable)')
    parser.add_argument('--clangd', default=False, action='store_const', const=True,
            help='Shorthand for --add-built-in-includes --add-libstdxx-includes ' +
                 '--filter-out=-Wformat-truncation --filter-out=-Wformat-overflow ' +
                 '--filter-out=-mno-thumb-interwork')
    _args = parser.parse_args()
    if _args.clangd:
        _args.add_built_in_includes = True
        _args.add_libstdcxx_includes = True
        _args.filter_out = ['-Wformat-truncation', '-Wformat-overflow', '-mno-thumb-interwork']
    generate_compile_commands(_args)
