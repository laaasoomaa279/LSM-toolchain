#ifndef LSM_REGISTER_ALLOCATOR_HPP
#define LSM_REGISTER_ALLOCATOR_HPP

#include "LinearScanAllocator.hpp"
#include "GraphColoring.hpp"

enum class RegAllocStrategy {
    LinearScan,
    GraphColoring
};

class RegisterAllocator {
private:
    LinearScanAllocator linearScan;
    GraphColoringAllocator graphColoring;

public:
    AllocationResult allocateFunction(const SSAFunction& func, 
                                     ArchType arch = ArchType::X86_64, 
                                     RegAllocStrategy strategy = RegAllocStrategy::LinearScan) {
        
        std::vector<LiveInterval> intervals = LivenessAnalyzer::computeIntervals(func);

        if (strategy == RegAllocStrategy::GraphColoring) {
            return graphColoring.allocate(intervals, arch);
        } else {
            return linearScan.allocate(intervals, arch);
        }
    }
};

#endif 