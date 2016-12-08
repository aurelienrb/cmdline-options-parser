/* This code is under MIT license.
   See https://opensource.org/licenses/MIT
   
   Code to parse command line options in a C++ program.
   
   Principles:
   - parameter which name start with '-' become optionnal flags
     - supported syntax: "-f=value", "-f value"
     - passing only "-f" is equivalent to passing "-f=true"
     - flags "-h, --help, -?" (help) and "-v, --version" (version) can be automatically handled
   - parameter which name don't start with '-' become "positional args":
     - the user can pass them directly on the command line without specifying a flag name
     - they are mandatory unless a default value is provided
     - they can still be associated with a flag name
     - "help" and "version" are reserved names for automatic processing of help and version messages

   Example:
   
   #include <cstdio>
   #include "cmdline_parser.h"
   
   int main(int argc, char * argv[]) {
       
       auto args = cmdline::parse(argc, argv, {
           { "help", "Simple program to rename a file." },
           { "version", "1.0" },
           { "input", "Input file to rename" }, // "input(s)"
           { { "-o", "--output" }, "Output file name", "output.txt" }, // "${input}.txt"
           { { "--verbose" }, "Print more info about what is being done" }
       });
       
       if (!std::rename(args["input"].c_str(), args["output"].c_str())) {
           std::cerr << "Failed to rename " << args["input"] << std::endl;
           return 1;
       }
       //else if (args.isset("--verbose")) {
       else if (args["--verbose"] == "true") {
           std::cout << "File was renamed to " << args["output"] << std::endl;
       }
       return 0;
   }
*/
#pragma once

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <cassert>
#include <initializer_list>

namespace cmdline {
    struct ProgramOption {
        std::string name;
        std::vector<std::string> flags;
        std::string description;
        std::string defaultValue;

        ProgramOption() {}
        ProgramOption(std::string optName, std::string optDescr, std::string optDefVal = "") : name(optName), description(optDescr), defaultValue(optDefVal) {
            assert(optDescr.back() != '.');
            if (name == "help") {
                assert(optDefVal.empty());
                description = "print this help message";
                defaultValue = optDescr;
                flags.assign({ "-h", "--help" /*, "-?"*/ });
            }
            else if (name == "version") {
                assert(optDefVal.empty());
                description = "print program version";
                defaultValue = optDescr;
                flags.assign({ "-v", "--version" });
            }
            else {
                assert(!description.empty());
            }
        }
        ProgramOption(std::initializer_list<std::string> optFlags, std::string optDescr, std::string optDefVal = "") : description(optDescr), defaultValue(optDefVal) {
            assert(optDescr.back() != '.');
            for (const auto & f : optFlags) {
                if (f.front() == '-') {
                    flags.push_back(f);
                }
                else {
                    assert(name.empty());
                    name = f;
                }
            }
        }
    };

    std::map<std::string, std::string>
    parse(int argc, char *argv[], std::vector<ProgramOption> options);
    
    namespace priv {
        inline std::string extractProgramName(const std::string & argv0) {
            size_t lastSlash = argv0.find_last_of('/');
            if (lastSlash == std::string::npos) {
                lastSlash = 0;
            }
            else {
                lastSlash += 1;
            }
#ifdef _WIN32
            const size_t lastAntiSlash = argv0.find_last_of('\\');
            if (lastAntiSlash + 1 > lastSlash) {
                lastSlash = lastAntiSlash + 1;
            }
#endif
            return argv0.substr(lastSlash);
        }

        inline void displayHelpMessageWindowsStyle(const std::string & argv0, const std::vector<ProgramOption> & options) {
            std::string aboutMsg;
            std::string allFlags;
            std::string allPositionals;
            for (const auto & opt : options) {
                if (opt.name == "help") {
                    aboutMsg = opt.defaultValue;
                }
                else if (opt.name == "version") {
                    // ignore
                }
                else if (opt.name.empty()) {
                    assert(!opt.flags.empty());
                    allFlags += " [" + opt.flags.front() + "]";
                }
                else {
                    allPositionals += " " + opt.name;
                }
            }

            if (!aboutMsg.empty()) {
                std::cout << aboutMsg << ".\n";
            }

            std::cout << "\n";
            std::cout << extractProgramName(argv0) << allFlags << allPositionals << "\n";

            std::cout << "\n";
            for (const auto opt : options) {
                if (!opt.flags.empty()) {
                    std::string allFlags;
                    for (const auto & f : opt.flags) {
                        if (!allFlags.empty()) {
                            allFlags += ", ";
                        }
                        allFlags += f;
                    }
                    std::cout << "  " << allFlags << "\n";
                    std::cout << std::string(8, ' ') << opt.description << "\n";
                }
            }
            std::cout << std::endl;
        }

