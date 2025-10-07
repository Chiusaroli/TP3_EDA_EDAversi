/**
 * @brief Implements the Reversi game AI with Alpha-Beta Pruning
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2023-2024
 */

#include <cstdlib>
#include <climits>
#include <algorithm>
#include <cstring>
#include <unordered_map>

#include "ai.h"
#include "controller.h"

 // Límite de nodos configurables
#define MAX_NODES 100000

// Contador global de nodos explorados
static int nodesExplored = 0;

// Matriz de pesos posicionales (estrategia de Reversi)
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

// Tabla de transposiciones
struct TranspositionEntry {
    int value;
    int depth;
};

static std::unordered_map<uint64_t, TranspositionEntry> ttable;

/**
 * @brief Genera hash del tablero para transposiciones
 */
uint64_t hashBoard(GameModel& model)
{
    uint64_t hash = 0;
    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
            hash = hash * 3 + model.board[y][x];
    return hash ^ ((uint64_t)model.currentPlayer << 60);
}

/**
 * @brief Función de evaluación optimizada (un solo recorrido del tablero)
 */
int evaluate(GameModel& model, Player player)
{
    Player opponent = (player == PLAYER_WHITE) ? PLAYER_BLACK : PLAYER_WHITE;
    Piece playerPiece = (player == PLAYER_WHITE) ? PIECE_WHITE : PIECE_BLACK;
    Piece opponentPiece = (player == PLAYER_WHITE) ? PIECE_BLACK : PIECE_WHITE;

    int totalPieces = 0;
    int positionalValue = 0;
    int stabilityValue = 0;
    int playerCount = 0;
    int opponentCount = 0;

    // UN SOLO RECORRIDO para todas las heurísticas basadas en posición
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            Piece piece = model.board[y][x];

            if (piece == PIECE_EMPTY)
                continue;

            totalPieces++;

            // Contar fichas
            if (piece == playerPiece)
                playerCount++;
            else
                opponentCount++;

            // Pesos posicionales
            if (piece == playerPiece)
                positionalValue += POSITION_WEIGHTS[y][x];
            else
                positionalValue -= POSITION_WEIGHTS[y][x];

            // Estabilidad de bordes
            bool isEdge = (x == 0 || x == BOARD_SIZE - 1 || y == 0 || y == BOARD_SIZE - 1);
            if (isEdge)
            {
                if (piece == playerPiece)
                    stabilityValue += 5;
                else
                    stabilityValue -= 5;
            }
        }
    }

    // === MOVILIDAD (solo en mid-game) ===
    int mobilityValue = 0;
    if (totalPieces < 50)
    {
        GameModel tempModel;
        memcpy(tempModel.board, model.board, sizeof(model.board));
        tempModel.gameOver = false;

        tempModel.currentPlayer = player;
        Moves playerMoves;
        getValidMoves(tempModel, playerMoves);

        tempModel.currentPlayer = opponent;
        Moves opponentMoves;
        getValidMoves(tempModel, opponentMoves);

        mobilityValue = ((int)playerMoves.size() - (int)opponentMoves.size()) * 3;

        if (opponentMoves.size() == 0 && playerMoves.size() > 0)
            mobilityValue += 50;
        if (playerMoves.size() > opponentMoves.size() * 2)
            mobilityValue += 20;
    }

    // === PARIDAD (end-game) ===
    int parityValue = 0;
    if (totalPieces >= 50)
    {
        int emptySquares = 64 - totalPieces;
        if (emptySquares % 2 == 1)
            parityValue = (model.currentPlayer == player) ? 10 : -10;
    }

    // === CONTEO DE FICHAS (peso según fase) ===
    int scoreDiff = playerCount - opponentCount;
    int pieceValue = 0;

    if (totalPieces >= 50)
        pieceValue = scoreDiff * 5;
    else if (totalPieces >= 40)
        pieceValue = scoreDiff * 2;
    else
        pieceValue = scoreDiff / 2;

    return positionalValue + mobilityValue + stabilityValue + parityValue + pieceValue;
}

/**
 * @brief Copia el tablero de forma optimizada
 */
void copyBoard(GameModel& source, GameModel& dest)
{
    memcpy(dest.board, source.board, sizeof(source.board));
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
        return score > other.score;
    }
};

