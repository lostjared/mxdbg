#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["pcons>=0.24"]
# ///
"""Pcons build for mxdbg.

mxdbg is an x86-64 Linux debugger built on ptrace. It can consume an installed
ollama_gen package, or build a source checkout in the same graph::

    uvx pcons -B build/pcons OLLAMA_GEN_SOURCE_DIR=..
    uvx pcons -B build/pcons VARIANT=debug OLLAMA_GEN_SOURCE_DIR=..
    uvx pcons -B build/pcons TESTS=0 OLLAMA_GEN_SOURCE_DIR=..
    uvx pcons -B build/pcons PCONS_INSTALL_PREFIX=/usr/local all install

Use ``PREFIX=/path/to/prefix`` when readline, libcurl, or an installed
ollama_gen package lives outside pkg-config's normal search path.
"""

import os
import platform as host_platform
from pathlib import Path

from pcons import Project, find_c_toolchain, get_platform, get_var
from pcons.core.subst import SourcePath, TargetPath

VERSION = "1.2.0"

project_dir = Path(__file__).parent.resolve()
platform = get_platform()


def option(name: str, default: bool = False) -> bool:
    """Read an ON/OFF build option."""
    return get_var(name, "1" if default else "0").lower() in (
        "1",
        "on",
        "true",
        "yes",
    )


if not platform.is_linux or host_platform.machine().lower() not in ("x86_64", "amd64"):
    raise SystemExit("mxdbg requires x86-64 Linux (ptrace and Linux register APIs).")

extra_prefixes = [
    Path(prefix)
    for prefix in (get_var("PREFIX") or "").split(os.pathsep)
    if prefix
]
if extra_prefixes:
    os.environ["PKG_CONFIG_PATH"] = os.pathsep.join(
        [str(prefix / "lib" / "pkgconfig") for prefix in extra_prefixes]
        + [os.environ.get("PKG_CONFIG_PATH", "")]
    )

project = Project("mxdbg", root_dir=project_dir)
env = project.Environment(toolchain=find_c_toolchain())
env.cxx.set_standard(20)
env.set_variant(get_var("VARIANT", "release"))
env.cxx.flags.extend(["-Wall", "-Wextra", "-Wpedantic", "-fPIC"])
# Find libmxdbg beside build-tree executables and in ../lib after installation.
env.link.flags.append("-Wl,-rpath,$$ORIGIN:$$ORIGIN/../lib")
# Pcons' default copy2-based install command preserves source mtimes. DrvFS
# rounds explicitly assigned mtimes down to whole seconds, which leaves each
# installed output older than its input and prevents Ninja from converging.
# GNU install creates parent directories and gives the output a fresh mtime.
env.install.copycmd = ["install", "-D", SourcePath(), TargetPath()]

ollama_source = get_var("OLLAMA_GEN_SOURCE_DIR", "")
if ollama_source:
    source_dir = Path(ollama_source).expanduser().resolve()
    if not (source_dir / "mx2-ollama.cpp").is_file() or not (
        source_dir / "mx2-ollama.hpp"
    ).is_file():
        raise SystemExit(
            "OLLAMA_GEN_SOURCE_DIR does not contain mx2-ollama.cpp and "
            "mx2-ollama.hpp: "
            f"{source_dir}"
        )
    # Pcons intentionally restricts add_subdirectory() to descendants of the
    # top-level project. CMake's OLLAMA_GEN_SOURCE_DIR commonly points to a
    # sibling or parent checkout, so describe that small library locally.
    ollama_curl = project.find_package("libcurl")
    jsoncpp = project.find_package("jsoncpp")
    assert ollama_curl is not None and jsoncpp is not None
    ollama_gen = project.StaticLibrary(
        "ollama_gen",
        env,
        sources=[source_dir / "mx2-ollama.cpp"],
    )
    ollama_gen.public.include_dirs.append(source_dir)
    ollama_gen.link(ollama_curl, jsoncpp)
else:
    ollama_gen = project.find_package("ollama_gen")
    if ollama_gen is None:
        raise SystemExit(
            "ollama_gen was not found. Install its Pcons package and set PREFIX, "
            "or pass OLLAMA_GEN_SOURCE_DIR=/path/to/ollama_gen."
        )

