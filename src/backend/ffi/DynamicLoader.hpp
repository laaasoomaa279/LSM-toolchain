#ifndef LSM_DYNAMIC_LOADER_HPP
#define LSM_DYNAMIC_LOADER_HPP

#include <string>
#include <unordered_map>
#include <cstdint>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

class DynamicLoader {
private:
    std::unordered_map<std::string, void*> loadedLibraries;
    std::unordered_map<std::string, uintptr_t> symbolCache;

public:
    DynamicLoader() = default;
    ~DynamicLoader() {
        for (auto& pair : loadedLibraries) {
            if (pair.second) {
#if defined(_WIN32) || defined(_WIN64)
                FreeLibrary(static_cast<HMODULE>(pair.second));
#else
                dlclose(pair.second);
#endif
            }
        }
    }

    uintptr_t resolveSymbol(const std::string& libraryName, const std::string& symbolName) {
        std::string key = libraryName + "::" + symbolName;
        if (symbolCache.count(key)) return symbolCache[key];

        void* handle = nullptr;
        if (loadedLibraries.count(libraryName)) {
            handle = loadedLibraries[libraryName];
        } else {
#if defined(_WIN32) || defined(_WIN64)
            handle = LoadLibraryA(libraryName.c_str());
#else
            std::string libPath = libraryName;
            if (libPath.find(".so") == std::string::npos && libPath.find(".dll") == std::string::npos) {
                libPath = "lib" + libraryName + ".so";
            }
            handle = dlopen(libPath.c_str(), RTLD_LAZY | RTLD_GLOBAL);
#endif
            if (handle) loadedLibraries[libraryName] = handle;
        }

        if (!handle) return 0;

        uintptr_t funcPtr = 0;
#if defined(_WIN32) || defined(_WIN64)
        funcPtr = reinterpret_cast<uintptr_t>(GetProcAddress(static_cast<HMODULE>(handle), symbolName.c_str()));
#else
        funcPtr = reinterpret_cast<uintptr_t>(dlsym(handle, symbolName.c_str()));
#endif
        if (funcPtr) symbolCache[key] = funcPtr;
        return funcPtr;
    }
};

#endif 