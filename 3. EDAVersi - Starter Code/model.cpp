/**
 * @brief Implements the Reversi game model
 * @author Marc S. Ressl, Francisco Chiusarolli, Tomas Agustin Garcilazo, Juan Luis Brusasca, Luca Mateo Forchiassin
 *
 * @copyright Copyright (c) 2023-2024
 */

#include "raylib.h"

#include "model.h"

 // The 8 possible directions (horizontal, vertical, and diagonal)
int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},  // up-left, up, up-right
        {0, -1},           {0, 1},    // left, right
        {1, -1},  {1, 0},  {1, 1}     // down-left, down, down-right
};

void initModel(GameModel& model)
{
    model.gameOver = true;

    model.playerTime[0] = 0;
    model.playerTime[1] = 0;

    model.lastMove = GAME_INVALID_SQUARE;

    memset(model.board, PIECE_EMPTY, sizeof(model.board));
}

void startModel(GameModel& model)
{
    model.gameOver = false;

    model.currentPlayer = PLAYER_BLACK;

    model.playerTime[0] = 0;
    model.playerTime[1] = 0;
    model.turnTimer = GetTime();

    model.lastMove = GAME_INVALID_SQUARE;

    memset(model.board, PIECE_EMPTY, sizeof(model.board));
    model.board[BOARD_SIZE / 2 - 1][BOARD_SIZE / 2 - 1] = PIECE_WHITE;
    model.board[BOARD_SIZE / 2 - 1][BOARD_SIZE / 2] = PIECE_BLACK;
    model.board[BOARD_SIZE / 2][BOARD_SIZE / 2] = PIECE_WHITE;
    model.board[BOARD_SIZE / 2][BOARD_SIZE / 2 - 1] = PIECE_BLACK;
}

Player getCurrentPlayer(GameModel& model)
{
    return model.currentPlayer;
}

int getScore(GameModel& model, Player player)
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

double getTimer(GameModel& model, Player player)
{
    double turnTime = 0;

    if (!model.gameOver && (player == model.currentPlayer))
        turnTime = GetTime() - model.turnTimer;

    return model.playerTime[player] + turnTime;
}

Piece getBoardPiece(GameModel& model, Square square)
{
    return model.board[square.y][square.x];
}

void setBoardPiece(GameModel& model, Square square, Piece piece)
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
    // Determine current player's piece and opponent's piece
    Piece currentPiece = (model.currentPlayer == PLAYER_WHITE) ? PIECE_WHITE : PIECE_BLACK;
    Piece opponentPiece = (model.currentPlayer == PLAYER_WHITE) ? PIECE_BLACK : PIECE_WHITE;

    // Check each square on the board
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            Square move = { x, y };

            // If the square is not empty, it's not valid
            if (getBoardPiece(model, move) != PIECE_EMPTY)
                continue;

            bool isValid = false;

            // Check each direction
            for (int d = 0; d < 8; d++)
            {
                int dx = directions[d][0];
                int dy = directions[d][1];

                Square current = { x + dx, y + dy };
                bool foundOpponent = false;

                // Advance in this direction while valid
                while (isSquareValid(current))
                {
                    Piece piece = getBoardPiece(model, current);

                    // If we find an empty square, this direction captures nothing
                    if (piece == PIECE_EMPTY)
                        break;

                    // If we find an opponent's piece, keep searching
                    if (piece == opponentPiece)
                    {
                        foundOpponent = true;
                        current.x += dx;
                        current.y += dy;
                        continue;  // Continue advancing in this direction
                    }

                    // If we find our piece
                    if (piece == currentPiece)
                    {
                        // It's only valid if we found at least one opponent's piece before
                        if (foundOpponent)
                        {
                            isValid = true;
                            break;  // Exit the for loop, we found a valid direction
                        }
                        break;  // End this direction
                    }
                }
            }

            // If the move is valid in at least one direction, add it
            if (isValid)
                validMoves.push_back(move);
        }
    }
}

bool playMove(GameModel& model, Square move)
{
    model.lastMove = move;  // NEW: Save the last move

    // Set game piece
    Piece piece =
        (getCurrentPlayer(model) == PLAYER_WHITE)
        ? PIECE_WHITE
        : PIECE_BLACK;

    setBoardPiece(model, move, piece);

    // Determine which is the enemy piece
    Piece enemyPiece = (piece == PIECE_WHITE) ? PIECE_BLACK : PIECE_WHITE;

    // For each of the 8 directions
    for (int dir = 0; dir < 8; dir++)
    {
        int dx = directions[dir][0];  // X displacement
        int dy = directions[dir][1];  // Y displacement

        // Start from the position next to where we placed our piece
        Square current = { move.x + dx, move.y + dy };

        // List to store the enemy pieces we find
        std::vector<Square> toFlip;

        // Advance in this direction while we're on the board
        while (isSquareValid(current))
        {
            Piece currentPiece = getBoardPiece(model, current);

            // If we find an empty square, there's nothing to flip
            if (currentPiece == PIECE_EMPTY)
            {
                break;  // Exit the while
            }

            // If we find an enemy piece, save it
            if (currentPiece == enemyPiece)
            {
                toFlip.push_back(current);
            }

            // If we find our piece
            if (currentPiece == piece)
            {
                // Only flip if there are enemy pieces in between
                if (toFlip.size() > 0)
                {
                    // Flip all the enemy pieces we saved
                    for (int i = 0; i < toFlip.size(); i++)
                    {
                        setBoardPiece(model, toFlip[i], piece);
                    }
                }
                break;  // Exit the while
            }

            // Advance to the next square in this direction
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