/**
 * @brief Ordena movimientos usando solo heurística barata (pesos posicionales)
 */
void orderMoves(Moves& moves, bool maximizing)
{
    if (moves.size() <= 1)
        return;

    std::vector<ScoredMove> scoredMoves;

    for (auto move : moves)
    {
        ScoredMove sm;
        sm.move = move;
        sm.score = POSITION_WEIGHTS[move.y][move.x];

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
 * @brief Implementa el algoritmo Minimax con poda Alfa-Beta
 */
int alphabeta(GameModel& model, int depth, int alpha, int beta,
    bool maximizingPlayer, Player aiPlayer)
{
    nodesExplored++;

    // Poda por cantidad de nodos
    if (nodesExplored >= MAX_NODES)
        return evaluate(model, aiPlayer);

    // Verificar tabla de transposiciones
    uint64_t hash = hashBoard(model);
    auto it = ttable.find(hash);
    if (it != ttable.end() && it->second.depth >= depth)
    {
        return it->second.value;
    }

    // Caso base
    if (depth == 0 || model.gameOver)
    {
        int value = evaluate(model, aiPlayer);

        // Guardar en tabla
        TranspositionEntry entry;
        entry.value = value;
        entry.depth = depth;
        ttable[hash] = entry;

        return value;
    }

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

    // Ordenar movimientos para mejorar poda
    orderMoves(validMoves, maximizingPlayer);

    int bestValue;

    if (maximizingPlayer)
    {
        bestValue = INT_MIN;

        for (auto move : validMoves)
        {
            GameModel newModel;
            simulateMove(model, move, newModel);

            int eval = alphabeta(newModel, depth - 1, alpha, beta, false, aiPlayer);
            bestValue = (eval > bestValue) ? eval : bestValue;

            alpha = (eval > alpha) ? eval : alpha;
            if (beta <= alpha)
                break; // Poda Beta
        }
    }
    else
    {
        bestValue = INT_MAX;

        for (auto move : validMoves)
        {
            GameModel newModel;
            simulateMove(model, move, newModel);

            int eval = alphabeta(newModel, depth - 1, alpha, beta, true, aiPlayer);
            bestValue = (eval < bestValue) ? eval : bestValue;

            beta = (eval < beta) ? eval : beta;
            if (beta <= alpha)
                break; // Poda Alfa
        }
    }

    // Guardar en tabla de transposiciones
    TranspositionEntry entry;
    entry.value = bestValue;
    entry.depth = depth;
    ttable[hash] = entry;

    return bestValue;
}

/**
 * @brief Retorna el mejor movimiento usando búsqueda iterativa en profundidad
 */
Square getBestMove(GameModel& model)
{
    Moves validMoves;
    getValidMoves(model, validMoves);

    if (validMoves.size() == 0)
        return GAME_INVALID_SQUARE;

    if (validMoves.size() == 1)
        return validMoves[0];

    // Limpiar tabla si está muy grande
    if (ttable.size() > 50000)
        ttable.clear();

    nodesExplored = 0;
    Square bestMove = validMoves[0];
    Player aiPlayer = model.currentPlayer;

    // BÚSQUEDA ITERATIVA EN PROFUNDIDAD
    // Explora profundidad 1, 2, 3... hasta alcanzar el límite de nodos
    for (int depth = 1; depth <= 20; depth++)
    {
        // Detenerse al 90% del límite para evitar cortes abruptos
        if (nodesExplored >= MAX_NODES * 0.9)
            break;

        int alpha = INT_MIN;
        int beta = INT_MAX;
        int bestValue = INT_MIN;

        // Ordenar movimientos en el nodo raíz
        orderMoves(validMoves, true);

        for (auto move : validMoves)
        {
            GameModel newModel;
            simulateMove(model, move, newModel);

            int value = alphabeta(newModel, depth - 1, alpha, beta, false, aiPlayer);

            if (value > bestValue)
            {
                bestValue = value;
                bestMove = move;
            }

            alpha = (value > alpha) ? value : alpha;

            // Si alcanzamos el límite, salir
            if (nodesExplored >= MAX_NODES * 0.9)
                break;
        }

        // Si completamos esta profundidad sin problemas, continuar
        // Si nos quedamos sin nodos, ya tenemos el mejor movimiento hasta ahora
    }

    return bestMove;
}