#ifndef LSM_LINEAR_SCAN_ALLOCATOR_HPP
#define LSM_LINEAR_SCAN_ALLOCATOR_HPP

#include "PhysicalRegister.hpp"
#include "LivenessAnalyzer.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>

struct AllocationResult {
    std::unordered_map<std::string, int> varToReg;
    std::unordered_map<std::string, int> varToStack;
    int totalStackSpillBytes = 0;
};

class LinearScanAllocator {
private:
    std::vector<int> freeGPRs;
    std::vector<int> freeFPRs;
    std::vector<LiveInterval*> activeIntervals;
    int currentSpillOffset = 0;

    void expireOldIntervals(LiveInterval* current) {
        auto it = activeIntervals.begin();
        while (it != activeIntervals.end()) {
            LiveInterval* active = *it;
            if (active->endPoint < current->startPoint) {
                if (active->assignedPhysicalReg != -1) {
                    if (active->type == LsmStaticType::Float64) {
                        if (std::find(freeFPRs.begin(), freeFPRs.end(), active->assignedPhysicalReg) == freeFPRs.end()) {
                            freeFPRs.push_back(active->assignedPhysicalReg);
                        }
                    } else {
                        if (std::find(freeGPRs.begin(), freeGPRs.end(), active->assignedPhysicalReg) == freeGPRs.end()) {
                            freeGPRs.push_back(active->assignedPhysicalReg);
                        }
                    }
                }
                it = activeIntervals.erase(it);
            } else {
                ++it;
            }
        }
    }

    void spillAtInterval(LiveInterval* current) {
        if (activeIntervals.empty()) {
            current->isSpilled = true;
            currentSpillOffset -= 8;
            current->stackOffset = currentSpillOffset;
            return;
        }

        LiveInterval* spillCandidate = activeIntervals.back();

        if (spillCandidate->endPoint > current->endPoint) {
            current->assignedPhysicalReg = spillCandidate->assignedPhysicalReg;
            spillCandidate->assignedPhysicalReg = -1;
            spillCandidate->isSpilled = true;
            currentSpillOffset -= 8;
            spillCandidate->stackOffset = currentSpillOffset;

            activeIntervals.pop_back();
            activeIntervals.push_back(current);
            std::sort(activeIntervals.begin(), activeIntervals.end(), [](LiveInterval* a, LiveInterval* b) {
                return a->endPoint < b->endPoint;
            });
        } else {
            current->isSpilled = true;
            currentSpillOffset -= 8;
            current->stackOffset = currentSpillOffset;
        }
    }

public:
    AllocationResult allocate(std::vector<LiveInterval>& intervals, ArchType arch = ArchType::X86_64) {
        AllocationResult result;
        activeIntervals.clear();
        currentSpillOffset = 0;

        if (arch == ArchType::X86_64) {
            freeGPRs = {10, 11, 3, 12, 13, 14, 15};
            freeFPRs = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        } else {
            freeGPRs = {9, 10, 11, 12, 13, 14, 15, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28};
            freeFPRs = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        }

        for (auto& interval : intervals) {
            expireOldIntervals(&interval);

            bool isFloat = (interval.type == LsmStaticType::Float64);
            auto& freePool = isFloat ? freeFPRs : freeGPRs;

            if (freePool.empty()) {
                spillAtInterval(&interval);
            } else {
                int regId = freePool.back();
                freePool.pop_back();
                interval.assignedPhysicalReg = regId;

                activeIntervals.push_back(&interval);
                std::sort(activeIntervals.begin(), activeIntervals.end(), [](LiveInterval* a, LiveInterval* b) {
                    return a->endPoint < b->endPoint;
                });
            }
        }

        for (const auto& interval : intervals) {
            if (interval.isSpilled) {
                result.varToStack[interval.varName] = interval.stackOffset;
            } else if (interval.assignedPhysicalReg != -1) {
                result.varToReg[interval.varName] = interval.assignedPhysicalReg;
            }
        }

        result.totalStackSpillBytes = -currentSpillOffset;
        return result;
    }
};

#endif 