//------------------------------------------------------------------------------
// vfx-rs
//------------------------------------------------------------------------------
#include "cppmm_ast_write_cmake.hpp"

#include "cppmm_ast.hpp"

#include "pystring.h"

#include <fmt/os.h>

#include <fstream>
#include <iostream>
#include <set>

#include "filesystem.hpp"

namespace fs = ghc::filesystem;

#define SPDLOG_ACTIVE_LEVEL TRACE

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#define panic(...)                                                             \
    {                                                                          \
        SPDLOG_CRITICAL(__VA_ARGS__);                                          \
        abort();                                                               \
    }

#define expect(CONDITION, ...)                                                 \
    if (!(CONDITION)) {                                                        \
        SPDLOG_CRITICAL(__VA_ARGS__);                                          \
        abort();                                                               \
    }

namespace cppmm {
namespace write {

//------------------------------------------------------------------------------
static void indent(fmt::ostream& out, const size_t depth) {
    for (size_t i = 0; i != depth; ++i) {
        out.print("    ");
    }
}

const std::string compute_out_include_path(const std::string& filename) {
    return fs::path(filename).parent_path();
}

void write_abigen_cmake(fs::path output_directory,
                        const std::set<std::string>& include_paths,
                        const std::vector<std::string>& target_link_libraries) {
    auto cmakefile_path = output_directory / "CMakeLists.txt";
    auto out = fmt::output_file(cmakefile_path.c_str());

    out.print("file(GLOB_RECURSE ABIGEN_SOURCE *.cpp)\n");
    out.print("add_executable(abigen ${{ABIGEN_SOURCE}})\n");

    for (const auto& i : include_paths) {
        out.print("target_include_directories(abigen PRIVATE {})\n", i);
    }

    out.print("target_include_directories(abigen PRIVATE ${{CMAKE_CURRENT_SOURCE_DIR}}/..)\n");

    for (const auto& l : target_link_libraries) {
        out.print("target_link_libraries(abigen PRIVATE {})\n", l);
    }

    fs::path py_path = output_directory / "insert_abi.py";
    std::ofstream out_py(py_path.c_str());

    out_py << R"(
import os
import sys
import re

in_path = sys.argv[1]
out_path = sys.argv[2]
abi_path = sys.argv[3]

abi = {}
with open(abi_path) as f:
    for l in f.readlines():
        name, size, align = l[:-1].split('|')
        abi[name] = (size, align)

print(abi)

re_size = re.compile('%SIZE(.+)%')
re_align = re.compile('%ALIGN(.+)%')

def size_repl(m):
    s = m.group(1)
    size, _ = abi[s]
    return size

def align_repl(m):
    s = m.group(1)
    _, align = abi[s]
    return align

for root, dirs, files in os.walk(in_path):
    print('root', root)
    print('dirs', dirs)
    for file in files:
        full_path = os.path.join(root, file)
        rel_path = os.path.relpath(full_path, in_path)
        rel_head = os.path.split(rel_path)[0]

        new_head = os.path.join(out_path, rel_head)

        # This doesn't always work, for... reasons, so need to handle the file 
        # exists error
        if not os.path.exists(new_head):
            import errno
            try:
                os.makedirs(new_head)
            except OSError as e:
                if e.errno == errno.EEXIST:
                    pass
                else:
                    raise
        
        new_fn = os.path.join(new_head, file)

        # Now read in the file contents replace all the markers, and write it 
        # out to the actual include directory
        # print('reading %s' % full_path)
        txt_in = ''
        with open(full_path) as f_in:
            txt_in = f_in.read()

            txt_in = re_size.sub(size_repl, txt_in)
            txt_in = re_align.sub(align_repl, txt_in)
            
        with open(new_fn, 'w') as f_out:
            f_out.write(txt_in)
)";
}

//------------------------------------------------------------------------------
void cmake(const char* project_name, const Root& root, size_t starting_point,
           const Libs& libs, const LibDirs& lib_dirs, int version_major,
           int version_minor, int version_patch, const char* base_project_name,
           const char* output_directory, const std::string& api_prefix) {
    expect(starting_point < root.tus.size(),
           "starting point ({}) is out of range ({})", starting_point,
           root.tus.size());

    auto cmakefile_path = fs::path(output_directory) / "CMakeLists.txt";

    auto out = fmt::output_file(cmakefile_path.c_str());

    // Minimum version
    out.print("cmake_minimum_required(VERSION 3.5)\n");
    out.print("project({} VERSION {}.{}.{})\n", project_name, version_major,
              version_minor, version_patch);
    out.print("set(CMAKE_CXX_STANDARD 14 CACHE STRING \"\")\n");

    // Library
    std::set<std::string> include_paths;

    const auto size = root.tus.size();
    out.print("set(SOURCES\n");
    std::string first_header = "";
    for (size_t i = starting_point; i < size; ++i) {
        const auto& tu = root.tus[i];

        // Source files are in a 'src' subdirectory
        indent(out, 1);
        std::string filename = pystring::os::path::join("src", tu->filename);
        out.print("{}\n", filename);

        if (i == starting_point) {
            std::string ext;
            pystring::os::path::splitext(first_header, ext, tu->filename);
            first_header += ".h";
            indent(out, 1);
            out.print("{}\n",
                      pystring::os::path::join("include", first_header));
        }

        // Add all the include paths
        for (auto& i : tu->include_paths) {
            include_paths.insert(i);
        }
    }

    indent(out, 1);
    out.print("{}\n", fmt::format("src/{}-errors.cpp", base_project_name));
    out.print(")\n");

    auto abigen_include_paths = include_paths;
    write_abigen_cmake(fs::path(output_directory) / "abigen",
                       abigen_include_paths, {});

    out.print(R"(
add_subdirectory(abigen)
add_custom_command(OUTPUT ${{CMAKE_CURRENT_SOURCE_DIR}}/include/{0}
    DEPENDS ${{CMAKE_CURRENT_SOURCE_DIR}}/include.in/{0} 
    COMMAND abigen
    COMMAND python ARGS 
        ${{CMAKE_CURRENT_SOURCE_DIR}}/abigen/insert_abi.py 
        ${{CMAKE_CURRENT_SOURCE_DIR}}/include.in 
        ${{CMAKE_CURRENT_BINARY_DIR}}/include 
        ${{CMAKE_CURRENT_BINARY_DIR}}/abigen.txt
)
)",
              first_header);

