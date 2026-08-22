#ifndef LSM_LIVENESS_ANALYZER_HPP
#define LSM_LIVENESS_ANALYZER_HPP

#include "../../middleend/ssa/SSAInstruction.hpp"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>

struct LiveInterval {
    std::string varName;
    LsmStaticType type;
    size_t startPoint = 0;
    size_t endPoint = 0;
    bool isSpilled = false;
    int stackOffset = 0;
    int assignedPhysicalReg = -1;

    bool contains(size_t point) const {
        return point >= startPoint && point <= endPoint;
    }

    bool overlaps(const LiveInterval& other) const {
        return !(endPoint < other.startPoint || startPoint > other.endPoint);
    }
};

class LivenessAnalyzer {
public:
    static std::vector<LiveInterval> computeIntervals(const SSAFunction& func) {
        std::unordered_map<std::string, size_t> defPoints;
        std::unordered_map<std::string, size_t> lastUsePoints;
        std::unordered_map<std::string, LsmStaticType> varTypes;

        size_t instructionIndex = 0;

        for (const auto& param : func.params) {
            defPoints[param.first] = 0;
            lastUsePoints[param.first] = 0;
            varTypes[param.first] = param.second;
        }

        for (const auto& block : func.blocks) {
            for (const auto& inst : block.instructions) {
                instructionIndex++;

                for (const auto& op : inst.operands) {
                    if (!op.isConstant && !op.name.empty()) {
                        if (defPoints.find(op.name) == defPoints.end()) {
                            defPoints[op.name] = instructionIndex;
                        }
                        lastUsePoints[op.name] = instructionIndex;
                        varTypes[op.name] = op.type;
                    }
                }

                if (!inst.result.name.empty()) {
                    if (defPoints.find(inst.result.name) == defPoints.end()) {
                        defPoints[inst.result.name] = instructionIndex;
                    }
                    if (lastUsePoints.find(inst.result.name) == lastUsePoints.end()) {
                        lastUsePoints[inst.result.name] = instructionIndex;
                    }
                    varTypes[inst.result.name] = inst.result.type;
                }
            }
        }

        std::vector<LiveInterval> intervals;
        for (const auto& pair : defPoints) {
            std::string vName = pair.first;
            size_t startP = pair.second;
            size_t endP = lastUsePoints[vName];
            if (endP < startP) endP = startP;

            intervals.push_back({vName, varTypes[vName], startP, endP, false, 0, -1});
        }

        std::sort(intervals.begin(), intervals.end(), [](const LiveInterval& a, const LiveInterval& b) {
            return a.startPoint < b.startPoint;
        });

        return intervals;
    }
};

#endif 