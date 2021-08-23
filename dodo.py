#!/usr/bin/env -S doit -f

from pathlib import Path

"""
Project directory structure :

Each folder in src/ represent "options" to pick from
with the caveat that picking an option is mandatory
You can think of them as alternate implementations of the same thing, or
as optional features.

The headers folder follow the same structure as the source folder.
Source files MAY include any header, but SHOULD NOT include headers of a sibling directory.

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

Source files in altvm_c may include headers from headers/, headers/compiler/,
headers/compiler/altvm_c, but should not include headers from headers/interpreter or
headers/compiler/compiler.


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

def generic_build(srcdir, objdir, compileopt, linkopt, mangler="", linkwith=None) :

    dot_mangler = mangler and ('.' + mangler)
    mangler_dash = mangler and (mangler + '-')
    hidden_mangler = '_' + mangler

    subfiles = list()
    flavors = list()
    if linkwith is None : linkwith = []
    for path in srcdir.iterdir() :
        if path.is_dir() :
            flavors.append(path)
        elif path.is_file() :
            dst = objdir / f"{srcdir.name}__{path.name}{dot_mangler}.o"
            subfiles.append(dst)
            yield {
                "basename" : hidden_mangler, # internal task, starts with '_'
                "name" : dst.name,
                "targets" : [dst],
                "file_dep" : [path],
                "actions" : [COMPILER + compileopt + ["-o", dst] + [path]],
                "clean" : True,
            }

    if len(subfiles) :
        target = objdir / f"{srcdir.name}{dot_mangler}.o"
        linkwith.append(target)
        yield {
            "basename" : hidden_mangler, # internal task, starts with '_'
            "name" : target.name,
            "targets" : [target],
            "file_dep" : subfiles,
            "actions" : [COMPILER + linkopt + ["-r"] + ["-o", target] + subfiles],
            "clean" : True,
        }

    if len(flavors) :
        for sub in flavors :
            yield from generic_build(sub, objdir, compileopt, linkopt,  mangler, linkwith.copy())
    else :
        target = Path(f"{mangler_dash}{srcdir.stem}")
        yield {
            "name" : target,
            "targets" : [target],
            "file_dep" : linkwith,
            "actions" : [COMPILER + linkopt + ["-o", target] + linkwith],
            "clean" : True,
        }


src = Path("src/")
headers = Path("headers/")
debug = Path("headers/debug.h")
buildpath = Path("build/")

COMPILER = ["gcc"]

# general options for GCC
GCC_OPT = [
"-Wall",
"-flto",
"-fcf-protection=full",
]

GCC_LINKOPT = GCC_OPT + [
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
"-fstack-clash-protection",
"-D_FORTIFY_SOURCE=2",
]
GCC_DEBUG = _GCC_COMPILEOPT + ["-D", "DEBUG", "-Og", "-Wall"]
GCC_MAIN = _GCC_COMPILEOPT + ["-O3"]


def task_debug() :
    yield from generic_build(src, buildpath, GCC_DEBUG, GCC_LINKOPT, "dbg")

def task_release() :
    yield from generic_build(src, buildpath, GCC_MAIN, GCC_LINKOPT)

DOIT_CONFIG = {'default_tasks': ['debug']}
