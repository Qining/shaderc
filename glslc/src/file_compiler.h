// Copyright 2015 The Shaderc Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GLSLC_FILE_COMPILER_H
#define GLSLC_FILE_COMPILER_H

#include <string>

#include "libshaderc_util/file_finder.h"
#include "libshaderc_util/string_piece.h"
#include "shaderc/shaderc.hpp"

#include "dependency_info.h"

namespace glslc {

// Context for managing compilation of source GLSL files into destination
// SPIR-V files.
class FileCompiler {
 public:
  FileCompiler() : needs_linking_(true), total_warnings_(0), total_errors_(0) {}

  // Compiles a shader received in input_file, returning true on success and
  // false otherwise. If force_shader_stage is not shaderc_glsl_infer_source or
  // any default shader stage then the given shader_stage will be used, otherwise
  // it will be determined from the source or the file type.
  //
  // Places the compilation output into a new file whose name is derived from
  // input_file according to the rules from glslc/README.asciidoc.
  //
  // If version/profile has been forced, the shader's version/profile is set to
  // that value regardless of the #version directive in the source code.
  //
  // Any errors/warnings found in the shader source will be output to std::cerr
  // and increment the counts reported by OutputMessages().
  bool CompileShaderFile(const std::string& input_file,
                         shaderc_shader_kind shader_stage);

  // Adds a directory to be searched when processing #include directives.
  //
  // Best practice: if you add an empty string before any other path, that will
  // correctly resolve both absolute paths and paths relative to the current
  // working directory.
  void AddIncludeDirectory(const std::string& path);

  // Sets the output filename. A name of "-" indicates standard output.
  void SetOutputFileName(const shaderc_util::string_piece& file) {
    output_file_name_ = file;
  }

  // Returns false if any options are incompatible. The num_files parameter
  // represents the number of files that will be compiled.
  bool ValidateOptions(size_t num_files);

  // Outputs to std::cerr the number of warnings and errors if there are any.
  void OutputMessages();

  // Sets the flag to indicate individual compilation mode. In this mode, all
  // files are compiled individually and written to separate output files
  // instead of linked together. This method also disables linking and sets the
  // output file extension to ".spv". Disassembly mode and preprocessing only
  // mode override this mode and flags.
  void SetIndividualCompilationFlag();

  // Sets the flag to indicate disassembly mode. In this mode, the compiler
  // emits disassembled textual output, instead of outputting object files.
  // This method also sets the output file extension to ".spvasm" and disables
  // linking. This mode overrides individual compilation mode, and preprocessing
  // only mode overrides this mode.
  void SetDisassemblyFlag();

  // Sets the flag to indicate preprocessing only mode. In this mode, instead of
  // outputting object files, the compiler emits the preprocessed source files.
  // This method disables linking and sets the output file to stdout. This mode
  // overrides disassembly mode and individual compilation mode.
  void SetPreprocessingOnlyFlag();

  // Gets the reference of the compiler options which reflects the command-line
  // arguments.
  shaderc::CompileOptions& options() { return options_; };

  // Gets a pointer which points to the dependency info dumping hander. Creates
  // such a handler if such one does not exist.
  DependencyInfoDumpingHandler* GetDependencyDumpingHandler() {
    if (!dependency_info_dumping_handler_) {
      dependency_info_dumping_handler_.reset(
          new DependencyInfoDumpingHandler());
    }
    return dependency_info_dumping_handler_.get();
  };

 private:
  // Returns the final file name to be used for the output file.
  //
  // If an output file name is specified by the SetOutputFileName(), use that
  // argument as the final output file name.
  //
  // If the user did not specify an output filename:
  //  If linking is not required, and the input filename has a
  //  standard stage extension (e.g. .vert) then returns the input filename
  //  without directory names but with the result extenstion (e.g. .spv or
  //  .spvasm) appended.
  //
  //  If linking is not required, and the input file name does not have a
  //  standard stage extension, then also returns the directory-stripped input
  //  filename, but replaces its extension with the result extension. (If the
  //  resolved input filename does not have an extension, then appends the result
  //  extension.)
  //
  //  If linking is required and output filename is not specified, returns
  //  "a.spv".
  std::string GetOutputFileName(std::string input_filename);

  // Returns the candidate output file name deduced from input file name and
  // user specified output file name. It is computed as follows:
  //
  // If the user did specify an output filename and the compiler is not in
  // preprocessing-only mode, then returns that file name.
  //
  // If the user did not specify an output filename:
  //  If the input filename has a standard stage extension (e.g. .vert) then
  //  returns the input filename without directory names but with the result
  //  extenstion (e.g. .spv or .spvasm) appended.
  //
  //  If the input file name does not have a standard stage extension, then also
  //  returns the directory-stripped input filename, but replaces its extension
  //  with the result extension. (If the resolved input filename does not have
  //  an extension, then appends the result extension.)
  //
  //  When a resolved extension is not available because the compiler is in
  //  preprocessing-only mode or the compilation requires linking, use .spv as
  //  the extension.
  std::string GetCandidateOutputFileName(std::string input_filename);

  // Performs actual SPIR-V compilation on the contents of input files.
  shaderc::Compiler compiler_;

  // Reflects the command-line arguments and goes into
  // compiler_.CompileGlslToSpv().
  shaderc::CompileOptions options_;

  // A FileFinder used to substitute #include directives in the source code.
  shaderc_util::FileFinder include_file_finder_;

  // Indicates whether linking is needed to generate the final output.
  bool needs_linking_;

  // Flag for disassembly mode.
  bool disassemble_ = false;

  // Flag for preprocessing only mode.
  bool preprocess_only_ = false;

  // The ownership of dependency dumping handler.
  std::unique_ptr<DependencyInfoDumpingHandler>
      dependency_info_dumping_handler_ = nullptr;

  // Reflects the type of file being generated.
  std::string file_extension_;
  // Name of the file where the compilation output will go.
  shaderc_util::string_piece output_file_name_;

  // Counts warnings encountered in compilation.
  size_t total_warnings_;
  // Counts errors encountered in compilation.
  size_t total_errors_;
};
}
#endif  // GLSLC_FILE_COMPILER_H
