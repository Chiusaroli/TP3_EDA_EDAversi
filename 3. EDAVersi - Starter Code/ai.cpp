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

// Matriz de pesos posicionales CORREGIDA (estrategia óptima de Reversi)
static const int POSITION_WEIGHTS[BOARD_SIZE][BOARD_SIZE] = {
    {120, -20,  20,   5,   5,  20, -20, 120},
    {-20, -40,  -5,  -5,  -5,  -5, -40, -20},
    { 20,  -5,  15,   3,   3,  15,  -5,  20},
    {  5,  -5,   3,   3,   3,   3,  -5,   5},
    {  5,  -5,   3,   3,   3,   3,  -5,   5},
    { 20,  -5,  15,   3,   3,  15,  -5,  20},
    {-20, -40,  -5,  -5,  -5,  -5, -40, -20},
    {120, -20,  20,   5,   5,  20, -20, 120}
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
 * @brief Cuenta movimientos válidos de forma eficiente
 */
int countMoves(GameModel& model, Player player)
{
    Piece currentPiece = (player == PLAYER_WHITE) ? PIECE_WHITE : PIECE_BLACK;
    Piece opponentPiece = (player == PLAYER_WHITE) ? PIECE_BLACK : PIECE_WHITE;

    int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };

    int moveCount = 0;

    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            if (model.board[y][x] != PIECE_EMPTY)
                continue;

            for (int d = 0; d < 8; d++)
            {
                int dx = directions[d][0];
                int dy = directions[d][1];
                int nx = x + dx;
                int ny = y + dy;
                bool foundOpponent = false;

                while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE)
                {
                    Piece piece = model.board[ny][nx];

                    if (piece == PIECE_EMPTY)
                        break;

                    if (piece == opponentPiece)
                    {
                        foundOpponent = true;
                        nx += dx;
                        ny += dy;
                        continue;
                    }

                    if (piece == currentPiece && foundOpponent)
                    {
                        moveCount++;
                        goto nextSquare;  // Este movimiento es válido, pasar al siguiente
                    }
                    break;
                }
            }
        nextSquare:;
        }
    }

    return moveCount;
}

/**
 * @brief Función de evaluación MEJORADA
 */
