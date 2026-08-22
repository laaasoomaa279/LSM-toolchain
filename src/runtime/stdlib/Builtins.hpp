#ifndef LSM_RUNTIME_BUILTINS_HPP
#define LSM_RUNTIME_BUILTINS_HPP

#include <iostream>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace LSM::Runtime {

    extern "C" {
        
        inline void lsm_print_str(const char* s) {
            if (s) std::cout << s << "\n";
        }

        inline void lsm_print_num(int64_t n) {
            std::cout << n << "\n";
        }

        inline void lsm_print_float(double f) {
            std::cout << f << "\n";
        }

        
        inline double lsm_sqrt(double x) { return std::sqrt(x); }
        inline double lsm_sin(double x)  { return std::sin(x); }
        inline double lsm_cos(double x)  { return std::cos(x); }
        inline double lsm_tan(double x)  { return std::tan(x); }
        inline double lsm_pow(double base, double exp) { return std::pow(base, exp); }
        inline double lsm_abs(double x)  { return std::fabs(x); }
        inline double lsm_floor(double x){ return std::floor(x); }
        inline double lsm_ceil(double x) { return std::ceil(x); }

        
        inline int64_t lsm_strlen(const char* s) {
            return s ? static_cast<int64_t>(std::strlen(s)) : 0;
        }
    }

} 

#endif 