/**
 * @file view.cpp
 * @brief Implements the Reversi game view and UI rendering
 * @author Marc S. Ressl, Francisco Chiusarolli, Tomas Agustin Garcilazo,
 *         Juan Luis Brusasca, Luca Mateo Forchiassin
 * @date 2023-2024
 *
 * This file handles all visual aspects of the Reversi game including board
 * rendering, piece display, valid move indicators, score display, timers,
 * and interactive buttons. Uses raylib for graphics rendering.
 */

#include <string>

#include "raylib.h"

#include "controller.h"
#include "model.h"

const char* GAME_NAME = "EDAversi";

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

const float SQUARE_SIZE = 80;
const float SQUARE_PADDING = 1.5F;
const float SQUARE_CONTENT_OFFSET = SQUARE_PADDING;
const float SQUARE_CONTENT_SIZE = SQUARE_SIZE - 2 * SQUARE_PADDING;

const float PIECE_CENTER = SQUARE_SIZE / 2;
const float PIECE_RADIUS = SQUARE_SIZE * 80 / 100 / 2;

const float VALID_MOVE_RADIUS = SQUARE_SIZE * 30 / 100 / 2;

const float BOARD_X = 40;
const float BOARD_Y = 40;
const float BOARD_CONTENT_SIZE = BOARD_SIZE * SQUARE_SIZE;

const float OUTERBORDER_PADDING = 40;
const float OUTERBORDER_X = BOARD_X - OUTERBORDER_PADDING;
const float OUTERBORDER_Y = BOARD_Y - OUTERBORDER_PADDING;
const float OUTERBORDER_WIDTH = 10;
const float OUTERBORDER_SIZE = BOARD_CONTENT_SIZE + 2 * OUTERBORDER_PADDING;

const int TITLE_FONT_SIZE = 72;
const int SUBTITLE_FONT_SIZE = 36;

const float INFO_CENTERED_X = OUTERBORDER_SIZE +
(WINDOW_WIDTH - OUTERBORDER_SIZE) / 2;

const float INFO_TITLE_Y = WINDOW_HEIGHT / 2;

const float INFO_WHITE_SCORE_Y = WINDOW_HEIGHT * 1 / 4 - SUBTITLE_FONT_SIZE / 2;
const float INFO_WHITE_TIME_Y = WINDOW_HEIGHT * 1 / 4 + SUBTITLE_FONT_SIZE / 2;

const float INFO_BLACK_SCORE_Y = WINDOW_HEIGHT * 3 / 4 - SUBTITLE_FONT_SIZE / 2;
const float INFO_BLACK_TIME_Y = WINDOW_HEIGHT * 3 / 4 + SUBTITLE_FONT_SIZE / 2;

const int INFO_BUTTON_WIDTH = 280;
const int INFO_BUTTON_HEIGHT = 64;

const float INFO_PLAYBLACK_BUTTON_X = INFO_CENTERED_X;
const float INFO_PLAYBLACK_BUTTON_Y = WINDOW_HEIGHT * 1 / 8;

const float INFO_PLAYWHITE_BUTTON_X = INFO_CENTERED_X;
const float INFO_PLAYWHITE_BUTTON_Y = WINDOW_HEIGHT * 7 / 8;

const float LAST_MOVE_RING_INNER_OFFSET = 5;

const int VALID_MOVE_ALPHA = 150;

/**
 * @brief Initializes the game window and graphics settings
 *
 * Creates the game window with specified dimensions and sets the target
 * frame rate for smooth rendering.
 */
void initView() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, GAME_NAME);
    SetTargetFPS(60);
}

/**
 * @brief Closes the game window and frees resources
 */
void freeView() {
    CloseWindow();
}

/**
 * @brief Draws text centered at a given position
 *
 * @param position Center point for the text
 * @param fontSize Size of the font
 * @param s String to display
 */
static void drawCenteredText(Vector2 position, int fontSize, std::string s) {
    DrawText(s.c_str(),
        (int)position.x - MeasureText(s.c_str(), fontSize) / 2,
        (int)position.y - fontSize / 2,
        fontSize,
        BROWN);
}

/**
 * @brief Draws a player's score with label
 *
 * @param label Text label to display before the score
 * @param position Center position for the score display
 * @param score Score value to display
 */
static void drawScore(std::string label, Vector2 position, int score) {
    std::string s = label + std::to_string(score);
    drawCenteredText(position, SUBTITLE_FONT_SIZE, s);
}

/**
 * @brief Draws a timer in MM:SS format
 *
 * @param position Center position for the timer display
 * @param time Time in seconds to display
 */
static void drawTimer(Vector2 position, double time) {
    int totalSeconds = (int)time;

    int seconds = totalSeconds % 60;
    int minutes = totalSeconds / 60;

    std::string s;

    if (minutes < 10)
        s.append("0");
    s.append(std::to_string(minutes));
    s.append(":");
    if (seconds < 10)
        s.append("0");
    s.append(std::to_string(seconds));

    drawCenteredText(position, SUBTITLE_FONT_SIZE, s);
}

/**
 * @brief Draws a rectangular button with centered text
 *
 * @param position Center position of the button
 * @param label Text to display on the button
 * @param backgroundColor Color of the button background
 */
static void drawButton(Vector2 position, std::string label,
    Color backgroundColor) {
    DrawRectangle(position.x - INFO_BUTTON_WIDTH / 2,
        position.y - INFO_BUTTON_HEIGHT / 2,
        INFO_BUTTON_WIDTH,
        INFO_BUTTON_HEIGHT,
        backgroundColor);

    drawCenteredText({ position.x, position.y }, SUBTITLE_FONT_SIZE, label.c_str());
}

/**
 * @brief Checks if the mouse pointer is over a button
 *
 * @param position Center position of the button to check
 * @return bool True if mouse is over the button, false otherwise
 */
