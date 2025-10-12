/**
 * @file ai.cpp
 * @brief Implements the Reversi game AI with Alpha-Beta Pruning
 * @author Marc S. Ressl, Francisco Chiusarolli, Tomas Agustin Garcilazo,
 *         Juan Luis Brusasca, Luca Mateo Forchiassin
 * @date 2023-2024
 *
 * This file implements an AI player for Reversi using the minimax algorithm
 * with alpha-beta pruning, adaptive depth search, and move ordering for
 * optimization. The evaluation function considers positional weights, mobility,
 * stability, parity, and piece count based on game phase.
 */

#include <cstdlib>
#include <climits>
#include <algorithm>

#include "ai.h"
#include "controller.h"

const int EARLY_GAME_DEPTH = 7;
const int MID_GAME_DEPTH = 8;
const int END_GAME_DEPTH = 12;
const int MAX_NODES = 500000;

static int nodesExplored = 0;

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
 * @brief Determines optimal search depth based on current game phase
 *
 * Adapts search depth dynamically: shallow search in early game to save time,
 * moderate depth in mid-game for balanced play, and deep search in endgame
 * for precise calculation.
 *
 * @param model Current game state
 * @return int Search depth (7 for early game, 8 for mid-game, 12 for endgame)
 */
int getSearchDepth(GameModel& model) {
    int totalPieces = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (model.board[y][x] != PIECE_EMPTY)
                totalPieces++;
        }
    }

    if (totalPieces <= 20)
        return EARLY_GAME_DEPTH;

    if (totalPieces >= 45)
        return END_GAME_DEPTH;

    return MID_GAME_DEPTH;
}

/**
 * @brief Evaluates the current board position using multiple heuristics
 *
 * Combines five evaluation factors weighted by game phase:
 * 1. Positional weights (corners valuable, X-squares dangerous)
 * 2. Mobility (number of available moves)
 * 3. Stability (pieces on edges harder to flip)
 * 4. Parity (advantage of making last move)
 * 5. Piece count (increasingly important toward endgame)
 *
 * @param model Current game state
 * @param player Player to evaluate for
 * @return int Evaluation score (positive favors player, negative favors opponent)
 */
int evaluate(GameModel& model, Player player) {
    Player opponent = (player == PLAYER_WHITE) ? PLAYER_BLACK : PLAYER_WHITE;
    Piece playerPiece = (player == PLAYER_WHITE) ? PIECE_WHITE : PIECE_BLACK;
    Piece opponentPiece = (player == PLAYER_WHITE) ? PIECE_BLACK : PIECE_WHITE;

    int totalPieces = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (model.board[y][x] != PIECE_EMPTY)
                totalPieces++;
        }
    }

    int positionalValue = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (model.board[y][x] == playerPiece)
                positionalValue += POSITION_WEIGHTS[y][x];
            else if (model.board[y][x] == opponentPiece)
                positionalValue -= POSITION_WEIGHTS[y][x];
        }
    }

    GameModel tempModel;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            tempModel.board[y][x] = model.board[y][x];
        }
    }
    tempModel.gameOver = false;

    tempModel.currentPlayer = player;
    Moves playerMoves;
    getValidMoves(tempModel, playerMoves);

    tempModel.currentPlayer = opponent;
    Moves opponentMoves;
    getValidMoves(tempModel, opponentMoves);

    int mobilityValue = 0;
    if (totalPieces < 50) {
        mobilityValue = ((int)playerMoves.size() - (int)opponentMoves.size()) * 3;

        if (opponentMoves.size() == 0 && playerMoves.size() > 0)
            mobilityValue += 50;

        if (playerMoves.size() > opponentMoves.size() * 2)
            mobilityValue += 20;
    }

    int stabilityValue = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            bool isEdge = (x == 0 || x == BOARD_SIZE - 1 ||
                y == 0 || y == BOARD_SIZE - 1);
            if (isEdge) {
                if (model.board[y][x] == playerPiece)
                    stabilityValue += 5;
                else if (model.board[y][x] == opponentPiece)
                    stabilityValue -= 5;
            }
        }
    }

    int parityValue = 0;
    if (totalPieces >= 50) {
        int emptySquares = 64 - totalPieces;
        if (emptySquares % 2 == 1)
            parityValue = (model.currentPlayer == player) ? 10 : -10;
    }

    int scoreDiff = getScore(model, player) - getScore(model, opponent);
    int pieceValue = 0;

    if (totalPieces >= 50)
        pieceValue = scoreDiff * 5;
    else if (totalPieces >= 40)
        pieceValue = scoreDiff * 2;
    else
        pieceValue = scoreDiff / 2;

    return positionalValue + mobilityValue + stabilityValue +
        parityValue + pieceValue;
}

/**
 * @brief Copies game state from source to destination
 *
 * @param source Source game model
 * @param dest Destination game model (modified)
 */
void copyBoard(GameModel& source, GameModel& dest) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            dest.board[y][x] = source.board[y][x];
        }
    }

    dest.currentPlayer = source.currentPlayer;
    dest.gameOver = source.gameOver;
}

