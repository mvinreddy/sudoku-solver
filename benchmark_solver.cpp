#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <thread>
#include <atomic>

struct Node {
    Node *L, *R, *U, *D;
    Node* C;
    int rowId;
};

struct ColumnHeader : public Node {
    int size;
    int name;
};

class DLXSolver {
private:
    ColumnHeader* header;
    std::vector<ColumnHeader*> cols;
    std::vector<int> solution;
    int branches;

    std::vector<Node> nodePool;
    size_t poolIdx;

    Node* allocateNode() {
        return &nodePool[poolIdx++];
    }

    void cover(Node* colNode) {
        ColumnHeader* col = static_cast<ColumnHeader*>(colNode);
        col->R->L = col->L;
        col->L->R = col->R;
        for (Node* i = col->D; i != col; i = i->D) {
            for (Node* j = i->R; j != i; j = j->R) {
                j->D->U = j->U;
                j->U->D = j->D;
                static_cast<ColumnHeader*>(j->C)->size--;
            }
        }
    }

    void uncover(Node* colNode) {
        ColumnHeader* col = static_cast<ColumnHeader*>(colNode);
        for (Node* i = col->U; i != col; i = i->U) {
            for (Node* j = i->L; j != i; j = j->L) {
                static_cast<ColumnHeader*>(j->C)->size++;
                j->D->U = j;
                j->U->D = j;
            }
        }
        col->R->L = col;
        col->L->R = col;
    }

    bool search() {
        if (header->R == header) return true;

        ColumnHeader* best = static_cast<ColumnHeader*>(header->R);
        for (Node* c = header->R; c != header; c = c->R) {
            ColumnHeader* col = static_cast<ColumnHeader*>(c);
            if (col->size < best->size) {
                best = col;
            }
        }

        if (best->size == 0) return false;

        cover(best);
        for (Node* row = best->D; row != best; row = row->D) {
            branches++;
            solution.push_back(row->rowId);
            for (Node* j = row->R; j != row; j = j->R) {
                cover(j->C);
            }

            if (search()) return true;

            for (Node* j = row->L; j != row; j = j->L) {
                uncover(j->C);
            }
            solution.pop_back();
        }
        uncover(best);
        return false;
    }

public:
    DLXSolver(int numCols) : branches(0), poolIdx(0) {
        nodePool.resize(3200);

        header = new ColumnHeader();
        header->name = -1;
        header->size = 0;
        header->L = header->R = header->U = header->D = header;

        cols.resize(numCols);
        for (int i = 0; i < numCols; ++i) {
            cols[i] = new ColumnHeader();
            cols[i]->name = i;
            cols[i]->size = 0;
            cols[i]->U = cols[i]->D = cols[i];
        }
        reset();
    }

    ~DLXSolver() {
        for (auto col : cols) delete col;
        delete header;
    }

    // Completely clears and rewires the matrix structure without dropping allocation vectors
    void reset() {
        branches = 0;
        poolIdx = 0;
        solution.clear();

        header->L = header->R = header->U = header->D = header;
        for (int i = 0; i < 324; ++i) {
            cols[i]->size = 0;
            cols[i]->U = cols[i]->D = cols[i];
            
            Node* last = header->L;
            cols[i]->L = last;
            cols[i]->R = header;
            last->R = cols[i];
            header->L = cols[i];
        }
    }

    void addRow(const std::vector<int>& colIdxs, int rowId) {
        Node* first = nullptr;
        Node* prev = nullptr;
        for (int ci : colIdxs) {
            ColumnHeader* col = cols[ci];
            Node* node = allocateNode();
            node->C = col;
            node->rowId = rowId;
            
            node->U = col->U;
            node->D = col;
            col->U->D = node;
            col->U = node;
            col->size++;

            if (!first) {
                first = node;
                node->L = node->R = node;
            } else {
                node->L = prev;
                node->R = first;
                prev->R = node;
                first->L = node;
            }
            prev = node;
        }
    }

    bool solve(int& totalBranches) {
        bool solved = search();
        totalBranches = branches;
        return solved;
    }
};

void workerTask(const std::vector<std::string>& batch, size_t startIdx, size_t endIdx, 
                std::atomic<long long>& globalBranches, std::atomic<int>& solvedCount) {
    int localSolved = 0;
    long long localBranches = 0;
    
    // ONE allocation per thread worker, completely bypassing hot loop allocations
    DLXSolver dlx(324);

    for (size_t i = startIdx; i < endIdx; ++i) {
        const std::string& line = batch[i];
        if (line.length() < 81) continue;

        dlx.reset(); // Zero-allocation reset loop

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

    workers.clear(); // Safe synchronization barrier

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