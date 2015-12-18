#include <iostream>
#include <string>
#include <cassert>
#include <unordered_set>
#include <ctime>
#include <vector>
#include <algorithm>
#include <thread>
#include <limits>

using namespace std;

const int NMAX = 999;
typedef float* deg_t;
deg_t degs[NMAX][2*NMAX];


int nwd(int a, int b)
{
    if(b!=0)
        return nwd(b,a%b);

    return a;
}

void calculate_degs() {
    const int &N = NMAX;
    for (int dx=1; dx<N; ++dx) for (int dy=1; dy<N; ++dy) {
        int div = nwd(dx, dy);
        if (div==1) {
            deg_t basedeg_pos = degs[dy][N+dx] = new float;
            deg_t basedeg_neg = degs[dy][N-dx] = new float;
            *basedeg_pos = +1.0*dy/dx;
            *basedeg_neg = -1.0*dy/dx;
        } else {
            int dx2 = dx/div, dy2 = dy/div;
            degs[dy][N+dx] = degs[dy2][N+dx2];
            degs[dy][N-dx] = degs[dy2][N-dx2];
        }
    }
}
deg_t get_deg(int dy, int dx) {
    return degs[dy][NMAX+dx];
}

class Solver {
    int N;
    bool cols[NMAX];
    bool ul[2*NMAX];
    bool ur[2*NMAX];
    // 39 int board[N] = {26, 28, 2, 24, 9, 23, 14, 32, 22, 37, 25, 1, 4, 29, 7, 34, 3, 15, 20, 18, 11, 38, 31, 0, 12, 36, 30, 6, 17, 5, 13, 27, 33, 8, 16, 19, 21, 10, 35};
    // 51: 18, 30, 24, 11, 28, 19, 31, 0, 43, 25, 29, 10, 13, 47, 50, 12, 3, 44, 48, 17, 8, 23, 45, 26, 5, 40, 46, 6, 34, 14, 41, 27, 49, 2, 16, 21, 4, 42, 32, 39, 35, 1, 15, 20, 36, 9, 22, 37, 7, 38, 33
    // = {3, 13, 18, 28, 17, 39, 0, 45, 38, 8, 33, 47, 21, 35, 34, 42, 2, 32, 30, 23, 9, 27, 24, 16, 43, 44, 31, 12, 29, 15, 6, 48, 14, 22, 1, 20, 37, 11, 26, 46, 25, 40, 10, 19, 5, 41, 7, 36, 4};
    int board[NMAX];
    unordered_set<deg_t> queen_pairs[NMAX];

    time_t starttime;
    int tries_left = TRIES_MAX;
    const int TRIES_MAX = 1000000; //numeric_limits<int>::max();

    class Queen {
        Solver *par;
        int row;
        int col;
    public:
        Queen(Solver*parent, int r, int c) : par(parent), row(r), col(c) {
            par->cols[col] = true;
            par->ur[row+col] = true;
            par->ul[row+par->N-col] = true;
            par->board[row] = col;
            for (int r=0; r<row; ++r) {
                int dy = row-r;
                int dx = col - par->board[r];
                deg_t deg = get_deg(dy, dx);
                par->queen_pairs[row].insert(deg);
            }
        }
        ~Queen() {
            par->cols[col] = false;
            par->ur[row+col] = false;
            par->ul[row+par->N-col] = false;
            par->queen_pairs[row].clear();
        }
    };

    class FoundSolution : public std::exception {};
    class Timeout       : public std::exception {};

    bool can_place(int row, int col) {
        if (cols[col] || ur[row+col] || ul[row+N-col])
            return false;
        for (int r=0; r<row; ++r) {
            int dy = row-r;
            int dx = col - board[r];
            deg_t deg = get_deg(dy, dx);
            if (queen_pairs[r].find(deg) != queen_pairs[r].end()) {
                return false;
            }
        }
        return true;
    }

    void print_solution() {
        cout <<"vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"<<endl;
        cout << "calculated in " << (clock()-starttime)/CLOCKS_PER_SEC<<endl;
        cout << "echo " << N <<endl << "echo ";
        for (int i=0; i<N; ++i)
            cout <<board[i]+1 << " ";
        cout <<endl;
        /*for (int i=0; i<N; ++i)
            cout <<board[i] << ", ";
        cout <<endl;*/
        /*
        for (int r=0; r<N; ++r) {
            for (int c=0; c<board[r]; ++c) {
                cout <<" ";
            }
            cout << "#" <<endl;
        }*/
        cout <<"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"<<endl;
    }

    void search(int row=0) {
        for (int c=0; c++<N; board[row] = (board[row]+1)%N) {
            if (can_place(row, board[row])) {
                Queen q(this, row, board[row]);
                if (row==N-1) {
                    throw FoundSolution();
                } else {
                    if (tries_left>0)
                        --tries_left;
                    if (tries_left==0) {
                        tries_left = TRIES_MAX;
                        throw Timeout();
                    }
                    search(row+1);
                }
            }
        }
    }
    void randomize()
    {
        vector<int> myvector;
        for (int i=0; i<N; ++i) myvector.push_back(i);
        random_shuffle(myvector.begin(), myvector.end());
        copy(myvector.begin(), myvector.end(), this->board);
    }
public:
    Solver(int n, vector<int> starter) : N(n){
        starttime = clock();
        copy(starter.begin(), starter.end(), this->board);
    }

    Solver(int n) : N(n){
        starttime = clock();
        randomize();
    }
    vector<int> operator()() {
        while (true) {
            try {
                search(0);
            } catch(FoundSolution) {
                print_solution();
                vector<int> ret(board, board+N);
                return ret;
            } catch (Timeout) {
//                cout << "start again" <<endl;
                randomize();
            }
        }

        return vector<int>();
    }
};





int main()
{
    srand(time(0));
    calculate_degs();
    for (int i=999; i<1000; i+=2) {
        vector<thread> threads;
        for (int t=0; t<7; ++t) {
            threads.push_back(thread(Solver(i)));
        }
        for (int t=0; t<7; ++t) {
            threads[t].join();
        }
//        Solver s(i);
//        s();
    }
    //    search();
    return 0;
}

