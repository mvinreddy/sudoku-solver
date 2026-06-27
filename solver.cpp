#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>

// Structure for a node in the Dancing Links (DLX) toroidally linked matrix
struct Node {
    Node *L, *R, *U, *D;
    Node* C; // Pointer to the column header
    int rowId; // Encodes Row, Column, and Value indices
};

// Structure for the column header node
struct ColumnHeader : public Node {
    int size; // Number of active nodes in this column (used for MRV)
    int name; // ID/Index of the constraint
};

class DLXSolver {
private:
    ColumnHeader* header;
    std::vector<ColumnHeader*> cols;
    std::vector<int> solution;
    int branches;

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
        // If no columns are left, exact cover is completed successfully
        if (header->R == header) return true;

        // MRV Heuristic: Find the column with the minimum remaining candidate rows
        ColumnHeader* best = static_cast<ColumnHeader*>(header->R);
        for (Node* c = header->R; c != header; c = c->R) {
            ColumnHeader* col = static_cast<ColumnHeader*>(c);
            if (col->size < best->size) {
                best = col;
            }
        }

        if (best->size == 0) return false; // Dead-end reached; backtrack early

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
    DLXSolver(int numCols) : branches(0) {
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
            
            Node* last = header->L;
            cols[i]->L = last;
            cols[i]->R = header;
            last->R = cols[i];
            header->L = cols[i];
        }
    }

    ~DLXSolver() {
        // Note: Clean implementation omits explicit individual node deallocations
        // to maximize execution speed via modern bulk process memory tear-downs.
        delete header;
    }

    void addRow(const std::vector<int>& colIdxs, int rowId) {
        Node* first = nullptr;
        Node* prev = nullptr;
        for (int ci : colIdxs) {
            ColumnHeader* col = cols[ci];
            Node* node = new Node();
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

    std::vector<int> getSolution(int& totalBranches) {
        if (search()) {
            totalBranches = branches;
            return solution;
        }
        totalBranches = branches;
        return {};
    }
};

void parseAndSolve(const std::string& inputStr) {
    if (inputStr.length() != 81) {
        std::cerr << "Error: Puzzle input length must be exactly 81 characters." << std::endl;
        return;
    }

    int board[9][9];
    for (int i = 0; i < 81; ++i) {
        char ch = inputStr[i];
        if (ch == '.' || ch == '0') {
            board[i / 9][i % 9] = 0;
        } else if (ch >= '1' && ch <= '9') {
            board[i / 9][i % 9] = ch - '0';
        } else {
            std::cerr << "Error: Invalid character '" << ch << "' in puzzle grid." << std::endl;
            return;
        }
    }

    // 4 exact cover constraints: Cell, Row, Column, Box
    const int NUM_COLS = 324; 
    DLXSolver dlx(NUM_COLS);

    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            int fixedValue = board[r][c];
            int box = (r / 3) * 3 + (c / 3);
            for (int d = 1; d <= 9; ++d) {
                if (fixedValue && fixedValue != d) continue;
                
                int rowId = r * 81 + c * 9 + (d - 1);
                dlx.addRow({
                    r * 9 + c,                  // 1. Cell constraint
                    81 + r * 9 + (d - 1),       // 2. Row constraint
                    162 + c * 9 + (d - 1),      // 3. Column constraint
                    243 + box * 9 + (d - 1)     // 4. Box constraint
                }, rowId);
            }
        }
    }

    auto t0 = std::chrono::high_resolution_clock::now();
    int branches = 0;
    std::vector<int> sol = dlx.getSolution(branches);
    auto t1 = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> duration = t1 - t0;

    if (sol.empty()) {
        std::cout << "No valid solution exists for the provided puzzle configuration." << std::endl;
        return;
    }

    // Unpack Row IDs back into matrix coordinates
    for (int id : sol) {
        int r = id / 81;
        int c = (id % 81) / 9;
        int d = (id % 9) + 1;
        board[r][c] = d;
    }

    std::cout << "\n--- SOLVED SUDOKU MATRIX ---" << std::endl;
    for (int r = 0; r < 9; ++r) {
        if (r > 0 && r % 3 == 0) std::cout << "------+-------+------" << std::endl;
        for (int c = 0; c < 9; ++c) {
            if (c > 0 && c % 3 == 0) std::cout << "| ";
            std::cout << board[r][c] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "----------------------------" << std::endl;
    std::cout << "DLX Compute Time : " << duration.count() << " ms" << std::endl;
    std::cout << "Search Branches  : " << branches << std::endl;
}

int main() {
    std::cout << "Enter 81-char string (use 0 or . for empty cells):" << std::endl;
    std::string puzzleInput;
    if (std::cin >> puzzleInput) {
        parseAndSolve(puzzleInput);
    }
    return 0;
}