#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>

class LSMBuilder {
private:
    std::string projectName = "lsm_project";
    std::string targetTarget = "host"; 
    std::string optLevel = "-O0";
    std::vector<std::string> sourceFiles;

public:
    LSMBuilder() = default;

    void parseConfig(const std::string& configPath) {
        std::ifstream file(configPath);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            if (line.rfind("name=", 0) == 0) projectName = line.substr(5);
            else if (line.rfind("target=", 0) == 0) targetTarget = line.substr(7);
            else if (line.rfind("opt=", 0) == 0) optLevel = line.substr(4);
            else if (line.rfind("src=", 0) == 0) sourceFiles.push_back(line.substr(4));
        }
    }

    void build() {
        std::cout << "\033[1;36m[*] LSM Build Engine Starting for project: " << projectName << "\033[0m\n";
        
        std::string command = "lsm build ";
        if (targetTarget == "baremetal") {
            command += "--baremetal ";
        }
        if (optLevel == "-O3") {
            command += "-O3 ";
        }

        for (const auto& src : sourceFiles) {
            command += src + " ";
        }

        command += "-o " + projectName;
        if (targetTarget == "baremetal") command += ".bin";

        std::cout << "\033[1;33m[Executing]: " << command << "\033[0m\n";
        int res = std::system(command.c_str());

        if (res == 0) {
            std::cout << "\033[1;32m[Success] Project built successfully!\033[0m\n";
        } else {
            std::cerr << "\033[1;31m[Error] Build failed with exit code: " << res << "\033[0m\n";
        }
    }
};

int main(int argc, char* argv[]) {
    LSMBuilder builder;
    builder.parseConfig("lsm.proj");
    builder.build();
    return 0;
}