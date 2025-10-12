/**
 * @brief Implements the Reversi game AI with Alpha-Beta Pruning
 * @author Marc S. Ressl, Francisco Chiusarolli, Tomas Agustin Garcilazo, Juan Luis Brusasca, Luca Mateo Forchiassin
 *
 * @copyright Copyright (c) 2023-2024
 */

#include <cstdlib>
#include <climits>
#include <algorithm>

#include "ai.h"
#include "controller.h"

 // Adaptive depth based on game phase
#define EARLY_GAME_DEPTH 7
#define MID_GAME_DEPTH 8
#define END_GAME_DEPTH 12

// Node limit for extreme cases
#define MAX_NODES 500000

// Global counter for explored nodes
static int nodesExplored = 0;

// Positional weight matrix (Reversi strategy)
// Corners are very valuable, X-squares (adjacent to corners) are dangerous
static const int POSITION_WEIGHTS[BOARD_SIZE][BOARD_SIZE] = {
    {100, -20,  10,   5,   5,  10, -20, 100},
    {-20, -50,  -2,  -2,  -2,  -2, -50, -20},
    { 10,  -2,   5,   1,   1,   5,  -2,  10},
    {  5,  -2,   1,   0,   0,   1,  -2,   5},
    {  5,  -2,   1,   0,   0,   1,  -2,   5},
    { 10,  -2,   5,   1,   1,   5,  -2,  10},
    {-20, -50,  -2,  -2,  -2,  -2, -50, -20},
    {100, -20,  10,   5,   5,  10, -20, 100}
};

/**
 * @brief Determines search depth based on game phase
 */
int getSearchDepth(GameModel& model)
{
    int totalPieces = 0;
    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
            if (model.board[y][x] != PIECE_EMPTY)
                totalPieces++;

    // Early game (4-20 pieces): moderate search
    if (totalPieces <= 20)
        return EARLY_GAME_DEPTH;

    // End game (45+ pieces): exhaustive search
    if (totalPieces >= 45)
        return END_GAME_DEPTH;

    // Mid game: deep search
    return MID_GAME_DEPTH;
}

/**
 * @brief Advanced evaluation function for Reversi
 */
int evaluate(GameModel& model, Player player)
{
    Player opponent = (player == PLAYER_WHITE) ? PLAYER_BLACK : PLAYER_WHITE;
    Piece playerPiece = (player == PLAYER_WHITE) ? PIECE_WHITE : PIECE_BLACK;
    Piece opponentPiece = (player == PLAYER_WHITE) ? PIECE_BLACK : PIECE_WHITE;

    int totalPieces = 0;
    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
            if (model.board[y][x] != PIECE_EMPTY)
                totalPieces++;

    // === 1. POSITIONAL WEIGHTS ===
    int positionalValue = 0;
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            if (model.board[y][x] == playerPiece)
                positionalValue += POSITION_WEIGHTS[y][x];
            else if (model.board[y][x] == opponentPiece)
                positionalValue -= POSITION_WEIGHTS[y][x];
        }
    }

    // === 2. MOBILITY (very important in mid game) ===
    GameModel tempModel;
    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
            tempModel.board[y][x] = model.board[y][x];
    tempModel.gameOver = false;

    tempModel.currentPlayer = player;
    Moves playerMoves;
    getValidMoves(tempModel, playerMoves);

    tempModel.currentPlayer = opponent;
    Moves opponentMoves;
    getValidMoves(tempModel, opponentMoves);

    int mobilityValue = 0;
    if (totalPieces < 50) // Mobility important until endgame
    {
        mobilityValue = ((int)playerMoves.size() - (int)opponentMoves.size()) * 3;

        // Severely penalize if opponent has no moves (very good)
        if (opponentMoves.size() == 0 && playerMoves.size() > 0)
            mobilityValue += 50;
        // Bonus if we have many moves
        if (playerMoves.size() > opponentMoves.size() * 2)
            mobilityValue += 20;
    }

    // === 3. PIECE STABILITY ===
    // Pieces on edges are more stable
    int stabilityValue = 0;
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            bool isEdge = (x == 0 || x == BOARD_SIZE - 1 || y == 0 || y == BOARD_SIZE - 1);
            if (isEdge)
            {
                if (model.board[y][x] == playerPiece)
                    stabilityValue += 5;
                else if (model.board[y][x] == opponentPiece)
                    stabilityValue -= 5;
            }
        }
    }

    // === 4. PARITY (in endgame) ===
    int parityValue = 0;
    if (totalPieces >= 50) // Only important at the end
    {
        int emptySquares = 64 - totalPieces;
        // We want to make the last move
        if (emptySquares % 2 == 1)
            parityValue = (model.currentPlayer == player) ? 10 : -10;
    }

    // === 5. PIECE COUNT (more important at the end) ===
    int scoreDiff = getScore(model, player) - getScore(model, opponent);
    int pieceValue = 0;

    if (totalPieces >= 50) // Endgame: pieces matter a lot
        pieceValue = scoreDiff * 5;
    else if (totalPieces >= 40) // Late mid-game
        pieceValue = scoreDiff * 2;
    else // Early-mid game: pieces matter little
        pieceValue = scoreDiff / 2;

    // Combine all heuristics
    return positionalValue + mobilityValue + stabilityValue + parityValue + pieceValue;
}