/**
 * @brief Simulates a move without modifying the original model
 *
 * @param model Current game state
 * @param move Move to simulate
 * @param newModel Output parameter containing the resulting game state
 */
void simulateMove(GameModel& model, Square move, GameModel& newModel) {
    copyBoard(model, newModel);
    playMove(newModel, move);
}

struct ScoredMove {
    Square move;
    int score;

    bool operator<(const ScoredMove& other) const {
        return score > other.score;
    }
};

/**
 * @brief Orders moves by evaluation score to improve pruning efficiency
 *
 * Evaluates each move and sorts them in descending order for maximizing player
 * or ascending order for minimizing player. This move ordering significantly
 * improves alpha-beta pruning cutoffs.
 *
 * @param model Current game state
 * @param moves List of valid moves (modified in place)
 * @param aiPlayer The AI player
 * @param maximizing True if maximizing player, false if minimizing
 */
void orderMoves(GameModel& model, Moves& moves, Player aiPlayer,
    bool maximizing) {
    std::vector<ScoredMove> scoredMoves;

    for (auto move : moves) {
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
    for (auto sm : scoredMoves) {
        moves.push_back(sm.move);
    }
}

/**
 * @brief Implements minimax algorithm with alpha-beta pruning
 *
 * Recursively searches the game tree using minimax with alpha-beta pruning
 * to find the best move. Includes move ordering for better pruning efficiency
 * and node limit to prevent excessive computation.
 *
 * @param model Current game state
 * @param depth Remaining search depth
 * @param alpha Alpha value for pruning
 * @param beta Beta value for pruning
 * @param maximizingPlayer True if maximizing, false if minimizing
 * @param aiPlayer The AI player to evaluate for
 * @return int Best evaluation score found at this depth
 */
int alphabeta(GameModel& model, int depth, int alpha, int beta,
    bool maximizingPlayer, Player aiPlayer) {
    nodesExplored++;

    if (nodesExplored >= MAX_NODES)
        return evaluate(model, aiPlayer);

    if (depth == 0 || model.gameOver)
        return evaluate(model, aiPlayer);

    Moves validMoves;
    getValidMoves(model, validMoves);

    if (validMoves.size() == 0) {
        GameModel newModel;
        copyBoard(model, newModel);
        newModel.currentPlayer = (newModel.currentPlayer == PLAYER_WHITE)
            ? PLAYER_BLACK : PLAYER_WHITE;

        Moves opponentMoves;
        getValidMoves(newModel, opponentMoves);
        if (opponentMoves.size() == 0) {
            newModel.gameOver = true;
            return evaluate(newModel, aiPlayer);
        }

        return alphabeta(newModel, depth - 1, alpha, beta,
            !maximizingPlayer, aiPlayer);
    }

    if (validMoves.size() > 1)
        orderMoves(model, validMoves, aiPlayer, maximizingPlayer);

    if (maximizingPlayer) {
        int maxEval = INT_MIN;

        for (auto move : validMoves) {
            GameModel newModel;
            simulateMove(model, move, newModel);

            int eval = alphabeta(newModel, depth - 1, alpha, beta, false, aiPlayer);
            maxEval = (eval > maxEval) ? eval : maxEval;

            alpha = (eval > alpha) ? eval : alpha;
            if (beta <= alpha)
                break;
        }

        return maxEval;
    }
    else {
        int minEval = INT_MAX;

        for (auto move : validMoves) {
            GameModel newModel;
            simulateMove(model, move, newModel);

            int eval = alphabeta(newModel, depth - 1, alpha, beta, true, aiPlayer);
            minEval = (eval < minEval) ? eval : minEval;

            beta = (eval < beta) ? eval : beta;
            if (beta <= alpha)
                break;
        }

        return minEval;
    }
}

/**
 * @brief Returns the best move for the current position
 *
 * Main entry point for AI move selection. Determines appropriate search depth
 * based on game phase, orders moves at root level, and uses alpha-beta search
 * to find the optimal move.
 *
 * @param model Current game state
 * @return Square Best move found, or GAME_INVALID_SQUARE if no moves available
 */
Square getBestMove(GameModel& model) {
    Moves validMoves;
    getValidMoves(model, validMoves);

    if (validMoves.size() == 0)
        return GAME_INVALID_SQUARE;

    if (validMoves.size() == 1)
        return validMoves[0];

    nodesExplored = 0;

    int searchDepth = getSearchDepth(model);

    Square bestMove = validMoves[0];
    int bestValue = INT_MIN;
    Player aiPlayer = model.currentPlayer;

    int alpha = INT_MIN;
    int beta = INT_MAX;

    orderMoves(model, validMoves, aiPlayer, true);

    for (auto move : validMoves) {
        GameModel newModel;
        simulateMove(model, move, newModel);

        int moveValue = alphabeta(newModel, searchDepth - 1, alpha, beta,
            false, aiPlayer);

        if (moveValue > bestValue) {
            bestValue = moveValue;
            bestMove = move;
        }

        alpha = (moveValue > alpha) ? moveValue : alpha;
    }

    return bestMove;
}