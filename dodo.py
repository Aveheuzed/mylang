#!/usr/bin/env -S doit -f

from pathlib import Path

"""
Project directory structure :

Each folder in src/ represent "options" to pick from
with the caveat that picking an option is mandatory
You can think of them as alternate implementations of the same thing, or
as optional features.

All headers are located at the same place, in the headers/ folder.
There is no build-related meaning to subdirectories of headers/.

Example :
src
├── compiler
│   ├── altvm_c
│   ├── altvm_s
│   └── compiler
└── interpreter
You can build an interpreter, or a compiler.
The compiler comes in 3 flavors :
  regular (with the regular VM),
  with an alterative VM implemented in C,
  with an alterative VM implemented in assembly.

Files in src/ are needed to build anything, whereas files in src/compiler/
are only be needed by compiler builds.


Build process :
* Compile each file in the current folder
* Link them all (partial linking)
* If there are subfolders :
(meaning there are still flavors to choose)
*     Recurse in the folders, giving them our objfile to link with
* Else :
(meaning we're the most refined flavor)
*     Link everything together in the cwd, producing the final executable
"""

def generic_build(build_name, srcdir, objdir, compileopt, linkopt, objsuffix=".o", linkwith=[]) :
    subfiles = list()
    flavors = list()
    for path in srcdir.iterdir() :
        if path.is_dir() :
            flavors.append(path)
        elif path.is_file() :
            dst = objdir / f"{srcdir.name}__{path.name}{objsuffix}"
            subfiles.append(dst)
            yield {
                "basename" : f"_{srcdir.stem}@{build_name}", # internal task, starts with '_'
                "name" : dst.name,
                "targets" : [dst],
                "file_dep" : [path],
                "actions" : [COMPILER + compileopt + ["-o", dst] + [path]],
                "clean" : True,
            }

    if len(subfiles) :
        target = objdir / f"{srcdir.name}{objsuffix}"
        linkwith.append(target)
        yield {
            "basename" : f"_{srcdir.stem}@{build_name}", # internal task, starts with '_'
            "name" : target.name,
            "targets" : [target],
            "file_dep" : subfiles,
            "actions" : [COMPILER + linkopt + ["-r"] + ["-o", target] + subfiles],
            "clean" : True,
        }

    if len(flavors) :
        for sub in flavors :
            yield from generic_build(build_name, sub, objdir, compileopt, linkopt, objsuffix, linkwith.copy())
    else :
        target = Path(f"./{build_name}_{srcdir.stem}")
        yield {
            "basename" : f"{srcdir.stem}@{build_name}",
            "name" : target,
            "targets" : [target],
            "file_dep" : linkwith,
            "actions" : [COMPILER + linkopt + ["-o", target] + linkwith],
            "clean" : True,
        }


src = Path("src/")
headers = Path("headers/")
debug = Path("headers/utils/debug.h")
buildpath = Path("build/")

COMPILER = ["gcc"]

# general options for GCC
GCC_OPT = [
"-flto",
"-Wl,-z,now",
"-Wl,-z,relro",
"-Wl,-z,notext",
]

# compilation comes in two flavor: debug build or main build
_GCC_COMPILEOPT = GCC_OPT + [
"-c",
"-include", debug,
"-iquote", headers,
"-fshort-enums",
"-fstack-protector-strong",
"-D_FORTIFY_SOURCE=2",
"-fcf-protection",
]
GCC_DEBUG = _GCC_COMPILEOPT + ["-D", "DEBUG", "-Og", "-Wall"]
GCC_MAIN = _GCC_COMPILEOPT + ["-O3"]


def task_debug() :
    yield from generic_build("debug", src, buildpath, GCC_DEBUG, GCC_OPT, ".dbg.o")

def task_release() :
    yield from generic_build("release", src, buildpath, GCC_MAIN, GCC_OPT)

DOIT_CONFIG = {'default_tasks': ['compile@debug']}
