#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <thread>
#include <atomic>
#include "../include/dlx_solver.hpp" // Links the core engine

void workerTask(const std::vector<std::string>& batch, size_t startIdx, size_t endIdx, 
                std::atomic<long long>& globalBranches, std::atomic<int>& solvedCount) {
    int localSolved = 0;
    long long localBranches = 0;
    
    DLXSolver dlx(324);

    for (size_t i = startIdx; i < endIdx; ++i) {
        const std::string& line = batch[i];
        if (line.length() < 81) continue;

        dlx.reset(); 

        int board[9][9];
        for (int j = 0; j < 81; ++j) {
            char ch = line[j];
            board[j / 9][j % 9] = (ch == '.' || ch == '0') ? 0 : ch - '0';
        }

        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                int val = board[r][c];
                int box = (r / 3) * 3 + (c / 3);
                for (int d = 1; d <= 9; ++d) {
                    if (val && val != d) continue;
                    dlx.addRow({
                        r * 9 + c,
                        81 + r * 9 + (d - 1),
                        162 + c * 9 + (d - 1),
                        243 + box * 9 + (d - 1)
                    }, r * 81 + c * 9 + (d - 1));
                }
            }
        }

        int puzzleBranches = 0;
        if (dlx.solve(puzzleBranches)) {
            localSolved++;
        }
        localBranches += puzzleBranches;
    }

    solvedCount += localSolved;
    globalBranches += localBranches;
}

int main() {
    std::cout << "==== OPTIMIZED MULTI-THREADED BENCHMARK HARNESS ====" << std::endl;
    std::cout << "Input puzzle strings line-by-line. Press Enter on an empty line to run." << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    std::vector<std::string> puzzles;
    std::string currentLine;
    
    while (std::getline(std::cin, currentLine)) {
        if (currentLine.empty()) break; 
        puzzles.push_back(currentLine);
    }

    size_t totalPuzzles = puzzles.size();
    if (totalPuzzles == 0) return 1;

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    std::cout << "\nLoaded " << totalPuzzles << " puzzles." << std::endl;
    std::cout << "Running on " << numThreads << " parallel thread contexts..." << std::endl;

    std::atomic<long long> totalBranches{0};
    std::atomic<int> totalSolved{0};

    auto t0 = std::chrono::high_resolution_clock::now();

    std::vector<std::jthread> workers;
    size_t chunkSize = totalPuzzles / numThreads;
    size_t remainder = totalPuzzles % numThreads;
    size_t currentStart = 0;

    for (unsigned int i = 0; i < numThreads; ++i) {
        size_t currentEnd = currentStart + chunkSize + (i < remainder ? 1 : 0);
        if (currentStart >= totalPuzzles) break;

        workers.emplace_back(workerTask, std::cref(puzzles), currentStart, currentEnd, 
                             std::ref(totalBranches), std::ref(totalSolved));
        currentStart = currentEnd;
    }

    workers.clear(); 

    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = t1 - t0;

    double avgTimePerPuzzle = (duration.count() * 1000.0) / totalPuzzles;
    double puzzlesPerSecond = totalPuzzles / duration.count();

    std::cout << "\n========= FINAL STATS =========" << std::endl;
    std::cout << "Throughput : " << puzzlesPerSecond << " puzzles/sec" << std::endl;
    std::cout << "Avg Latency: " << avgTimePerPuzzle << " ms per grid" << std::endl;
    std::cout << "===============================" << std::endl;

    return 0;
}