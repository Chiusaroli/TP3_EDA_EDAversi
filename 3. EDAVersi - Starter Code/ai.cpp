/**
 * @brief Implements the Reversi game AI
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2023-2024
 */

#include <cstdlib>
#include <climits>

#include "ai.h"
#include "controller.h"

 // Profundidad máxima del árbol Minimax
#define MAX_DEPTH 5

/**
 * @brief Función de evaluación: retorna la diferencia de puntajes
 *
 * @param model El modelo del juego
 * @param player El jugador que queremos maximizar
 * @return Valor de evaluación (positivo es bueno para player)
 */
int evaluate(GameModel& model, Player player)
{
    Player opponent = (player == PLAYER_WHITE) ? PLAYER_BLACK : PLAYER_WHITE;
    return getScore(model, player) - getScore(model, opponent);
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
 * @brief Implementa el algoritmo Minimax recursivo
 *
 * @param model El modelo del juego actual
 * @param depth Profundidad actual en el árbol
 * @param maximizingPlayer true si es turno de MAX, false si es MIN
 * @param aiPlayer El jugador que representa la IA
 * @return El valor minimax del nodo
 */
int minimax(GameModel& model, int depth, bool maximizingPlayer, Player aiPlayer)
{
    // Caso base: profundidad 0 o juego terminado
    if (depth == 0 || model.gameOver)
    {
        return evaluate(model, aiPlayer);
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

        // Verificar si el oponente tampoco tiene movimientos (fin del juego)
        Moves opponentMoves;
        getValidMoves(newModel, opponentMoves);
        if (opponentMoves.size() == 0)
        {
            newModel.gameOver = true;
            return evaluate(newModel, aiPlayer);
        }

        // El oponente juega
        return minimax(newModel, depth - 1, !maximizingPlayer, aiPlayer);
    }

    if (maximizingPlayer)
    {
        // Nodo MAX: maximizar el valor
        int maxEval = INT_MIN;

        for (auto move : validMoves)
        {
            GameModel newModel;
            simulateMove(model, move, newModel);

            int eval = minimax(newModel, depth - 1, false, aiPlayer);
            maxEval = (eval > maxEval) ? eval : maxEval;
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

            int eval = minimax(newModel, depth - 1, true, aiPlayer);
            minEval = (eval < minEval) ? eval : minEval;
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

    // Buscar el mejor movimiento usando Minimax
    Square bestMove = validMoves[0];
    int bestValue = INT_MIN;
    Player aiPlayer = model.currentPlayer;

    for (auto move : validMoves)
    {
        GameModel newModel;
        simulateMove(model, move, newModel);

        // Llamar a minimax desde la perspectiva del oponente (nivel MIN)
        int moveValue = minimax(newModel, MAX_DEPTH - 1, false, aiPlayer);

        if (moveValue > bestValue)
        {
            bestValue = moveValue;
            bestMove = move;
        }
    }

    return bestMove;
}
