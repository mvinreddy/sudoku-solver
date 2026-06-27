#pragma once
#include <vector>

// 1. Core Data Structures for the Dancing Links Toroidal Matrix
struct Node {
    Node *L, *R, *U, *D;
    Node* C;
    int rowId;
};

struct ColumnHeader : public Node {
    int size;
    int name;
};

// 2. The Reusable Algorithm X Engine
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

    // Direct data access helper for the individual solver to read coordinates
    std::vector<int> getSolutionRowIds() const {
        return solution;
    }
};