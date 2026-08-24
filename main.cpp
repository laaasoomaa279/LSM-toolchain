#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

#include "src/frontend/lexer/Lexer.hpp"
#include "src/frontend/parser/Parser.hpp"
#include "src/middleend/types/TypeChecker.hpp"
#include "src/middleend/types/Monomorphizer.hpp"
#include "src/middleend/lowering/RegionLowering.hpp"
#include "src/middleend/ssa/SSABuilder.hpp"

#include "src/middleend/opt/OptimizationPassManager.hpp"

#include "src/backend/regalloc/RegisterAllocator.hpp"
#include "src/backend/regalloc/PhysicalRegister.hpp"

#include "src/backend/x86_64/X86_64Encoder.hpp"
#include "src/backend/x86_64/PeepholeX86.hpp"
#include "src/backend/arm64/ARM64Encoder.hpp"
#include "src/backend/baremetal/BareMetalX86Encoder.hpp"
#include "src/backend/baremetal/BareMetalX86_32Encoder.hpp"
#include "src/backend/baremetal/BareMetalX86ModernEncoder.hpp" 

#include "src/backend/jit/JITEngine.hpp"
#include "src/backend/linker/ELFBuilder.hpp"
#include "src/backend/raw/RawBinaryBuilder.hpp"
#include "src/loader/ModuleLoader.hpp"

#include "src/runtime/sched/Scheduler.hpp"
#include "tools/lsp/LanguageServer.hpp"

