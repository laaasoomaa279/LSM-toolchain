#ifndef LSM_GRAPH_COLORING_HPP
#define LSM_GRAPH_COLORING_HPP

#include "PhysicalRegister.hpp"
#include "LivenessAnalyzer.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <algorithm>

class GraphColoringAllocator {
private:
    struct Node {
        std::string varName;
        LsmStaticType type;
        std::unordered_set<std::string> neighbors;
        int color = -1;
        bool isSpilled = false;
        int stackOffset = 0;
    };

public:
    AllocationResult allocate(std::vector<LiveInterval>& intervals, ArchType arch = ArchType::X86_64) {
        AllocationResult result;
        if (intervals.empty()) return result;

        std::unordered_map<std::string, Node> graph;
        for (const auto& inter : intervals) {
            graph[inter.varName] = {inter.varName, inter.type, {}, -1, false, 0};
        }

        
        for (size_t i = 0; i < intervals.size(); ++i) {
            for (size_t j = i + 1; j < intervals.size(); ++j) {
                if (intervals[i].overlaps(intervals[j])) {
                    graph[intervals[i].varName].neighbors.insert(intervals[j].varName);
                    graph[intervals[j].varName].neighbors.insert(intervals[i].varName);
                }
            }
        }

        std::vector<int> availableGPRs;
        std::vector<int> availableFPRs;
        if (arch == ArchType::X86_64) {
            availableGPRs = {10, 11, 3, 12, 13, 14, 15};
            availableFPRs = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        } else {
            availableGPRs = {9, 10, 11, 12, 13, 14, 15, 19, 20, 21, 22, 23};
            availableFPRs = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
        }

        size_t kGPR = availableGPRs.size();
        size_t kFPR = availableFPRs.size();

        std::stack<std::string> selectStack;
        std::unordered_set<std::string> removedNodes;

        
        bool progress = true;
        while (removedNodes.size() < graph.size()) {
            progress = false;

            for (auto& pair : graph) {
                if (removedNodes.count(pair.first)) continue;

                size_t activeDegree = 0;
                for (const auto& neighbor : pair.second.neighbors) {
                    if (!removedNodes.count(neighbor)) activeDegree++;
                }

                size_t kLimit = (pair.second.type == LsmStaticType::Float64) ? kFPR : kGPR;

                if (activeDegree < kLimit) {
                    selectStack.push(pair.first);
                    removedNodes.insert(pair.first);
                    progress = true;
                    break;
                }
            }

            
            if (!progress) {
                std::string maxNode = "";
                size_t maxDegree = 0;
                for (auto& pair : graph) {
                    if (removedNodes.count(pair.first)) continue;
                    size_t deg = 0;
                    for (const auto& neighbor : pair.second.neighbors) {
                        if (!removedNodes.count(neighbor)) deg++;
                    }
                    if (deg >= maxDegree) {
                        maxDegree = deg;
                        maxNode = pair.first;
                    }
                }
                if (!maxNode.empty()) {
                    selectStack.push(maxNode);
                    removedNodes.insert(maxNode);
                }
            }
        }

        int currentSpillOffset = 0;

        
        while (!selectStack.empty()) {
            std::string varName = selectStack.top();
            selectStack.pop();

            auto& node = graph[varName];
            bool isFloat = (node.type == LsmStaticType::Float64);
            const auto& colorsPool = isFloat ? availableFPRs : availableFPRs;

            std::unordered_set<int> usedColors;
            for (const auto& neighbor : node.neighbors) {
                if (graph[neighbor].color != -1) {
                    usedColors.insert(graph[neighbor].color);
                }
            }

            int assignedColor = -1;
            for (int c : colorsPool) {
                if (!usedColors.count(c)) {
                    assignedColor = c;
                    break;
                }
            }

            if (assignedColor != -1) {
                node.color = assignedColor;
                result.varToReg[varName] = assignedColor;
            } else {
                node.isSpilled = true;
                currentSpillOffset -= 8;
                node.stackOffset = currentSpillOffset;
                result.varToStack[varName] = currentSpillOffset;
            }
        }

        result.totalStackSpillBytes = -currentSpillOffset;
        return result;
    }
};

#endif 