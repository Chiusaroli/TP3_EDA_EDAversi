/**
 * @brief Implements the Reversi game AI with Alpha-Beta Pruning
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2023-2024
 */

#include <cstdlib>
#include <climits>
#include <algorithm>

#include "ai.h"
#include "controller.h"

 // Profundidad adaptativa según fase del juego
#define EARLY_GAME_DEPTH 7
#define MID_GAME_DEPTH 8
#define END_GAME_DEPTH 12

// Límite de nodos para casos extremos
#define MAX_NODES 500000

// Contador global de nodos explorados
static int nodesExplored = 0;

// Matriz de pesos posicionales (estrategia de Reversi)
// Las esquinas valen mucho, las casillas X (adyacentes a esquinas) son peligrosas
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
 * @brief Determina la profundidad de búsqueda según la fase del juego
 */
int getSearchDepth(GameModel& model)
{
    int totalPieces = 0;
    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
            if (model.board[y][x] != PIECE_EMPTY)
                totalPieces++;

    // Juego inicial (4-20 fichas): búsqueda moderada
    if (totalPieces <= 20)
        return EARLY_GAME_DEPTH;

    // Final del juego (45+ fichas): búsqueda exhaustiva
    if (totalPieces >= 45)
        return END_GAME_DEPTH;

    // Medio juego: búsqueda profunda
    return MID_GAME_DEPTH;
}

/**
 * @brief Función de evaluación avanzada para Reversi
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

    // === 1. PESOS POSICIONALES ===
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

    // === 2. MOVILIDAD (muy importante en medio juego) ===
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
    if (totalPieces < 50) // Movilidad importante hasta el final
    {
        mobilityValue = ((int)playerMoves.size() - (int)opponentMoves.size()) * 3;

        // Penalizar severamente si el oponente no tiene movimientos (muy bueno)
        if (opponentMoves.size() == 0 && playerMoves.size() > 0)
            mobilityValue += 50;
        // Bonus si tenemos muchos movimientos
        if (playerMoves.size() > opponentMoves.size() * 2)
            mobilityValue += 20;
    }

    // === 3. ESTABILIDAD DE FICHAS ===
    // Fichas en bordes son más estables
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

    // === 4. PARIDAD (en end-game) ===
    int parityValue = 0;
    if (totalPieces >= 50) // Solo importante al final
    {
        int emptySquares = 64 - totalPieces;
        // Queremos hacer el último movimiento
        if (emptySquares % 2 == 1)
            parityValue = (model.currentPlayer == player) ? 10 : -10;
    }

    // === 5. CONTEO DE FICHAS (más importante al final) ===
    int scoreDiff = getScore(model, player) - getScore(model, opponent);
    int pieceValue = 0;

    if (totalPieces >= 50) // End-game: las fichas importan mucho
        pieceValue = scoreDiff * 5;
    else if (totalPieces >= 40) // Late mid-game
        pieceValue = scoreDiff * 2;
    else // Early-mid game: las fichas importan poco
        pieceValue = scoreDiff / 2;

    // Combinar todas las heurísticas
    return positionalValue + mobilityValue + stabilityValue + parityValue + pieceValue;
}

/**
 * @brief Copia el estado del tablero
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
 * @brief Simula un movimiento sin modificar el modelo original
 */
void simulateMove(GameModel& model, Square move, GameModel& newModel)
{
    copyBoard(model, newModel);
    playMove(newModel, move);
}

/**
 * @brief Estructura para ordenar movimientos
 */
struct ScoredMove
{
    Square move;
    int score;

    bool operator<(const ScoredMove& other) const
    {
        return score > other.score; // Orden descendente
    }
};

/**
 * @brief Ordena movimientos por su valor heurístico (mejora poda alfa-beta)
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
 * @brief Implementa el algoritmo Minimax con poda Alfa-Beta mejorado
 */
int alphabeta(GameModel& model, int depth, int alpha, int beta,
    bool maximizingPlayer, Player aiPlayer)
{
    nodesExplored++;

    // Poda por cantidad de nodos (emergencia)
    if (nodesExplored >= MAX_NODES)
        return evaluate(model, aiPlayer);

    // Caso base
    if (depth == 0 || model.gameOver)
        return evaluate(model, aiPlayer);

    // Obtener movimientos válidos
    Moves validMoves;
    getValidMoves(model, validMoves);

    // Si no hay movimientos válidos, pasar turno
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

    // ORDENAR MOVIMIENTOS para mejorar poda (movimientos prometedores primero)
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
                break; // Poda Beta
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
                break; // Poda Alfa
        }

        return minEval;
    }
}

Square getBestMove(GameModel& model)
{
    Moves validMoves;
    getValidMoves(model, validMoves);

    if (validMoves.size() == 0)
        return GAME_INVALID_SQUARE;

    if (validMoves.size() == 1)
        return validMoves[0];

    nodesExplored = 0;

    // Determinar profundidad según fase del juego
    int searchDepth = getSearchDepth(model);

    Square bestMove = validMoves[0];
    int bestValue = INT_MIN;
    Player aiPlayer = model.currentPlayer;

    int alpha = INT_MIN;
    int beta = INT_MAX;

    // Ordenar movimientos en el nodo raíz
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