readline = project.find_package("readline")
curl = project.find_package("libcurl")
assert readline is not None and curl is not None

lib_sources = [
    "ai_config.cpp",
    "process.cpp",
    "pipe.cpp",
    "debugger.cpp",
    "disassembly.cpp",
    "debug_context.cpp",
    "static_analysis.cpp",
    "scanner.cpp",
    "string_buffer.cpp",
    "types.cpp",
    "exception.cpp",
]
libmxdbg = project.SharedLibrary(
    "libmxdbg",
    env,
    sources=[project_dir / "libmxdbg" / source for source in lib_sources],
)
libmxdbg.output_name = "mxdbg"
libmxdbg.public.include_dirs.append(project_dir / "libmxdbg" / "include")
libmxdbg.link(ollama_gen)

mxdbg = project.Program(
    "mxdbg",
    env,
    sources=[project_dir / "mxdbg" / "main.cpp"],
)
mxdbg.private.include_dirs.append(project_dir / "mxdbg" / "include")
mxdbg.link(libmxdbg, ollama_gen, readline, curl)

# Assertions are the test suite's checks, so keep them enabled even when the
# library and executable use the release variant.
test_env = env.clone()
for compiler in (test_env.cc, test_env.cxx):
    if "NDEBUG" in compiler.defines:
        compiler.defines.remove("NDEBUG")


def test_program(name: str, *sources: str, link_library: bool = True):
    """Create one of the CMake test executables."""
    program = project.Program(
        name,
        test_env,
        sources=[project_dir / "tests" / source for source in sources],
    )
    program.private.include_dirs.append(project_dir / "libmxdbg" / "include")
    if link_library:
        program.link(libmxdbg, ollama_gen)
    return program


if option("TESTS", default=True):
    ai_config_test = test_program(
        "ai_config_test", "ai_config_test.cpp", "../libmxdbg/ai_config.cpp"
    )
    debug_context_test = test_program(
        "debug_context_test",
        "debug_context_test.cpp",
        "../libmxdbg/debug_context.cpp",
        link_library=False,
    )
    static_analysis_test = test_program(
        "static_analysis_test",
        "static_analysis_test.cpp",
        "../libmxdbg/static_analysis.cpp",
        link_library=False,
    )
    disassembly_test = test_program(
        "disassembly_test",
        "disassembly_test.cpp",
        "../libmxdbg/disassembly.cpp",
        link_library=False,
    )
    step_out_fixture = test_program(
        "step_out_fixture", "step_out_fixture.c", link_library=False
    )
    step_out_fixture.private.link_flags.append("-no-pie")

    registered_tests = {
        "process_continue": test_program("process_continue_test", "process_continue.cpp"),
        "process_attach": test_program("process_attach_test", "process_pid.cpp"),
        "pipe_test": test_program("pipe_test", "pipe_test.cpp"),
        "pipe_write": test_program("pipe_write_test", "pipe_write.cpp"),
        "exception_test": test_program("exception_test", "exception_test.cpp"),
        "register_test": test_program("register_test", "register_test.cpp"),
        "debug_context": debug_context_test,
        "static_analysis": static_analysis_test,
        "disassembly": disassembly_test,
        "ai_config": ai_config_test,
    }
    for test_name, program in registered_tests.items():
        project.Test(
            test_name,
            program,
            timeout=5.0 if test_name == "process_attach" else None,
            labels=["unit"],
        )

    project.Test("cli_help", mxdbg, args=["--help"], labels=["cli"])
    project.Test("cli_version", mxdbg, args=["--version"], labels=["cli"])

public_headers = sorted((project_dir / "libmxdbg" / "include" / "mxdbg").glob("*.hpp"))
project.Alias(
    "install",
    project.Install("lib", [libmxdbg], mode=0o755),
    # InstallAs avoids ambiguity with the source directory also named mxdbg.
    project.InstallAs("bin/mxdbg", mxdbg, name="install_mxdbg", mode=0o755),
    project.Install("include/mxdbg", public_headers, mode=0o644),
)
