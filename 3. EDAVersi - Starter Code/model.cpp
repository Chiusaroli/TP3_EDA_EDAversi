/**
 * @brief Implements the Reversi game model
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2023-2024
 */

#include "raylib.h"

#include "model.h"

void initModel(GameModel &model)
{
    model.gameOver = true;

    model.playerTime[0] = 0;
    model.playerTime[1] = 0;

    memset(model.board, PIECE_EMPTY, sizeof(model.board));
}

void startModel(GameModel &model)
{
    model.gameOver = false;

    model.currentPlayer = PLAYER_BLACK;

    model.playerTime[0] = 0;
    model.playerTime[1] = 0;
    model.turnTimer = GetTime();

    memset(model.board, PIECE_EMPTY, sizeof(model.board));
    model.board[BOARD_SIZE / 2 - 1][BOARD_SIZE / 2 - 1] = PIECE_WHITE;
    model.board[BOARD_SIZE / 2 - 1][BOARD_SIZE / 2] = PIECE_BLACK;
    model.board[BOARD_SIZE / 2][BOARD_SIZE / 2] = PIECE_WHITE;
    model.board[BOARD_SIZE / 2][BOARD_SIZE / 2 - 1] = PIECE_BLACK;
}

Player getCurrentPlayer(GameModel &model)
{
    return model.currentPlayer;
}

int getScore(GameModel &model, Player player)
{
    int score = 0;

    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            if (((model.board[y][x] == PIECE_WHITE) &&
                 (player == PLAYER_WHITE)) ||
                ((model.board[y][x] == PIECE_BLACK) &&
                 (player == PLAYER_BLACK)))
                score++;
        }

    return score;
}

double getTimer(GameModel &model, Player player)
{
    double turnTime = 0;

    if (!model.gameOver && (player == model.currentPlayer))
        turnTime = GetTime() - model.turnTimer;

    return model.playerTime[player] + turnTime;
}

Piece getBoardPiece(GameModel &model, Square square)
{
    return model.board[square.y][square.x];
}

void setBoardPiece(GameModel &model, Square square, Piece piece)
{
    model.board[square.y][square.x] = piece;
}

bool isSquareValid(Square square)
{
    return (square.x >= 0) &&
           (square.x < BOARD_SIZE) &&
           (square.y >= 0) &&
           (square.y < BOARD_SIZE);
}

void getValidMoves(GameModel& model, Moves& validMoves)
{
    // Determinar la ficha del jugador actual y del oponente
    Piece currentPiece = (model.currentPlayer == PLAYER_WHITE) ? PIECE_WHITE : PIECE_BLACK;
    Piece opponentPiece = (model.currentPlayer == PLAYER_WHITE) ? PIECE_BLACK : PIECE_WHITE;

    // Las 8 direcciones posibles (horizontal, vertical y diagonal)
    int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},  // arriba-izq, arriba, arriba-der
        {0, -1},           {0, 1},    // izquierda, derecha
        {1, -1},  {1, 0},  {1, 1}     // abajo-izq, abajo, abajo-der
    };

    // Revisar cada casilla del tablero
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            Square move = { x, y };

            // Si la casilla no está vacía, no es válida
            if (getBoardPiece(model, move) != PIECE_EMPTY)
                continue;

            bool isValid = false;

            // Revisar cada dirección
            for (int d = 0; d < 8; d++)
            {
                int dx = directions[d][0];
                int dy = directions[d][1];

                Square current = { x + dx, y + dy };
                bool foundOpponent = false;

                // Avanzar en esta dirección mientras sea válido
                while (isSquareValid(current))
                {
                    Piece piece = getBoardPiece(model, current);

                    // Si encontramos una casilla vacía, esta dirección no captura nada
                    if (piece == PIECE_EMPTY)
                        break;

                    // Si encontramos una ficha del oponente, seguimos buscando
                    if (piece == opponentPiece)
                    {
                        foundOpponent = true;
                        current.x += dx;
                        current.y += dy;
                        continue;  // Seguir avanzando en esta dirección
                    }

                    // Si encontramos nuestra ficha
                    if (piece == currentPiece)
                    {
                        // Solo es válido si antes encontramos al menos una ficha del oponente
                        if (foundOpponent)
                        {
                            isValid = true;
                            break;  // Salir del for, ya encontramos una dirección válida
                        }
                        break;  // Terminar esta dirección
                    }
                }
            }

            // Si el movimiento es válido en al menos una dirección, agregarlo
            if (isValid)
                validMoves.push_back(move);
        }
    }
}

bool playMove(GameModel &model, Square move)
{
    // Set game piece
    Piece piece =
        (getCurrentPlayer(model) == PLAYER_WHITE)
            ? PIECE_WHITE
            : PIECE_BLACK;

    setBoardPiece(model, move, piece);

    // Definir las 8 direcciones posibles (arriba, abajo, izq, der, y 4 diagonales)
    int directions[8][2] = {
        {-1, -1},  // Arriba-Izquierda
        {0, -1},   // Arriba
        {1, -1},   // Arriba-Derecha
        {-1, 0},   // Izquierda
        {1, 0},    // Derecha
        {-1, 1},   // Abajo-Izquierda
        {0, 1},    // Abajo
        {1, 1}     // Abajo-Derecha
    };

    // Determinar cuál es la pieza enemiga
    Piece enemyPiece = (piece == PIECE_WHITE) ? PIECE_BLACK : PIECE_WHITE;

    // Para cada una de las 8 direcciones
    for (int dir = 0; dir < 8; dir++)
    {
        int dx = directions[dir][0];  // Desplazamiento en X
        int dy = directions[dir][1];  // Desplazamiento en Y

        // Empezar desde la posición siguiente a donde pusimos nuestra ficha
        Square current = { move.x + dx, move.y + dy };

        // Lista para guardar las fichas enemigas que encontremos
        std::vector<Square> toFlip;

        // Avanzar en esta dirección mientras estemos en el tablero
        while (isSquareValid(current))
        {
            Piece currentPiece = getBoardPiece(model, current);

            // Si encontramos una casilla vacía, no hay nada que voltear
            if (currentPiece == PIECE_EMPTY)
            {
                break;  // Salir del while
            }

            // Si encontramos una ficha enemiga, la guardamos
            if (currentPiece == enemyPiece)
            {
                toFlip.push_back(current);
            }

            // Si encontramos una ficha nuestra
            if (currentPiece == piece)
            {
                // Solo volteamos si hay fichas enemigas en el medio
                if (toFlip.size() > 0)
                {
                    // Voltear todas las fichas enemigas que guardamos
                    for (int i = 0; i < toFlip.size(); i++)
                    {
                        setBoardPiece(model, toFlip[i], piece);
                    }
                }
                break;  // Salir del while
            }

            // Avanzar a la siguiente casilla en esta dirección
            current.x += dx;
            current.y += dy;
        }
    }

    // Update timer
    double currentTime = GetTime();
    model.playerTime[model.currentPlayer] += currentTime - model.turnTimer;
    model.turnTimer = currentTime;

    // Swap player
    model.currentPlayer =
        (model.currentPlayer == PLAYER_WHITE)
            ? PLAYER_BLACK
            : PLAYER_WHITE;

    // Game over?
    Moves validMoves;
    getValidMoves(model, validMoves);

    if (validMoves.size() == 0)
    {
        // Swap player
        model.currentPlayer =
            (model.currentPlayer == PLAYER_WHITE)
                ? PLAYER_BLACK
                : PLAYER_WHITE;

        Moves validMoves;
        getValidMoves(model, validMoves);

        if (validMoves.size() == 0)
            model.gameOver = true;
    }

    return true;
}
