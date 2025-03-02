#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <iomanip>

using namespace std;

const double EXPLORATION_FACTOR = sqrt(2.0);

// Structure to represent the state of the game
struct GameState
{
    static const int BOARD_SIZE = 3;         // 3x3 board
    int board[BOARD_SIZE][BOARD_SIZE] = {0}; // 0 = empty, 1 = player 1, 2 = player 2
    int currentPlayer = 1;                   // Player 1 starts

    bool isTerminal() const
    {
        // Check if someone won or board is full
        int reward = getReward();
        if (reward != 0)
            return true;

        // Check for draw
        for (int i = 0; i < BOARD_SIZE; ++i)
            for (int j = 0; j < BOARD_SIZE; ++j)
                if (board[i][j] == 0)
                    return false;

        return true; // Board is full (draw)
    }

    vector<GameState> getPossibleMoves() const
    {
        vector<GameState> moves;

        // If game is over, no moves possible
        if (isTerminal())
            return moves;

        // Generate all possible moves
        for (int i = 0; i < BOARD_SIZE; ++i)
        {
            for (int j = 0; j < BOARD_SIZE; ++j)
            {
                if (board[i][j] == 0) // If the cell is empty
                {
                    GameState newState = *this;
                    newState.board[i][j] = currentPlayer;
                    newState.currentPlayer = 3 - currentPlayer; // Switch player (1->2, 2->1)
                    moves.push_back(newState);
                }
            }
        }
        return moves;
    }

    int getReward() const
    {
        // Check rows
        for (int i = 0; i < BOARD_SIZE; ++i)
        {
            if (board[i][0] != 0 && board[i][0] == board[i][1] && board[i][1] == board[i][2])
                return (board[i][0] == 1) ? 1 : -1;
        }

        // Check columns
        for (int i = 0; i < BOARD_SIZE; ++i)
        {
            if (board[0][i] != 0 && board[0][i] == board[1][i] && board[1][i] == board[2][i])
                return (board[0][i] == 1) ? 1 : -1;
        }

        // Check diagonals
        if (board[0][0] != 0 && board[0][0] == board[1][1] && board[1][1] == board[2][2])
            return (board[0][0] == 1) ? 1 : -1;

        if (board[0][2] != 0 && board[0][2] == board[1][1] && board[1][1] == board[2][0])
            return (board[0][2] == 1) ? 1 : -1;

        // No win condition found
        return 0;
    }

    // Compare two game states (needed for the expand() function in MCTSNode)
    bool operator==(const GameState &other) const
    {
        for (int i = 0; i < BOARD_SIZE; ++i)
            for (int j = 0; j < BOARD_SIZE; ++j)
                if (board[i][j] != other.board[i][j])
                    return false;

        return true;
    }

    // Display the board
    void printBoard() const
    {
        cout << "  0 1 2" << endl;
        for (int i = 0; i < BOARD_SIZE; ++i)
        {
            cout << i << " ";
            for (int j = 0; j < BOARD_SIZE; ++j)
            {
                char symbol;
                if (board[i][j] == 0)
                    symbol = '.';
                else if (board[i][j] == 1)
                    symbol = 'X';
                else
                    symbol = 'O';

                cout << symbol << " ";
            }
            cout << endl;
        }
        cout << "Player " << currentPlayer << "'s turn" << endl;
    }

    // Make move based on coordinates
    bool makeMove(int row, int col)
    {
        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE || board[row][col] != 0)
            return false;

        board[row][col] = currentPlayer;
        currentPlayer = 3 - currentPlayer; // Switch players
        return true;
    }
};

// Node structure for MCTS
struct MCTSNode
{
    GameState state;
    MCTSNode *parent;
    vector<MCTSNode *> children;
    int visits;
    double reward;

    MCTSNode(const GameState &state, MCTSNode *parent = nullptr)
        : state(state), parent(parent), visits(0), reward(0.0) {}

    ~MCTSNode()
    {
        // Clean up all children recursively
        for (MCTSNode *child : children)
            delete child;
    }

    bool isFullyExpanded() const
    {
        return children.size() == state.getPossibleMoves().size();
    }

    bool isTerminal() const
    {
        return state.isTerminal();
    }

    MCTSNode *bestChild(bool exploration = true) const
    {
        double bestValue = -numeric_limits<double>::infinity();
        vector<MCTSNode *> bestChildren;

        for (MCTSNode *child : children)
        {
            double exploitValue = child->reward / child->visits;
            double exploreValue = exploration ? EXPLORATION_FACTOR * sqrt(log(visits) / child->visits) : 0;
            double ucb1Value = exploitValue + exploreValue;

            if (ucb1Value > bestValue)
            {
                bestValue = ucb1Value;
                bestChildren.clear();
                bestChildren.push_back(child);
            }
            else if (ucb1Value == bestValue)
            {
                bestChildren.push_back(child);
            }
        }

        // If there are multiple best children, randomly select one
        if (bestChildren.size() > 0)
            return bestChildren[rand() % bestChildren.size()];

        return nullptr; // No children available
    }

