/**
 * @file model.cpp
 * @brief Implements the Reversi game model
 * @author Marc S. Ressl, Francisco Chiusarolli, Tomas Agustin Garcilazo,
 *         Juan Luis Brusasca, Luca Mateo Forchiassin
 * @date 2023-2024
 *
 * This file implements the core game logic for Reversi, including board state
 * management, move validation, piece flipping, score calculation, and game flow
 * control. It handles the rules of Reversi and maintains game state.
 */

#include "raylib.h"

#include "model.h"

const int DIRECTION_COUNT = 8;

int directions[DIRECTION_COUNT][2] = {
  {-1, -1}, {-1, 0}, {-1, 1},
  {0, -1},           {0, 1},
  {1, -1},  {1, 0},  {1, 1}
};

/**
 * @brief Initializes the game model to a neutral state
 *
 * Sets the game to "game over" state with an empty board and reset timers.
 * Used for initial setup before starting a new game.
 *
 * @param model Game model to initialize (modified)
 */
void initModel(GameModel& model) {
    model.gameOver = true;

    model.playerTime[0] = 0;
    model.playerTime[1] = 0;

    model.lastMove = GAME_INVALID_SQUARE;

    memset(model.board, PIECE_EMPTY, sizeof(model.board));
}

/**
 * @brief Starts a new game with initial Reversi configuration
 *
 * Sets up the standard Reversi starting position with four pieces in the
 * center (two white, two black in diagonal pattern), resets timers, and
 * sets black as the starting player.
 *
 * @param model Game model to start (modified)
 */
void startModel(GameModel& model) {
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

/**
 * @brief Returns the current player
 *
 * @param model Current game state
 * @return Player Current player (PLAYER_WHITE or PLAYER_BLACK)
 */
Player getCurrentPlayer(GameModel& model) {
    return model.currentPlayer;
}

/**
 * @brief Calculates the score for a given player
 *
 * Counts the number of pieces on the board belonging to the specified player.
 *
 * @param model Current game state
 * @param player Player whose score to calculate
 * @return int Number of pieces belonging to the player
 */
int getScore(GameModel& model, Player player) {
    int score = 0;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (((model.board[y][x] == PIECE_WHITE) && (player == PLAYER_WHITE)) ||
                ((model.board[y][x] == PIECE_BLACK) && (player == PLAYER_BLACK)))
                score++;
        }
    }

    return score;
}

/**
 * @brief Returns accumulated time for a player
 *
 * Calculates total time spent by the player including the current turn
 * if it's their turn and the game is still active.
 *
 * @param model Current game state
 * @param player Player whose time to retrieve
 * @return double Total time in seconds
 */
double getTimer(GameModel& model, Player player) {
    double turnTime = 0;

    if (!model.gameOver && (player == model.currentPlayer))
        turnTime = GetTime() - model.turnTimer;

    return model.playerTime[player] + turnTime;
}

/**
 * @brief Returns the piece at a given board position
 *
 * @param model Current game state
 * @param square Board position to query
 * @return Piece Piece at the position (PIECE_EMPTY, PIECE_WHITE, or PIECE_BLACK)
 */
Piece getBoardPiece(GameModel& model, Square square) {
    return model.board[square.y][square.x];
}

/**
 * @brief Sets a piece at a given board position
 *
 * @param model Current game state (modified)
 * @param square Board position to set
 * @param piece Piece to place at the position
 */
void setBoardPiece(GameModel& model, Square square, Piece piece) {
    model.board[square.y][square.x] = piece;
}

/**
 * @brief Checks if a square is within board boundaries
 *
 * @param square Square to validate
 * @return bool True if square is within bounds, false otherwise
 */
bool isSquareValid(Square square) {
    return (square.x >= 0) && (square.x < BOARD_SIZE) &&
        (square.y >= 0) && (square.y < BOARD_SIZE);
}

/**
 * @brief Finds all valid moves for the current player
 *
 * A move is valid if it places a piece that captures at least one opponent
 * piece in any direction (horizontal, vertical, or diagonal). The move must
 * form a continuous line of opponent pieces ending with the player's own piece.
 *
 * @param model Current game state
 * @param validMoves Output list of valid moves (cleared and populated)
 */
void getValidMoves(GameModel& model, Moves& validMoves) {
    Piece currentPiece = (model.currentPlayer == PLAYER_WHITE)
        ? PIECE_WHITE : PIECE_BLACK;
    Piece opponentPiece = (model.currentPlayer == PLAYER_WHITE)
        ? PIECE_BLACK : PIECE_WHITE;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Square move = { x, y };

            if (getBoardPiece(model, move) != PIECE_EMPTY)
                continue;

            bool isValid = false;

            for (int d = 0; d < DIRECTION_COUNT; d++) {
                int dx = directions[d][0];
                int dy = directions[d][1];

                Square current = { x + dx, y + dy };
                bool foundOpponent = false;

                while (isSquareValid(current)) {
                    Piece piece = getBoardPiece(model, current);

                    if (piece == PIECE_EMPTY)
                        break;

                    if (piece == opponentPiece) {
                        foundOpponent = true;
                        current.x += dx;
                        current.y += dy;
                        continue;
                    }

                    if (piece == currentPiece) {
                        if (foundOpponent) {
                            isValid = true;
                            break;
                        }
                        break;
                    }
                }
            }

            if (isValid)
                validMoves.push_back(move);
        }
    }
}

/**
 * @brief Executes a move on the board
 *
 * Places the current player's piece at the specified position and flips all
 * captured opponent pieces in all valid directions. Updates game timers,
 * switches to the next player, and checks for game over conditions.
 *
 * Game ends when neither player has valid moves available.
 *
 * @param model Current game state (modified)
 * @param move Square where the piece is placed
 * @return bool True if move was executed successfully
 */
bool playMove(GameModel& model, Square move) {
    model.lastMove = move;

    Piece piece = (getCurrentPlayer(model) == PLAYER_WHITE)
        ? PIECE_WHITE : PIECE_BLACK;

    setBoardPiece(model, move, piece);

    Piece enemyPiece = (piece == PIECE_WHITE) ? PIECE_BLACK : PIECE_WHITE;

    for (int dir = 0; dir < DIRECTION_COUNT; dir++) {
        int dx = directions[dir][0];
        int dy = directions[dir][1];

        Square current = { move.x + dx, move.y + dy };

        std::vector<Square> toFlip;

        while (isSquareValid(current)) {
            Piece currentPiece = getBoardPiece(model, current);

            if (currentPiece == PIECE_EMPTY)
                break;

            if (currentPiece == enemyPiece) {
                toFlip.push_back(current);
            }

            if (currentPiece == piece) {
                if (toFlip.size() > 0) {
                    for (int i = 0; i < toFlip.size(); i++) {
                        setBoardPiece(model, toFlip[i], piece);
                    }
                }
                break;
            }

            current.x += dx;
            current.y += dy;
        }
    }

    double currentTime = GetTime();
    model.playerTime[model.currentPlayer] += currentTime - model.turnTimer;
    model.turnTimer = currentTime;

    model.currentPlayer = (model.currentPlayer == PLAYER_WHITE)
        ? PLAYER_BLACK : PLAYER_WHITE;

    Moves validMoves;
    getValidMoves(model, validMoves);

    if (validMoves.size() == 0) {
        model.currentPlayer = (model.currentPlayer == PLAYER_WHITE)
            ? PLAYER_BLACK : PLAYER_WHITE;

        Moves validMoves;
        getValidMoves(model, validMoves);

        if (validMoves.size() == 0)
            model.gameOver = true;
    }

    return true;
}