        inline void displayHelpMessage(const std::string & argv0, const std::vector<ProgramOption> & options) {
            std::string aboutMsg;
            std::string allFlags;
            std::string allPositionals;
            std::string helpAndVersion;
            for (const auto & opt : options) {
                if (opt.name == "help") {
                    aboutMsg = opt.defaultValue;
                    for (auto f : opt.flags) {
                        if (!helpAndVersion.empty()) {
                            helpAndVersion += " | ";
                        }
                        helpAndVersion += f;
                    }
                }
                else if (opt.name == "version") {
                    for (auto f : opt.flags) {
                        if (!helpAndVersion.empty()) {
                            helpAndVersion += " | ";
                        }
                        helpAndVersion += f;
                    }
                }
                else if (!opt.name.empty()) {
                    allPositionals += " " + opt.name;
                }
            }

            const std::string progName = extractProgramName(argv0);
            std::cout << "Usage: " << progName << " [OPTIONS]" << allPositionals << "\n";
            if (!helpAndVersion.empty()) {
                std::cout << "       " << progName << " [" << helpAndVersion << "]\n";
            }
            std::cout << "\n";

            if (!aboutMsg.empty()) {
                std::cout << aboutMsg << ".\n";
                std::cout << "\n";
            }

            std::cout << "Options:\n";
            std::cout << "\n";

            for (const auto opt : options) {
                if (!opt.flags.empty()) {
                    std::string allFlags;
                    for (const auto & f : opt.flags) {
                        if (!allFlags.empty()) {
                            allFlags += ", ";
                        }
                        allFlags += f;
                    }
                    size_t paddingLength = (allFlags.length() < 20) ? (20 - allFlags.length()) : 0;
                    std::cout << "  " << allFlags << std::string(paddingLength, ' ') << opt.description << "\n";
                }
            }
            std::cout << std::endl;
        }

        inline void setValue(std::map<std::string, std::string> & result, const ProgramOption & opt, const std::string & value) {
            assert(result[opt.name].empty());
            result[opt.name] = value;
        }
    }

    inline std::map<std::string, std::string>
    parse(int argc, char *argv[], std::vector<ProgramOption> options) {
        std::map<std::string, std::string> result;
        ProgramOption positionalOption{};
        
        // associate each flag with its full description + fill default values
        std::map<std::string, ProgramOption> allFlags;
        for (const auto opt : options) {
            for (auto name : opt.flags) {
                assert(allFlags.count(name) == 0);
                allFlags[name] = opt;
                result[name] = opt.defaultValue;
            }
            if (!opt.name.empty() && opt.flags.empty() && opt.name != "help" && opt.name != "version") {
                assert(positionalOption.name.empty()); // only 1 positional option
                positionalOption = opt;
            }
        }

        // process the given command line
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg.front() == '-') {
                const auto it = allFlags.find(arg);
                if (it != allFlags.end()) {
                    const auto opt = it->second;
                    // process reserved names
                    if (opt.name == "help") {
                        priv::displayHelpMessage(argv[0], options);
                        std::cout.flush();
                        std::exit(0);
                    }
                    else if (opt.name == "version") {
                        std::cout << opt.defaultValue << std::endl;
                        std::exit(0);
                    }
                    // process named options
                    else if (!opt.name.empty()) {
                        // we expect a value for named options
                        ++i;
                        if (i == argc || argv[i][0] == '-') {
                            std::cerr << "Error: missing value for option '" << arg << "' (" << opt.description << ").\n";
                            std::exit(1);
                        }
                        priv::setValue(result, opt, argv[i]);
                    }
                    // process flags
                    else {
                        priv::setValue(result, opt, "true");
                    }
                }
                else {
                    std::cerr << "Error: unknown option '" << arg << "'" << std::endl;
                    priv::displayHelpMessage(argv[0], options);
                    std::exit(1);
                }
            }
            else if (!positionalOption.name.empty()) {
                priv::setValue(result, positionalOption, arg);
                // for now, we support only 1 positional arg value
                positionalOption = ProgramOption{};
            }
            else {
                std::cerr << "Error: unexpected value '" << arg << "'." << std::endl;
                priv::displayHelpMessage(argv[0], options);
                std::exit(1);
            }
        }

        // checking that positionnal arg is set
        if (!positionalOption.name.empty()) {
            assert(result[positionalOption.name].empty());
            std::cerr << "Error: missing '" << positionalOption.name << "' value (" << positionalOption.description << ").\n";
            priv::displayHelpMessage(argv[0], options);
            std::exit(1);
        }

        return result;
    }
}