static bool isMousePointerOverButton(Vector2 position) {
    Vector2 mousePosition = GetMousePosition();

    return (mousePosition.x >= (position.x - INFO_BUTTON_WIDTH / 2)) &&
        (mousePosition.x < (position.x + INFO_BUTTON_WIDTH / 2)) &&
        (mousePosition.y >= (position.y - INFO_BUTTON_HEIGHT / 2)) &&
        (mousePosition.y < (position.y + INFO_BUTTON_HEIGHT / 2));
}

/**
 * @brief Renders the complete game view
 *
 * Draws the game board, pieces, valid move indicators, scores, timers,
 * and game control buttons. Highlights the last move played and shows
 * valid moves for the human player.
 *
 * @param model Current game state to render
 */
void drawView(GameModel& model) {
    BeginDrawing();

    ClearBackground(BEIGE);

    DrawRectangle(OUTERBORDER_X,
        OUTERBORDER_Y,
        OUTERBORDER_SIZE,
        OUTERBORDER_SIZE,
        BLACK);

    Moves validMoves;
    if (!model.gameOver)
        getValidMoves(model, validMoves);

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Square square = { x, y };

            Vector2 position = {
              BOARD_X + (float)square.x * SQUARE_SIZE,
              BOARD_Y + (float)square.y * SQUARE_SIZE
            };

            DrawRectangleRounded({ position.x + SQUARE_CONTENT_OFFSET,
                                  position.y + SQUARE_CONTENT_OFFSET,
                                  SQUARE_CONTENT_SIZE,
                                  SQUARE_CONTENT_SIZE },
                0.2F,
                6,
                DARKGREEN);

            Piece piece = getBoardPiece(model, square);

            if (piece != PIECE_EMPTY) {
                DrawCircle((int)position.x + PIECE_CENTER,
                    (int)position.y + PIECE_CENTER,
                    PIECE_RADIUS,
                    (piece == PIECE_WHITE) ? WHITE : BLACK);

                if (isSquareValid(model.lastMove) &&
                    square.x == model.lastMove.x &&
                    square.y == model.lastMove.y) {
                    DrawRing({ (float)position.x + PIECE_CENTER,
                              (float)position.y + PIECE_CENTER },
                        PIECE_RADIUS - LAST_MOVE_RING_INNER_OFFSET,
                        PIECE_RADIUS,
                        0, 360, 36,
                        (piece == PIECE_WHITE) ? GOLD : ORANGE);
                }
            }
            else if (!model.gameOver &&
                model.currentPlayer == model.humanPlayer) {
                bool isValidMove = false;
                for (const auto& move : validMoves) {
                    if (move.x == square.x && move.y == square.y) {
                        isValidMove = true;
                        break;
                    }
                }

                if (isValidMove) {
                    Color indicatorColor = (model.currentPlayer == PLAYER_BLACK)
                        ? Color{ 50, 50, 50, VALID_MOVE_ALPHA }
                    : Color{ 255, 255, 255, VALID_MOVE_ALPHA };

                    DrawCircle((int)position.x + PIECE_CENTER,
                        (int)position.y + PIECE_CENTER,
                        VALID_MOVE_RADIUS,
                        indicatorColor);
                }
            }
        }
    }

    drawScore("Black score: ",
        { INFO_CENTERED_X, INFO_WHITE_SCORE_Y },
        getScore(model, PLAYER_BLACK));
    drawTimer({ INFO_CENTERED_X, INFO_WHITE_TIME_Y },
        getTimer(model, PLAYER_BLACK));
    drawCenteredText({ INFO_CENTERED_X, INFO_TITLE_Y },
        TITLE_FONT_SIZE,
        GAME_NAME);
    drawScore("White score: ",
        { INFO_CENTERED_X, INFO_BLACK_SCORE_Y },
        getScore(model, PLAYER_WHITE));
    drawTimer({ INFO_CENTERED_X, INFO_BLACK_TIME_Y },
        getTimer(model, PLAYER_WHITE));

    if (model.gameOver) {
        drawButton({ INFO_PLAYBLACK_BUTTON_X, INFO_PLAYBLACK_BUTTON_Y },
            "Play black",
            BLACK);

        drawButton({ INFO_PLAYWHITE_BUTTON_X, INFO_PLAYWHITE_BUTTON_Y },
            "Play white",
            WHITE);
    }

    EndDrawing();
}

/**
 * @brief Returns the board square under the mouse pointer
 *
 * Converts mouse screen coordinates to board square coordinates.
 *
 * @return Square Board square under pointer, or GAME_INVALID_SQUARE if outside
 */
Square getSquareOnMousePointer() {
    Vector2 mousePosition = GetMousePosition();
    Square square = { (int)floor((mousePosition.x - BOARD_X) / SQUARE_SIZE),
                     (int)floor((mousePosition.y - BOARD_Y) / SQUARE_SIZE) };

    if (isSquareValid(square))
        return square;
    else
        return GAME_INVALID_SQUARE;
}

/**
 * @brief Checks if mouse pointer is over the "Play Black" button
 *
 * @return bool True if mouse is over the button, false otherwise
 */
bool isMousePointerOverPlayBlackButton() {
    return isMousePointerOverButton({ INFO_PLAYBLACK_BUTTON_X,
                                     INFO_PLAYBLACK_BUTTON_Y });
}

/**
 * @brief Checks if mouse pointer is over the "Play White" button
 *
 * @return bool True if mouse is over the button, false otherwise
 */
bool isMousePointerOverPlayWhiteButton() {
    return isMousePointerOverButton({ INFO_PLAYWHITE_BUTTON_X,
                                     INFO_PLAYWHITE_BUTTON_Y });
}