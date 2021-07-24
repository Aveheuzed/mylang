#!/usr/bin/env -S doit -f

from pathlib import Path

DOIT_CONFIG = {'default_tasks': ['debug']}

def generic_build(target, srcdir, objdir, compileopt, linkopt, objsuffix=".o"):
    subfiles = list()
    for subtask in recursive_build(srcdir, objdir, compileopt, linkopt, objsuffix) :
        if not subtask["is_agglomerated"] :
            subfiles.extend(subtask["targets"])
        del subtask["is_agglomerated"]
        yield subtask

    yield {
        "name" : target.name,
        "targets" : [target],
        "file_dep" : subfiles,
        "actions" : [COMPILER + linkopt + ["-o", target] + subfiles],
        "clean" : True,
    }

def recursive_build(srcdir, objdir, compileopt, linkopt, objsuffix=".o") :
    for path in srcdir.iterdir() :
        if path.is_dir() :
            subfiles = list()
            for subtask in recursive_build(path, objdir, compileopt, linkopt, objsuffix) :
                if not subtask["is_agglomerated"] :
                    subfiles.extend(subtask["targets"])
                    subtask["is_agglomerated"] = True

                yield subtask

            dst = (objdir/path.stem).with_suffix(objsuffix)
            yield {
                "is_agglomerated" : False, # flag for personal use
                "name" : dst.name,
                "targets" : [dst],
                "file_dep" : subfiles,
                "actions" : [COMPILER + linkopt + ["-r"] + ["-o", dst] + subfiles],
                "clean" : True,
            }

        elif path.is_file() :
            dst = (objdir/path.stem).with_suffix(objsuffix)
            yield {
                "is_agglomerated" : False, # flag for personal use

                "name" : dst.name,
                "targets" : [dst],
                "file_dep" : [path],
                "actions" : [COMPILER + compileopt + ["-o", dst] + [path]],
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
    yield from generic_build(Path("./debug"), src, buildpath, GCC_DEBUG, GCC_OPT, ".dbg.o")

def task_release() :
    yield from generic_build(Path("./release"), src, buildpath, GCC_MAIN, GCC_OPT)