    MCTSNode *expand()
    {
        vector<GameState> possibleMoves = state.getPossibleMoves();

        for (const GameState &move : possibleMoves)
        {
            // Check if this move is already expanded
            bool alreadyExpanded = false;
            for (MCTSNode *child : children)
            {
                if (child->state == move)
                {
                    alreadyExpanded = true;
                    break;
                }
            }

            if (!alreadyExpanded)
            {
                MCTSNode *newNode = new MCTSNode(move, this);
                children.push_back(newNode);
                return newNode;
            }
        }
        return nullptr; // No more expansions possible
    }
};

// MCTS Algorithm Functions
MCTSNode *select(MCTSNode *node)
{
    while (!node->isTerminal() && node->isFullyExpanded())
    {
        node = node->bestChild();
        if (node == nullptr)
            break;
    }

    if (!node->isTerminal() && !node->isFullyExpanded())
        return node->expand();

    return node;
}

double simulate(GameState state)
{
    // Random playout until terminal state
    while (!state.isTerminal())
    {
        vector<GameState> moves = state.getPossibleMoves();
        if (moves.empty())
            break;
        state = moves[rand() % moves.size()]; // Randomly select a move
    }
    return state.getReward();
}

void backpropagate(MCTSNode *node, double reward)
{
    while (node != nullptr)
    {
        node->visits++;
        // Negate reward as we alternate perspectives
        node->reward += reward;
        reward = -reward; // Negate for the parent (opponent's perspective)
        node = node->parent;
    }
}

GameState monteCarloTreeSearch(const GameState &rootState, int maxIterations)
{
    MCTSNode *root = new MCTSNode(rootState);

    for (int i = 0; i < maxIterations; ++i)
    {
        // Selection & Expansion
        MCTSNode *selectedNode = select(root);

        // Simulation
        double reward = simulate(selectedNode->state);

        // Backpropagation
        backpropagate(selectedNode, reward);
    }

    // Find best move (child with highest visit count)
    MCTSNode *bestChild = nullptr;
    int maxVisits = -1;

    cout << "Move statistics:" << endl;
    cout << setw(10) << "Row" << setw(10) << "Col" << setw(10) << "Visits" << setw(10) << "Win Rate" << endl;

    for (MCTSNode *child : root->children)
    {
        // Find the move coordinates by comparing boards
        int moveRow = -1, moveCol = -1;
        for (int i = 0; i < GameState::BOARD_SIZE; ++i)
        {
            for (int j = 0; j < GameState::BOARD_SIZE; ++j)
            {
                if (rootState.board[i][j] == 0 && child->state.board[i][j] != 0)
                {
                    moveRow = i;
                    moveCol = j;
                    break;
                }
            }
            if (moveRow != -1)
                break;
        }

        double winRate = child->reward / child->visits;
        cout << setw(10) << moveRow << setw(10) << moveCol
             << setw(10) << child->visits
             << setw(10) << fixed << setprecision(2) << (winRate + 1) / 2 * 100 << "%" << endl;

        if (child->visits > maxVisits)
        {
            bestChild = child;
            maxVisits = child->visits;
        }
    }

    GameState bestMove = bestChild ? bestChild->state : rootState;

    // Cleanup
    delete root;

    return bestMove;
}

int main()
{
    srand(static_cast<unsigned>(time(0))); // Seed random number generator
    GameState game;
    int simulations = 10000; // Number of MCTS simulations per move
    int player = 1;          // Human plays as X (player 1)

    cout << "=== Tic-Tac-Toe with Monte Carlo Tree Search ===" << endl;
    cout << "You are X, AI is O" << endl;

    while (!game.isTerminal())
    {
        game.printBoard();

        if (game.currentPlayer == player)
        {
            // Human's turn
            int row, col;
            bool validMove = false;

            while (!validMove)
            {
                cout << "Enter your move (row col): ";
                cin >> row >> col;

                validMove = game.makeMove(row, col);
                if (!validMove)
                    cout << "Invalid move. Try again." << endl;
            }
        }
        else
        {
            // AI's turn
            cout << "AI is thinking..." << endl;
            game = monteCarloTreeSearch(game, simulations);
        }

        // Check game result
        if (game.isTerminal())
        {
            game.printBoard();
            int result = game.getReward();
            if (result == 0)
                cout << "Game ended in a draw!" << endl;
            else if ((result > 0 && player == 1) || (result < 0 && player == 2))
                cout << "You win!" << endl;
            else
                cout << "AI wins!" << endl;
        }
    }

    return 0;
}
