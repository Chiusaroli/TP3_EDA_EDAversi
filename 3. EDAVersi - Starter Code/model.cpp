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

void getValidMoves(GameModel &model, Moves &validMoves)
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
            Square move = {x, y};
            
            // Si la casilla no est� vac�a, no es v�lida
            if (getBoardPiece(model, move) != PIECE_EMPTY)
                continue;
            
            bool isValid = false;
            
            // Revisar cada direcci�n
            for (int d = 0; d < 8; d++)
            {
                int dx = directions[d][0];
                int dy = directions[d][1];
                
                Square current = {x + dx, y + dy};
                bool foundOpponent = false;
                
                // Avanzar en esta direcci�n
                while (isSquareValid(current))
                {
                    Piece piece = getBoardPiece(model, current);
                    
                    // Si encontramos una casilla vac�a, esta direcci�n no es v�lida
                    if (piece == PIECE_EMPTY)
                        break;
                    
                    // Si encontramos una ficha del oponente, seguimos buscando
                    if (piece == opponentPiece)
                    {
                        foundOpponent = true;
                        current.x += dx;
                        current.y += dy;
                    }
                    // Si encontramos nuestra ficha
                    else if (piece == currentPiece)
                    {
                        // Solo es v�lido si antes encontramos al menos una ficha del oponente
                        if (foundOpponent)
                            isValid = true;
                        break;
                    }
                }
                
                // Si ya encontramos una direcci�n v�lida, no necesitamos seguir
                if (isValid)
                    break;
            }
            
            // Si el movimiento es v�lido en al menos una direcci�n, agregarlo
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

    // To-do: your code goes here...

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
