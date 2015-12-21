#include <iostream>
#include <string>
#include <cassert>
#include <unordered_set>
#include <ctime>
#include <vector>
#include <algorithm>
#include <thread>
#include <climits>
#include <signal.h>

using namespace std;

const int NMAX = 999;
typedef float* deg_t;
deg_t degs[NMAX][2*NMAX];
volatile bool stopall = false;


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
    if (dy<0) {
        dx = -dx;
        dy = -dy;
    }
    return degs[dy][NMAX+dx];
}

class Solver {
    int N;
    bool cols[NMAX];
    bool ul[2*NMAX];
    bool ur[2*NMAX];
    int board[NMAX];

    time_t starttime;

    class FoundSolution : public std::exception {};
    class Timeout       : public std::exception {};

    int field_cost(int row, int col, bool halfboard=false) {
        int maxrow = halfboard?row:N;
        int maxazimuths = halfboard?row:N-1;
        int cost = 0;
        int ur = row+col;
        int ul = row+N-col;
        unordered_set<deg_t> azimuths;
        for (int q=0; q<maxrow; ++q) {
            if (q==row)
                continue;
            int qur = q+board[q];
            int qul = q+N-board[q];
            if (qur==ur || qul==ul || board[q]==col)
                cost+=1;
            int dy = row-q;
            int dx = col - board[q];
            deg_t deg = get_deg(dy, dx);
            azimuths.insert(deg);
        }
        cost += (maxazimuths-azimuths.size());
        return cost;
    }

    void search() {
        int iteration = 0;
        while (true) {
            if (stopall) {
                cout << "aborting after " << iteration <<" iterations" <<endl;
                return;
            }
            ++iteration;
            int row = iteration%10!=0?find_maxcost_row():find_nonzerocost_row();
//            int row = find_nonzerocost_row();
            if (row<0) {
                cout << "found after " << iteration <<" iterations"<<endl;
                throw FoundSolution();
            }
            if (rand()%50==0) {
                // to break out of local minimum
                cout << "randomizing row "<<row<<endl;
                board[row] = rand()%N;
            } else {
                improve_row(row);
            }
        }
    }

    void initialize_board() {
        cout <<"initializing board ";
        for (int r=0; r<N; ++r) {
            extend_board(r);
            cout <<r <<" " << flush;
        }
    }

    void extend_board(int r, bool real_extend = false)
    {
        if (real_extend)
            ++N;
        vector<int> mincols;
        mincols.reserve(N);
        int mincost = INT_MAX;
        for (int c=0; c<N; ++c) {
            int cost = field_cost(r,c, true);
            if (cost<mincost) {
                mincols.clear();
                mincost = cost;
            }
            if (cost == mincost) {
                mincols.push_back(c);
            }
        }
        board[r] = mincols[rand()%mincols.size()];
    }

    int find_maxcost_row() {
        vector<int> maxrows;
        vector<int> badrows;
        maxrows.reserve(N);
        int maxcost = 0;
        for (int r=0; r<N; ++r) {
            int cost = field_cost(r, board[r]);
            if (cost>0)
                badrows.push_back(r);
            if (cost>maxcost) {
                maxrows.clear();
                maxcost = cost;
            }
            if (cost == maxcost) {
                maxrows.push_back(r);
            }
        }
        if (maxcost==0)
            return -1;
        int row = maxrows[rand()%maxrows.size()];
        cout << "fixing row " << row
             << " with cost "<<maxcost
             << ", bad rows num="<< badrows.size()
             <<endl;
        return row;
    }

    int find_nonzerocost_row() {
        vector<int> badrows;
        badrows.reserve(N);
        int maxcost = 0;
        for (int r=0; r<N; ++r) {
            int cost = field_cost(r, board[r]);
            if (cost>0) {
                maxcost = max(maxcost, cost);
                badrows.push_back(r);
            }
        }
        if (badrows.empty())
            return -1;
        int row = badrows[rand()%badrows.size()];
        cout << "fixing row " << row
             << ", while max cost "<<maxcost
             << ", bad rows num="<< badrows.size()<<endl;
        return row;
    }

    void improve_row(int row) {
        vector<int> mincols;
        mincols.reserve(N);
        int mincost = field_cost(row, board[row]);
        for (int c=0; c<N; ++c) {
            if (c==board[row])
                continue;
            int cost = field_cost(row, c);
            if (cost<mincost) {
                mincols.clear();
                mincost = cost;
            }
            if (cost == mincost) {
                mincols.push_back(c);
            }
        }
        if (!mincols.empty())
            board[row] = mincols[rand()%mincols.size()];
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

public:
    Solver(int n) : N(n){
        starttime = clock();
//        randomize();
    }

    vector<int> operator()(int maxn=-1) {
        assert(maxn < NMAX && "maxn shall not be greater than anticipated");
        if (maxn<0)
            maxn = N;
        initialize_board();
        while (true) {
            try {
                search();
            } catch(FoundSolution) {
                // noop
            } catch (Timeout) {
//                cout << "start again" <<endl;
                continue;
            }
            print_solution();
            if (N<maxn && !stopall) {
                extend_board(N, true);
                continue;
            }
            vector<int> ret(board, board+N);
            return ret;
        }

        return vector<int>();
    }
};

void set_stopall(int) {
    stopall = true;
}




int main()
{
    signal(SIGINT, set_stopall);
    signal(SIGTERM, set_stopall);
    srand(4);
    srand(time(0));
    calculate_degs();
    Solver s(50);
    s();
    return 0;
}