    out.print("set(LIBNAME {}-{}_{})\n", project_name, version_major,
              version_minor);
    out.print("add_library(${{LIBNAME}} STATIC ${{SOURCES}})\n");
    out.print("add_library(${{LIBNAME}}-shared SHARED ${{SOURCES}})\n");

    // hide symbols by default
    out.print("\nset_target_properties(${{LIBNAME}} PROPERTIES "
              "CXX_VISIBILITY_PRESET hidden)\n");
    out.print("set_target_properties(${{LIBNAME}}-shared PROPERTIES "
              "CXX_VISIBILITY_PRESET hidden)\n\n");

    // tell it we're building hte library not linking against it
    out.print("target_compile_definitions(${{LIBNAME}} PRIVATE "
              "{}_CPPMM_BUILD_EXPORT)\n",
              api_prefix);
    out.print("target_compile_definitions(${{LIBNAME}}-shared PRIVATE "
              "{}_CPPMM_BUILD_EXPORT)\n",
              api_prefix);

    // Windows...
    out.print("if (WIN32)\n");
    out.print("target_compile_definitions(${{LIBNAME}} PRIVATE _Bool=bool)\n");
    out.print(
        "target_compile_definitions(${{LIBNAME}}-shared PRIVATE _Bool=bool)\n");
    out.print("endif()\n");

    // Add the include path of the output headers
    // include_paths.insert(compute_out_include_path("./"));
    include_paths.insert("include");
    include_paths.insert("${CMAKE_CURRENT_BINARY_DIR}/include");
    include_paths.insert("src");
    include_paths.insert("private");

    // Include directories
    for (auto& include_path : include_paths) {
        out.print("target_include_directories(${{LIBNAME}} PRIVATE {})\n",
                  include_path);
        out.print(
            "target_include_directories(${{LIBNAME}}-shared PRIVATE {})\n",
            include_path);
    }

    // Add the libraries
    for (auto& lib : libs) {
        auto lib_var = std::string("LIB_") + pystring::upper(lib);

        out.print("find_library ( {} NAMES {} PATHS", lib_var, lib);
        for (auto& lib_dir : lib_dirs) {
            out.print(" {}", lib_dir);
        }
        out.print(")\n");
        out.print("target_link_libraries (${{LIBNAME}} ${{{}}})\n", lib_var);
        out.print("target_link_libraries (${{LIBNAME}}-shared ${{{}}})\n",
                  lib_var);
    }

    // add install command for rust cmake
    out.print("install(TARGETS ${{LIBNAME}} DESTINATION "
              "${{CMAKE_INSTALL_PREFIX}})\n");
    out.print("install(TARGETS ${{LIBNAME}}-shared DESTINATION "
              "${{CMAKE_INSTALL_PREFIX}})\n");
}

