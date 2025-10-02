/**
 * @brief Implements the Reversi game AI with Alpha-Beta Pruning
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2023-2024
 */

#include <cstdlib>
#include <climits>

#include "ai.h"
#include "controller.h"

 // Profundidad máxima del árbol Minimax
#define MAX_DEPTH 6

// Límite de nodos explorados antes de detener la búsqueda
#define MAX_NODES 100000

// Contador global de nodos explorados
static int nodesExplored = 0;

/**
 * @brief Función de evaluación mejorada para Reversi
 *
 * Combina múltiples heurísticas:
 * - Diferencia de fichas (menos importante al inicio)
 * - Control de esquinas (muy importante)
 * - Movilidad (número de movimientos válidos)
 *
 * @param model El modelo del juego
 * @param player El jugador que queremos maximizar
 * @return Valor de evaluación (positivo es bueno para player)
 */
int evaluate(GameModel& model, Player player)
{
    Player opponent = (player == PLAYER_WHITE) ? PLAYER_BLACK : PLAYER_WHITE;

    // Peso para la diferencia de fichas
    int scoreDiff = getScore(model, player) - getScore(model, opponent);

    // Peso para las esquinas (muy valioso en Reversi)
    int cornerValue = 0;
    Square corners[4] = { {0, 0}, {0, BOARD_SIZE - 1}, {BOARD_SIZE - 1, 0}, {BOARD_SIZE - 1, BOARD_SIZE - 1} };

    Piece playerPiece = (player == PLAYER_WHITE) ? PIECE_WHITE : PIECE_BLACK;
    Piece opponentPiece = (player == PLAYER_WHITE) ? PIECE_BLACK : PIECE_WHITE;

    for (int i = 0; i < 4; i++)
    {
        Piece piece = getBoardPiece(model, corners[i]);
        if (piece == playerPiece)
            cornerValue += 25;
        else if (piece == opponentPiece)
            cornerValue -= 25;
    }

    // Movilidad: número de movimientos disponibles
    GameModel tempModel;
    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
            tempModel.board[y][x] = model.board[y][x];

    tempModel.currentPlayer = player;
    tempModel.gameOver = false;
    Moves playerMoves;
    getValidMoves(tempModel, playerMoves);

    tempModel.currentPlayer = opponent;
    Moves opponentMoves;
    getValidMoves(tempModel, opponentMoves);

    int mobilityValue = (int)playerMoves.size() - (int)opponentMoves.size();

    // Combinar las heurísticas
    return scoreDiff + cornerValue + (mobilityValue * 2);
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
 * @brief Implementa el algoritmo Minimax con poda Alfa-Beta
 *
 * @param model El modelo del juego actual
 * @param depth Profundidad actual en el árbol
 * @param alpha Mejor valor garantizado para MAX
 * @param beta Mejor valor garantizado para MIN
 * @param maximizingPlayer true si es turno de MAX, false si es MIN
 * @param aiPlayer El jugador que representa la IA
 * @return El valor minimax del nodo
 */
int alphabeta(GameModel& model, int depth, int alpha, int beta,
    bool maximizingPlayer, Player aiPlayer)
{
    nodesExplored++;

    // Poda por cantidad de nodos: detener si exploramos demasiados nodos
    if (nodesExplored >= MAX_NODES)
        return evaluate(model, aiPlayer);

    // Caso base: profundidad 0 o juego terminado
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

        // Verificar si el oponente tampoco tiene movimientos (fin del juego)
        Moves opponentMoves;
        getValidMoves(newModel, opponentMoves);
        if (opponentMoves.size() == 0)
        {
            newModel.gameOver = true;
            return evaluate(newModel, aiPlayer);
        }

        // El oponente juega
        return alphabeta(newModel, depth - 1, alpha, beta, !maximizingPlayer, aiPlayer);
    }

    if (maximizingPlayer)
    {
        // Nodo MAX: maximizar el valor
        int maxEval = INT_MIN;

        for (auto move : validMoves)
        {
            GameModel newModel;
            simulateMove(model, move, newModel);

            int eval = alphabeta(newModel, depth - 1, alpha, beta, false, aiPlayer);
            maxEval = (eval > maxEval) ? eval : maxEval;

            // Poda alfa: si maxEval >= beta, podar
            alpha = (eval > alpha) ? eval : alpha;
            if (beta <= alpha)
                break; // Poda Beta
        }

        return maxEval;
    }
    else
    {
        // Nodo MIN: minimizar el valor
        int minEval = INT_MAX;

        for (auto move : validMoves)
        {
            GameModel newModel;
            simulateMove(model, move, newModel);

            int eval = alphabeta(newModel, depth - 1, alpha, beta, true, aiPlayer);
            minEval = (eval < minEval) ? eval : minEval;

            // Poda beta: si minEval <= alpha, podar
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

    // Si no hay movimientos válidos, retornar uno inválido
    if (validMoves.size() == 0)
        return GAME_INVALID_SQUARE;

    // Si solo hay un movimiento, retornarlo directamente
    if (validMoves.size() == 1)
        return validMoves[0];

    // Reiniciar contador de nodos
    nodesExplored = 0;

    // Buscar el mejor movimiento usando Minimax con Alfa-Beta
    Square bestMove = validMoves[0];
    int bestValue = INT_MIN;
    Player aiPlayer = model.currentPlayer;

    int alpha = INT_MIN;
    int beta = INT_MAX;

    for (auto move : validMoves)
    {
        GameModel newModel;
        simulateMove(model, move, newModel);

        // Llamar a alphabeta desde la perspectiva del oponente (nivel MIN)
        int moveValue = alphabeta(newModel, MAX_DEPTH - 1, alpha, beta, false, aiPlayer);

        if (moveValue > bestValue)
        {
            bestValue = moveValue;
            bestMove = move;
        }

        // Actualizar alpha en el nodo raíz
        alpha = (moveValue > alpha) ? moveValue : alpha;
    }

    return bestMove;
}