void printUsage() {
    std::cout << "\033[1;36m========================================================\033[0m\n";
    std::cout << "\033[1;32m      LSM (Language for Systems & Machines) Toolchain v1.0.0\033[0m\n";
    std::cout << "\033[1;36m========================================================\033[0m\n";
    std::cout << "Usage:\n";
    std::cout << "  lsm run <file.lsm> [-O3]                         : Compile and execute via Multi-threaded JIT\n";
    std::cout << "  lsm build <file.lsm> [-O3] [-o out] [--m32]      : Build standalone executable\n";
    std::cout << "  lsm build --baremetal <file.lsm> [--bootable]    : Export kernel binary (Standard Engine)\n";
    std::cout << "  lsm build --baremetal --modern <file.lsm>       : Export kernel binary (Modern Engine)\n";
    std::cout << "  lsm --lsp                                        : Run Language Server Protocol for IDEs\n";
    std::cout << "  lsm --version                                    : Display version info\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "--version" || command == "-v") {
        std::cout << "LSM Toolchain v1.0.0 (High-Performance Engine + Dual BareMetal Encoders)\n";
        return 0;
    }

    if (command == "--lsp") {
        LanguageServer lsp;
        lsp.run();
        return 0;
    }

    if (command != "run" && command != "build") {
        std::cerr << "\033[1;31m[Error] Unknown command: " << command << "\033[0m\n";
        printUsage();
        return 1;
    }

    bool isBareMetal = false;
    bool isBootable = false;
    bool isModernEngine = false; 
    bool optimizeO3 = false;
    ArchType selectedArch = ArchType::X86_64;
    std::string sourceFile = "";
    std::string outputFile = "";
    std::string entryPoint = "_start";

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--baremetal") {
            isBareMetal = true;
        } else if (arg == "--bootable") {
            isBootable = true;
            isBareMetal = true;
        } else if (arg == "--modern") {
            isModernEngine = true; 
        } else if (arg == "--m32" || arg == "-m32") {
            selectedArch = ArchType::X86_32;
        } else if (arg == "-O3") {
            optimizeO3 = true;
        } else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg == "--entry" && i + 1 < argc) {
            entryPoint = argv[++i];
        } else if (arg.rfind("--entry=", 0) == 0) {
            entryPoint = arg.substr(8);
        } else if (sourceFile.empty()) {
            sourceFile = arg;
        }
    }

    if (sourceFile.empty()) {
        std::cerr << "\033[1;31m[Error] No source file specified.\033[0m\n";
        return 1;
    }

    if (outputFile.empty()) {
        size_t lastDot = sourceFile.find_last_of('.');
        outputFile = (lastDot == std::string::npos) ? sourceFile : sourceFile.substr(0, lastDot);
        if (isBootable) outputFile += ".img";
        else if (isBareMetal) outputFile += ".bin";
    }

    try {
        auto compileStart = std::chrono::high_resolution_clock::now();

        std::cout << "\033[1;34m[1/6] Parsing & Constructing AST/CST...\033[0m\n";
        auto rootProgram = std::make_unique<ProgramNode>();
        ModuleLoader loader;
        auto loadRes = loader.loadAndMerge(sourceFile, rootProgram);
        if (loadRes.isErr()) {
            std::cerr << "\033[1;31m" << loadRes.unwrapErr() << "\033[0m\n";
            return 1;
        }

        std::cout << "\033[1;34m[2/6] Monomorphizing Generics...\033[0m\n";
        Monomorphizer monomorphizer;
        monomorphizer.process(rootProgram.get());

        std::cout << "\033[1;34m[3/6] Static Type Checking & Record Layout Computation...\033[0m\n";
        TypeChecker typeChecker(selectedArch);
        auto typeRes = typeChecker.checkProgram(rootProgram.get());
        if (typeRes.isErr()) {
            std::cerr << "\033[1;31m[Type Errors Detected]:\033[0m\n";
            for (const auto& err : typeRes.unwrapErr()) {
                std::cerr << "  - " << err << "\n";
            }
            return 1;
        }

        std::cout << "\033[1;34m[4/6] Lowering Scopes & Generating SSA IR (Entry: " << entryPoint << ")...\033[0m\n";
        RegionLowering lowering;
        std::unique_ptr<ASTNode> astRoot = std::move(rootProgram);
        lowering.lowerRegions(astRoot);

        SSABuilder ssaBuilder;
        ssaBuilder.setTargetArch(selectedArch);
        ssaBuilder.setEntryPoint(entryPoint);
        auto ssaProgram = ssaBuilder.buildProgram(static_cast<ProgramNode*>(astRoot.get()));

        if (optimizeO3) {
            std::cout << "\033[1;33m[5/6] Running Optimization Passes (-O3: DCE, LICM, Constant Propagation, AV)...\033[0m\n";
            OptimizationPassManager optManager;
            optManager.runOptimizations(ssaProgram, true);
        } else {
            std::cout << "\033[1;34m[5/6] Skipping Middle-End Optimizations (-O0)...\033[0m\n";
        }

        std::cout << "\033[1;34m[6/6] Generating Native Machine Code...\033[0m\n";
        std::vector<uint8_t> machineCode;

        if (isBareMetal) {
            if (selectedArch == ArchType::X86_32) {
                BareMetalX86_32Encoder baremetal32Encoder;
                machineCode = baremetal32Encoder.encodeProgram(ssaProgram);
            } else if (isModernEngine) {
                std::cout << "\033[1;32m[Info] Using Modern BareMetal Engine\033[0m\n";
                BareMetalX86ModernEncoder modernEncoder;
                machineCode = modernEncoder.encodeProgram(ssaProgram);
            } else {
                std::cout << "\033[1;32m[Info] Using Standard BareMetal Engine\033[0m\n";
                BareMetalX86Encoder baremetalEncoder;
                machineCode = baremetalEncoder.encodeProgram(ssaProgram);
            }

            auto compileEnd = std::chrono::high_resolution_clock::now();
            auto compileDurationUs = std::chrono::duration_cast<std::chrono::microseconds>(compileEnd - compileStart).count();
            std::cout << "\033[1;32m[Compilation Finished in " << compileDurationUs / 1000.0 << " ms]\033[0m\n";

            if (isBootable) {
                RawBinaryBuilder::buildBootableImage(outputFile, machineCode);
                std::cout << "\033[1;32m[Success] Bootable Disk Image built -> " << outputFile << " (" << machineCode.size() << " bytes kernel)\033[0m\n";
            } else {
                RawBinaryBuilder::buildRawBinary(outputFile, machineCode);
                std::cout << "\033[1;32m[Success] Raw Kernel Binary built -> " << outputFile << " (" << machineCode.size() << " bytes)\033[0m\n";
            }
            return 0;
        }

        X86_64Encoder hostedEncoder;
        for (const auto& stmt : static_cast<ProgramNode*>(astRoot.get())->stmts) {
            if (stmt && stmt->type == ASTNodeType::ExternFuncDecl) {
                auto ext = static_cast<ExternFuncDeclNode*>(stmt.get());
                hostedEncoder.registerExternalSymbol(ext->library, ext->name);
            }
        }
        machineCode = hostedEncoder.encodeProgram(ssaProgram);

        if (optimizeO3) {
            machineCode = PeepholeX86::optimize(machineCode);
        }

        auto compileEnd = std::chrono::high_resolution_clock::now();
        auto compileDurationUs = std::chrono::duration_cast<std::chrono::microseconds>(compileEnd - compileStart).count();
        std::cout << "\033[1;32m[Compilation Finished in " << compileDurationUs / 1000.0 << " ms]\033[0m\n";

        if (command == "run") {
            JITEngine jit;
            std::cout << "\033[1;32m----------------- [Execution Output] -----------------\033[0m\n";
            auto runStartTime = std::chrono::high_resolution_clock::now();

            int64_t exitCode = jit.execute(machineCode);

            auto runEndTime = std::chrono::high_resolution_clock::now();
            auto runDurationUs = std::chrono::duration_cast<std::chrono::microseconds>(runEndTime - runStartTime).count();

            std::cout << "\033[1;32m------------------------------------------------------\033[0m\n";
            std::cout << "\033[1;36m[Process Exited with Code: " << exitCode << " | Execution Time: " << runDurationUs / 1000.0 << " ms]\033[0m\n";
        } 
        else if (command == "build") {
            ELFBuilder::buildExecutable(outputFile, machineCode, selectedArch);
            std::cout << "\033[1;32m[Success] Standalone Executable built -> " << outputFile << " (" << machineCode.size() << " bytes)\033[0m\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "\033[1;31m[Fatal Error] " << e.what() << "\033[0m\n";
        return 1;
    }

    return 0;
}