//------------------------------------------------------------------------------
void cmake_modern(const char* project_name, const Root& root,
                  size_t starting_point,
                  const std::vector<std::string>& find_packages,
                  const std::vector<std::string>& target_link_libraries,
                  int version_major, int version_minor, int version_patch,
                  const char* base_project_name, const char* output_directory,
                  const std::string& api_prefix) {
    expect(starting_point < root.tus.size(),
           "starting point ({}) is out of range ({})", starting_point,
           root.tus.size());

    auto cmakefile_path = fs::path(output_directory) / "CMakeLists.txt";

    auto out = fmt::output_file(cmakefile_path.c_str());

    // Minimum version
    out.print("cmake_minimum_required(VERSION 3.5)\n");
    out.print("project({} VERSION {}.{}.{})\n", project_name, version_major,
              version_minor, version_patch);
    out.print("set(CMAKE_CXX_STANDARD 14 CACHE STRING \"\")\n");

    // Library
    std::set<std::string> include_paths;

    const auto size = root.tus.size();
    out.print("set(SOURCES\n");
    std::string first_header = "";
    for (size_t i = starting_point; i < size; ++i) {
        const auto& tu = root.tus[i];

        // Source files are in a 'src' subdirectory
        indent(out, 1);
        std::string filename = pystring::os::path::join("src", tu->filename);
        out.print("{}\n", filename);

        if (i == starting_point) {
            std::string ext;
            pystring::os::path::splitext(first_header, ext, tu->filename);
            first_header += ".h";
            indent(out, 1);
            out.print("{}\n",
                      pystring::os::path::join("include", first_header));
        }

        // Add all the include paths
        for (auto& i : tu->include_paths) {
            include_paths.insert(i);
        }
    }

    indent(out, 1);
    out.print("src/{}\n", fmt::format("{}-errors.cpp", base_project_name));
    out.print(")\n");

    // Add the libraries
    for (const auto& pkg : find_packages) {
        out.print("find_package({} REQUIRED)\n", pkg);
    }

    auto abigen_include_paths = include_paths;
    write_abigen_cmake(fs::path(output_directory) / "abigen",
                       abigen_include_paths, target_link_libraries);

    out.print(R"(
add_subdirectory(abigen)
add_custom_command(OUTPUT ${{CMAKE_CURRENT_SOURCE_DIR}}/include/{0}
    DEPENDS ${{CMAKE_CURRENT_SOURCE_DIR}}/include.in/{0} 
    COMMAND abigen
    COMMAND python ARGS 
        ${{CMAKE_CURRENT_SOURCE_DIR}}/abigen/insert_abi.py 
        ${{CMAKE_CURRENT_SOURCE_DIR}}/include.in 
        ${{CMAKE_CURRENT_BINARY_DIR}}/include 
        ${{CMAKE_CURRENT_BINARY_DIR}}/abigen.txt
)
)",
              first_header);

    out.print("set(LIBNAME {}-{}_{})\n", project_name, version_major,
              version_minor);
    out.print("add_library(${{LIBNAME}} STATIC ${{SOURCES}})\n");
    out.print("add_library(${{LIBNAME}}-shared SHARED ${{SOURCES}})\n");

    // hide symbols by default
    out.print("\nset_target_properties(${{LIBNAME}} PROPERTIES "
              "CXX_VISIBILITY_PRESET hidden)\n");
    out.print("set_target_properties(${{LIBNAME}}-shared PROPERTIES "
              "CXX_VISIBILITY_PRESET hidden)\n\n");

    // tell it we're building hte library not linking against it
    out.print("\ntarget_compile_definitions(${{LIBNAME}} PRIVATE "
              "{}_CPPMM_BUILD_EXPORT)\n",
              api_prefix);
    out.print("\ntarget_compile_definitions(${{LIBNAME}}-shared PRIVATE "
              "{}_CPPMM_BUILD_EXPORT)\n",
              api_prefix);

    // Windows...
    out.print("if (WIN32)\n");
    out.print("target_compile_definitions(${{LIBNAME}} PRIVATE _Bool=bool)\n");
    out.print(
        "target_compile_definitions(${{LIBNAME}}-shared PRIVATE _Bool=bool)\n");
    out.print("endif()\n");

    // Add the include path of the output headers
    // include_paths.insert(compute_out_include_path("./src"));
    include_paths.insert("include");
    include_paths.insert("${CMAKE_CURRENT_BINARY_DIR}/include");
    include_paths.insert("src");
    include_paths.insert("private");

    // Include directories
    for (auto& include_path : include_paths) {
        out.print("target_include_directories(${{LIBNAME}} PRIVATE {})\n",
                  include_path);
        out.print(
            "target_include_directories(${{LIBNAME}}-shared PRIVATE {})\n",
            include_path);
    }

    for (auto& lib : target_link_libraries) {
        out.print("target_link_libraries(${{LIBNAME}} {})\n", lib);
        out.print("target_link_libraries(${{LIBNAME}}-shared {})\n", lib);
    }

    // add install command for rust cmake
    out.print("install(TARGETS ${{LIBNAME}} DESTINATION "
              "${{CMAKE_INSTALL_PREFIX}})\n");
    out.print("install(TARGETS ${{LIBNAME}}-shared DESTINATION "
              "${{CMAKE_INSTALL_PREFIX}})\n");
}

} // namespace write
} // namespace cppmm
