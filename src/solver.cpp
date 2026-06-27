#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include "../include/dlx_solver.hpp" // Links the core engine

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

    DLXSolver dlx(324);

    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            int fixedValue = board[r][c];
            int box = (r / 3) * 3 + (c / 3);
            for (int d = 1; d <= 9; ++d) {
                if (fixedValue && fixedValue != d) continue;
                
                int rowId = r * 81 + c * 9 + (d - 1);
                dlx.addRow({
                    r * 9 + c,                  
                    81 + r * 9 + (d - 1),       
                    162 + c * 9 + (d - 1),      
                    243 + box * 9 + (d - 1)     
                }, rowId);
            }
        }
    }

    auto t0 = std::chrono::high_resolution_clock::now();
    int branches = 0;
    bool hasSol = dlx.solve(branches);
    auto t1 = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> duration = t1 - t0;

    if (!hasSol) {
        std::cout << "No valid solution exists for the provided puzzle configuration." << std::endl;
        return;
    }

    // Retrieve row IDs using the helper function in our header
    std::vector<int> sol = dlx.getSolutionRowIds();
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