/**
 * @brief Copies the board state
 */
void copyBoard(GameModel& source, GameModel& dest)
{
    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
            dest.board[y][x] = source.board[y][x];

    dest.currentPlayer = source.currentPlayer;
    dest.gameOver = source.gameOver;
}

/**
 * @brief Simulates a move without modifying the original model
 */
void simulateMove(GameModel& model, Square move, GameModel& newModel)
{
    copyBoard(model, newModel);
    playMove(newModel, move);
}

/**
 * @brief Structure for ordering moves
 */
struct ScoredMove
{
    Square move;
    int score;

    bool operator<(const ScoredMove& other) const
    {
        return score > other.score; // Descending order
    }
};

/**
 * @brief Orders moves by their heuristic value (improves alpha-beta pruning)
 */
void orderMoves(GameModel& model, Moves& moves, Player aiPlayer, bool maximizing)
{
    std::vector<ScoredMove> scoredMoves;

    for (auto move : moves)
    {
        GameModel newModel;
        simulateMove(model, move, newModel);

        ScoredMove sm;
        sm.move = move;
        sm.score = evaluate(newModel, aiPlayer);

        if (!maximizing)
            sm.score = -sm.score;

        scoredMoves.push_back(sm);
    }

    std::sort(scoredMoves.begin(), scoredMoves.end());

    moves.clear();
    for (auto sm : scoredMoves)
        moves.push_back(sm.move);
}

/**
 * @brief Implements the Minimax algorithm with improved Alpha-Beta pruning
 */
int alphabeta(GameModel& model, int depth, int alpha, int beta,
    bool maximizingPlayer, Player aiPlayer)
{
    nodesExplored++;

    // Prune by node count (emergency)
    if (nodesExplored >= MAX_NODES)
        return evaluate(model, aiPlayer);

    // Base case
    if (depth == 0 || model.gameOver)
        return evaluate(model, aiPlayer);

    // Get valid moves
    Moves validMoves;
    getValidMoves(model, validMoves);

    // If no valid moves, pass turn
    if (validMoves.size() == 0)
    {
        GameModel newModel;
        copyBoard(model, newModel);
        newModel.currentPlayer = (newModel.currentPlayer == PLAYER_WHITE)
            ? PLAYER_BLACK : PLAYER_WHITE;

        Moves opponentMoves;
        getValidMoves(newModel, opponentMoves);
        if (opponentMoves.size() == 0)
        {
            newModel.gameOver = true;
            return evaluate(newModel, aiPlayer);
        }

        return alphabeta(newModel, depth - 1, alpha, beta, !maximizingPlayer, aiPlayer);
    }

    // ORDER MOVES to improve pruning (promising moves first)
    if (validMoves.size() > 1)
        orderMoves(model, validMoves, aiPlayer, maximizingPlayer);

    if (maximizingPlayer)
    {
        int maxEval = INT_MIN;

        for (auto move : validMoves)
        {
            GameModel newModel;
            simulateMove(model, move, newModel);

            int eval = alphabeta(newModel, depth - 1, alpha, beta, false, aiPlayer);
            maxEval = (eval > maxEval) ? eval : maxEval;

            alpha = (eval > alpha) ? eval : alpha;
            if (beta <= alpha)
                break; // Beta cutoff
        }

        return maxEval;
    }
    else
    {
        int minEval = INT_MAX;

        for (auto move : validMoves)
        {
            GameModel newModel;
            simulateMove(model, move, newModel);

            int eval = alphabeta(newModel, depth - 1, alpha, beta, true, aiPlayer);
            minEval = (eval < minEval) ? eval : minEval;

            beta = (eval < beta) ? eval : beta;
            if (beta <= alpha)
                break; // Alpha cutoff
        }

        return minEval;
    }
}

/**
 * @brief Returns the best move for the current position
 */
Square getBestMove(GameModel& model)
{
    Moves validMoves;
    getValidMoves(model, validMoves);

    if (validMoves.size() == 0)
        return GAME_INVALID_SQUARE;

    if (validMoves.size() == 1)
        return validMoves[0];

    nodesExplored = 0;

    // Determine depth based on game phase
    int searchDepth = getSearchDepth(model);

    Square bestMove = validMoves[0];
    int bestValue = INT_MIN;
    Player aiPlayer = model.currentPlayer;

    int alpha = INT_MIN;
    int beta = INT_MAX;

    // Order moves at root node
    orderMoves(model, validMoves, aiPlayer, true);

    for (auto move : validMoves)
    {
        GameModel newModel;
        simulateMove(model, move, newModel);

        int moveValue = alphabeta(newModel, searchDepth - 1, alpha, beta, false, aiPlayer);

        if (moveValue > bestValue)
        {
            bestValue = moveValue;
            bestMove = move;
        }

        alpha = (moveValue > alpha) ? moveValue : alpha;
    }

    return bestMove;
}