int evaluate(GameModel& model, Player player)
{
    Player opponent = (player == PLAYER_WHITE) ? PLAYER_BLACK : PLAYER_WHITE;
    Piece playerPiece = (player == PLAYER_WHITE) ? PIECE_WHITE : PIECE_BLACK;
    Piece opponentPiece = (player == PLAYER_WHITE) ? PIECE_BLACK : PIECE_WHITE;

    int totalPieces = 0;
    int positionalValue = 0;
    int playerCount = 0;
    int opponentCount = 0;
    int cornerBonus = 0;
    int edgeBonus = 0;

    // Recorrido único del tablero
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
            int weight = POSITION_WEIGHTS[y][x];
            if (piece == playerPiece)
                positionalValue += weight;
            else
                positionalValue -= weight;

            // BONUS ESPECIAL POR ESQUINAS
            bool isCorner = ((x == 0 || x == 7) && (y == 0 || y == 7));
            if (isCorner)
            {
                if (piece == playerPiece)
                    cornerBonus += 50;
                else
                    cornerBonus -= 50;
            }

            // Bonus por bordes estables (si hay esquina del mismo color)
            bool isEdge = (x == 0 || x == 7 || y == 0 || y == 7);
            if (isEdge && !isCorner)
            {
                // Verificar si hay una esquina ocupada en esta fila/columna
                bool hasCornerSupport = false;

                if (x == 0 || x == 7)
                {
                    if ((model.board[0][x] == piece) || (model.board[7][x] == piece))
                        hasCornerSupport = true;
                }
                if (y == 0 || y == 7)
                {
                    if ((model.board[y][0] == piece) || (model.board[y][7] == piece))
                        hasCornerSupport = true;
                }

                if (hasCornerSupport)
                {
                    if (piece == playerPiece)
                        edgeBonus += 5;
                    else
                        edgeBonus -= 5;
                }
            }
        }
    }

    // === MOVILIDAD (crucial en mid-game) ===
    int mobilityValue = 0;
    if (totalPieces < 50)
    {
        int playerMobility = countMoves(model, player);
        int opponentMobility = countMoves(model, opponent);

        mobilityValue = (playerMobility - opponentMobility) * 5;

        // Penalización severa si el oponente no tiene movimientos
        if (opponentMobility == 0 && playerMobility > 0)
            mobilityValue += 100;

        // Bonus si tenemos el doble de movilidad
        if (playerMobility > opponentMobility * 2)
            mobilityValue += 30;
    }

    // === CONTEO DE FICHAS (estrategia por fase) ===
    int scoreDiff = playerCount - opponentCount;
    int pieceValue = 0;

    if (totalPieces >= 52)  // Endgame final (últimas 12 fichas)
    {
        // En endgame, cada ficha cuenta MUCHO
        pieceValue = scoreDiff * 15;
    }
    else if (totalPieces >= 45)  // Late game
    {
        pieceValue = scoreDiff * 8;
    }
    else if (totalPieces >= 35)  // Mid-late game
    {
        pieceValue = scoreDiff * 3;
    }
    else if (totalPieces >= 20)  // Mid game
    {
        // En mid game, tener MENOS fichas puede ser mejor
        pieceValue = scoreDiff * -1;
    }
    else  // Early game
    {
        // En early game, minimizar fichas es buena estrategia
        pieceValue = scoreDiff * -3;
    }

    // === PARIDAD (solo en late game) ===
    int parityValue = 0;
    if (totalPieces >= 50)
    {
        int emptySquares = 64 - totalPieces;
        // Si quedan pocas casillas y hay paridad favorable
        if (emptySquares % 2 == 1)
        {
            // Queremos tener el último movimiento
            parityValue = (model.currentPlayer == player) ? 15 : -15;
        }
    }

    // === FRONTERA (fichas expuestas - queremos MENOS en early/mid game) ===
    int frontierValue = 0;
    if (totalPieces < 45)
    {
        int playerFrontier = 0;
        int opponentFrontier = 0;

        for (int y = 0; y < BOARD_SIZE; y++)
        {
            for (int x = 0; x < BOARD_SIZE; x++)
            {
                Piece piece = model.board[y][x];
                if (piece == PIECE_EMPTY)
                    continue;

                // Verificar si tiene al menos una casilla vacía adyacente
                bool hasFreeNeighbor = false;
                for (int dy = -1; dy <= 1; dy++)
                {
                    for (int dx = -1; dx <= 1; dx++)
                    {
                        if (dx == 0 && dy == 0)
                            continue;
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE)
                        {
                            if (model.board[ny][nx] == PIECE_EMPTY)
                            {
                                hasFreeNeighbor = true;
                                break;
                            }
                        }
                    }
                    if (hasFreeNeighbor)
                        break;
                }

                if (hasFreeNeighbor)
                {
                    if (piece == playerPiece)
                        playerFrontier++;
                    else
                        opponentFrontier++;
                }
            }
        }

        // Penalizar tener muchas fichas en la frontera
        frontierValue = (opponentFrontier - playerFrontier) * 2;
    }

    return positionalValue + mobilityValue + pieceValue +
        parityValue + cornerBonus + edgeBonus + frontierValue;
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
 * @brief Ordena movimientos con heurística mejorada
 */
void orderMoves(GameModel& model, Moves& moves, bool maximizing)
{
    if (moves.size() <= 1)
        return;

    std::vector<ScoredMove> scoredMoves;

    for (auto move : moves)
    {
        ScoredMove sm;
        sm.move = move;
        sm.score = POSITION_WEIGHTS[move.y][move.x];

        // Bonus extra por esquinas
        bool isCorner = ((move.x == 0 || move.x == 7) && (move.y == 0 || move.y == 7));
        if (isCorner)
            sm.score += 200;

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
    orderMoves(model, validMoves, maximizingPlayer);

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
    for (int depth = 1; depth <= 20; depth++)
    {
        // Detenerse al 90% del límite para evitar cortes abruptos
        if (nodesExplored >= MAX_NODES * 0.9)
            break;

        int alpha = INT_MIN;
        int beta = INT_MAX;
        int bestValue = INT_MIN;

        // Ordenar movimientos en el nodo raíz
        orderMoves(model, validMoves, true);

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
    }

    return bestMove;
}