/**
 * @brief Advanced AI implementation for Reversi
 * @author Based on "Estrategia Reversista" by Lea Tosti
 */

#include "ai.h"
#include <algorithm>
#include <limits>
#include <cmath>
#include <array>

 // ==================== CONSTANTES DE EVALUACIÓN ====================

 // Pesos para las diferentes fases del juego
namespace Weights {
    // Early game (primeros ~20 movimientos)
    namespace Early {
        constexpr double MOBILITY = 10.0;
        constexpr double POTENTIAL_MOBILITY = 5.0;
        constexpr double CORNER_CONTROL = 25.0;
        constexpr double STABILITY = 8.0;
        constexpr double EDGE_STABILITY = 5.0;
        constexpr double PIECE_COUNT = -2.0;  // Negativo: queremos pocas fichas
        constexpr double FRONTIER = -5.0;     // Penalizar fichas externas
        constexpr double POSITION = 3.0;
    }

<<<<<<< HEAD
    // Mid game (movimientos ~20-40)
    namespace Mid {
        constexpr double MOBILITY = 8.0;
        constexpr double POTENTIAL_MOBILITY = 4.0;
        constexpr double CORNER_CONTROL = 30.0;
        constexpr double STABILITY = 15.0;
        constexpr double EDGE_STABILITY = 10.0;
        constexpr double PIECE_COUNT = 0.0;
        constexpr double FRONTIER = -4.0;
        constexpr double POSITION = 2.0;
        constexpr double PARITY = 5.0;
    }

    // End game (últimos ~20 movimientos)
    namespace End {
        constexpr double MOBILITY = 5.0;
        constexpr double POTENTIAL_MOBILITY = 2.0;
        constexpr double CORNER_CONTROL = 40.0;
        constexpr double STABILITY = 25.0;
        constexpr double EDGE_STABILITY = 15.0;
        constexpr double PIECE_COUNT = 10.0;  // Positivo: contar fichas
        constexpr double FRONTIER = -2.0;
        constexpr double POSITION = 1.0;
        constexpr double PARITY = 15.0;
    }
}

// Direcciones para buscar fichas (8 direcciones)
const std::array<std::pair<int, int>, 8> DIRECTIONS = { {
    {-1, -1}, {-1, 0}, {-1, 1},
    {0, -1},           {0, 1},
    {1, -1},  {1, 0},  {1, 1}
} };

// Matriz de valores posicionales (según importancia estratégica)
const int POSITION_WEIGHTS[BOARD_SIZE][BOARD_SIZE] = {
    {100, -20,  10,   5,   5,  10, -20, 100},
    {-20, -50,  -2,  -2,  -2,  -2, -50, -20},
    { 10,  -2,   5,   1,   1,   5,  -2,  10},
    {  5,  -2,   1,   0,   0,   1,  -2,   5},
    {  5,  -2,   1,   0,   0,   1,  -2,   5},
    { 10,  -2,   5,   1,   1,   5,  -2,  10},
    {-20, -50,  -2,  -2,  -2,  -2, -50, -20},
    {100, -20,  10,   5,   5,  10, -20, 100}
=======
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
>>>>>>> 2e2afc2e35ec3a62511aa59954803de0c47af064
};

// Casillas especiales
const std::array<Square, 4> CORNERS = { {
    {0, 0}, {0, 7}, {7, 0}, {7, 7}
} };

const std::array<Square, 8> X_SQUARES = { {
    {1, 1}, {1, 6}, {6, 1}, {6, 6},  // Casillas X (diagonales a esquinas)
    {0, 1}, {1, 0}, {0, 6}, {1, 7},  // Casillas C (adyacentes a esquinas)
} };

// ==================== ESTRUCTURAS AUXILIARES ====================

struct EvaluationWeights {
    double mobility;
    double potentialMobility;
    double cornerControl;
    double stability;
    double edgeStability;
    double pieceCount;
    double frontier;
    double position;
    double parity;
};

// ==================== FUNCIONES AUXILIARES ====================

inline Piece getOpponentPiece(Piece piece) {
    return (piece == PIECE_BLACK) ? PIECE_WHITE : PIECE_BLACK;
}

<<<<<<< HEAD
inline Player getOpponentPlayer(Player player) {
    return (player == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK;
}

inline int countPieces(GameModel& model, Piece piece) {
    int count = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (model.board[y][x] == piece)
                count++;
        }
    }
    return count;
}

inline int getTotalPieces(GameModel& model) {
    return countPieces(model, PIECE_BLACK) + countPieces(model, PIECE_WHITE);
}
=======
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
>>>>>>> 2e2afc2e35ec3a62511aa59954803de0c47af064

inline int getGamePhase(GameModel& model) {
    int totalPieces = getTotalPieces(model);
    if (totalPieces < 24) return 0;      // Early game
    if (totalPieces < 44) return 1;      // Mid game
    return 2;                             // End game
}

EvaluationWeights getWeightsForPhase(int phase) {
    EvaluationWeights weights;

    if (phase == 0) {  // Early game
        weights.mobility = Weights::Early::MOBILITY;
        weights.potentialMobility = Weights::Early::POTENTIAL_MOBILITY;
        weights.cornerControl = Weights::Early::CORNER_CONTROL;
        weights.stability = Weights::Early::STABILITY;
        weights.edgeStability = Weights::Early::EDGE_STABILITY;
        weights.pieceCount = Weights::Early::PIECE_COUNT;
        weights.frontier = Weights::Early::FRONTIER;
        weights.position = Weights::Early::POSITION;
        weights.parity = 0.0;
    }
    else if (phase == 1) {  // Mid game
        weights.mobility = Weights::Mid::MOBILITY;
        weights.potentialMobility = Weights::Mid::POTENTIAL_MOBILITY;
        weights.cornerControl = Weights::Mid::CORNER_CONTROL;
        weights.stability = Weights::Mid::STABILITY;
        weights.edgeStability = Weights::Mid::EDGE_STABILITY;
        weights.pieceCount = Weights::Mid::PIECE_COUNT;
        weights.frontier = Weights::Mid::FRONTIER;
        weights.position = Weights::Mid::POSITION;
        weights.parity = Weights::Mid::PARITY;
    }
    else {  // End game
        weights.mobility = Weights::End::MOBILITY;
        weights.potentialMobility = Weights::End::POTENTIAL_MOBILITY;
        weights.cornerControl = Weights::End::CORNER_CONTROL;
        weights.stability = Weights::End::STABILITY;
        weights.edgeStability = Weights::End::EDGE_STABILITY;
        weights.pieceCount = Weights::End::PIECE_COUNT;
        weights.frontier = Weights::End::FRONTIER;
        weights.position = Weights::End::POSITION;
        weights.parity = Weights::End::PARITY;
    }

<<<<<<< HEAD
    return weights;
}

// ==================== MOVILIDAD ====================

int countMobility(GameModel& model, Player player) {
    Player originalPlayer = model.currentPlayer;
    model.currentPlayer = player;

    Moves validMoves;
    getValidMoves(model, validMoves);
    int mobility = validMoves.size();

    model.currentPlayer = originalPlayer;
    return mobility;
}

// Movilidad potencial: cuenta casillas vacías adyacentes a fichas del oponente
int countPotentialMobility(GameModel& model, Player player) {
    Piece opponentPiece = (player == PLAYER_BLACK) ? PIECE_WHITE : PIECE_BLACK;
    int potential = 0;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (model.board[y][x] == PIECE_EMPTY) {
                // Verificar si hay una ficha oponente adyacente
                for (const auto& dir : DIRECTIONS) {
                    int nx = x + dir.first;
                    int ny = y + dir.second;

                    if (isSquareValid({ nx, ny }) &&
                        model.board[ny][nx] == opponentPiece) {
                        potential++;
                        break;
                    }
=======
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
>>>>>>> 2e2afc2e35ec3a62511aa59954803de0c47af064
                }
            }
        }
    }

<<<<<<< HEAD
    return potential;
=======
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
>>>>>>> 2e2afc2e35ec3a62511aa59954803de0c47af064
}

// ==================== FICHAS ESTABLES ====================

// Verifica si una ficha es estable (no puede ser volteada)
bool isStable(GameModel& model, Square square) {
    Piece piece = model.board[square.y][square.x];
    if (piece == PIECE_EMPTY) return false;

    // Las esquinas siempre son estables
    for (const auto& corner : CORNERS) {
        if (square.x == corner.x && square.y == corner.y)
            return true;
    }

    // Verificar estabilidad por todas las direcciones
    bool stable = true;

    for (const auto& dir : DIRECTIONS) {
        bool stableInDirection = false;

        // Verificar hacia un lado
        int x = square.x - dir.first;
        int y = square.y - dir.second;
        bool reachedEdge1 = false;
        while (isSquareValid({ x, y })) {
            if (model.board[y][x] != piece) break;
            x -= dir.first;
            y -= dir.second;
            if (!isSquareValid({ x, y })) {
                reachedEdge1 = true;
                break;
            }
        }

        // Verificar hacia el otro lado
        x = square.x + dir.first;
        y = square.y + dir.second;
        bool reachedEdge2 = false;
        while (isSquareValid({ x, y })) {
            if (model.board[y][x] != piece) break;
            x += dir.first;
            y += dir.second;
            if (!isSquareValid({ x, y })) {
                reachedEdge2 = true;
                break;
            }
        }

        if (reachedEdge1 && reachedEdge2) {
            stableInDirection = true;
        }

        if (!stableInDirection) {
            stable = false;
            break;
        }
    }

    return stable;
}

int countStablePieces(GameModel& model, Piece piece) {
    int count = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (model.board[y][x] == piece && isStable(model, { x, y }))
                count++;
        }
    }
    return count;
}

// ==================== CONTROL DE ESQUINAS ====================

int evaluateCornerControl(GameModel& model, Piece piece) {
    int score = 0;

<<<<<<< HEAD
    for (const auto& corner : CORNERS) {
        if (model.board[corner.y][corner.x] == piece) {
            score += 100;  // Esquina capturada
        }
        else if (model.board[corner.y][corner.x] == PIECE_EMPTY) {
            // Penalizar por casillas X y C ocupadas si la esquina está vacía
            int cx = corner.x;
            int cy = corner.y;
=======
/**
 * @brief Ordena movimientos con heurística mejorada
 */
void orderMoves(GameModel& model, Moves& moves, bool maximizing)
{
    if (moves.size() <= 1)
        return;
>>>>>>> 2e2afc2e35ec3a62511aa59954803de0c47af064

            // Casilla X (diagonal)
            int dx = (cx == 0) ? 1 : -1;
            int dy = (cy == 0) ? 1 : -1;
            if (model.board[cy + dy][cx + dx] == piece) {
                score -= 25;  // Mal: ocupar X sin controlar esquina
            }

<<<<<<< HEAD
            // Casillas C (adyacentes)
            if (model.board[cy][cx + dx] == piece) {
                score -= 15;
            }
            if (model.board[cy + dy][cx] == piece) {
                score -= 15;
            }
        }
=======
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
>>>>>>> 2e2afc2e35ec3a62511aa59954803de0c47af064
    }

    return score;
}

// ==================== FICHAS DE FRONTERA ====================

// Cuenta fichas externas (con casillas vacías adyacentes)
int countFrontierPieces(GameModel& model, Piece piece) {
    int count = 0;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (model.board[y][x] == piece) {
                // Verificar si hay casilla vacía adyacente
                for (const auto& dir : DIRECTIONS) {
                    int nx = x + dir.first;
                    int ny = y + dir.second;

                    if (isSquareValid({ nx, ny }) &&
                        model.board[ny][nx] == PIECE_EMPTY) {
                        count++;
                        break;
                    }
                }
            }
        }
    }

    return count;
}

// ==================== ESTABILIDAD DE BORDES ====================

int evaluateEdgeStability(GameModel& model, Piece piece) {
    int score = 0;

    // Evaluar los 4 bordes
    for (int i = 0; i < BOARD_SIZE; i++) {
        // Borde superior
        if (model.board[0][i] == piece) score += 4;
        // Borde inferior
        if (model.board[7][i] == piece) score += 4;
        // Borde izquierdo
        if (model.board[i][0] == piece) score += 4;
        // Borde derecho
        if (model.board[i][7] == piece) score += 4;
    }

    return score;
}

// ==================== EVALUACIÓN POSICIONAL ====================

int evaluatePosition(GameModel& model, Piece piece) {
    int score = 0;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (model.board[y][x] == piece) {
                score += POSITION_WEIGHTS[y][x];
            }
        }
    }

    return score;
}

// ==================== PARIDAD ====================

int evaluateParity(GameModel& model) {
    int emptySquares = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (model.board[y][x] == PIECE_EMPTY)
                emptySquares++;
        }
    }

    // Si el número de casillas vacías es impar, el jugador actual tiene ventaja
    return (emptySquares % 2 == 1) ? 10 : -10;
}

// ==================== FUNCIÓN DE EVALUACIÓN PRINCIPAL ====================

double evaluatePosition(GameModel& model, Player player) {
    Piece playerPiece = (player == PLAYER_BLACK) ? PIECE_BLACK : PIECE_WHITE;
    Piece opponentPiece = getOpponentPiece(playerPiece);

    int phase = getGamePhase(model);
    EvaluationWeights weights = getWeightsForPhase(phase);

    double score = 0.0;

    // 1. Movilidad (más importante en early/mid game)
    int playerMobility = countMobility(model, player);
    int opponentMobility = countMobility(model, getOpponentPlayer(player));
    if (playerMobility + opponentMobility > 0) {
        score += weights.mobility * 100.0 * (playerMobility - opponentMobility) /
            (playerMobility + opponentMobility);
    }

    // 2. Movilidad potencial
    int playerPotential = countPotentialMobility(model, player);
    int opponentPotential = countPotentialMobility(model, getOpponentPlayer(player));
    if (playerPotential + opponentPotential > 0) {
        score += weights.potentialMobility * 100.0 * (playerPotential - opponentPotential) /
            (playerPotential + opponentPotential);
    }

    // 3. Control de esquinas
    int playerCorners = evaluateCornerControl(model, playerPiece);
    int opponentCorners = evaluateCornerControl(model, opponentPiece);
    score += weights.cornerControl * (playerCorners - opponentCorners);

    // 4. Fichas estables
    int playerStable = countStablePieces(model, playerPiece);
    int opponentStable = countStablePieces(model, opponentPiece);
    score += weights.stability * (playerStable - opponentStable);

    // 5. Estabilidad de bordes
    int playerEdge = evaluateEdgeStability(model, playerPiece);
    int opponentEdge = evaluateEdgeStability(model, opponentPiece);
    score += weights.edgeStability * (playerEdge - opponentEdge);

    // 6. Conteo de fichas (solo importante al final)
    int playerPieces = countPieces(model, playerPiece);
    int opponentPieces = countPieces(model, opponentPiece);
    score += weights.pieceCount * (playerPieces - opponentPieces);

    // 7. Fichas de frontera (queremos minimizarlas)
    int playerFrontier = countFrontierPieces(model, playerPiece);
    int opponentFrontier = countFrontierPieces(model, opponentPiece);
    score += weights.frontier * (playerFrontier - opponentFrontier);

    // 8. Evaluación posicional
    int playerPosition = evaluatePosition(model, playerPiece);
    int opponentPosition = evaluatePosition(model, opponentPiece);
    score += weights.position * (playerPosition - opponentPosition);

    // 9. Paridad (solo en endgame)
    if (phase == 2) {
        int parityScore = evaluateParity(model);
        if (model.currentPlayer == player) {
            score += weights.parity * parityScore;
        }
        else {
            score -= weights.parity * parityScore;
        }
    }

    return score;
}

// ==================== ALGORITMO MINIMAX CON ALPHA-BETA PRUNING ====================

double minimax(GameModel& model, int depth, double alpha, double beta, bool isMaximizing, Player originalPlayer) {
    // Caso base: profundidad alcanzada o juego terminado
    if (depth == 0 || model.gameOver) {
        return evaluatePosition(model, originalPlayer);
    }

    Moves validMoves;
    getValidMoves(model, validMoves);

    // Si no hay movimientos válidos, pasar turno
    if (validMoves.empty()) {
        GameModel newModel = model;
        newModel.currentPlayer = getOpponentPlayer(model.currentPlayer);

        Moves opponentMoves;
        getValidMoves(newModel, opponentMoves);

        if (opponentMoves.empty()) {
            // Juego terminado
            newModel.gameOver = true;
            return evaluatePosition(newModel, originalPlayer);
        }

        // Pasar turno
        return minimax(newModel, depth - 1, alpha, beta, !isMaximizing, originalPlayer);
    }

<<<<<<< HEAD
    if (isMaximizing) {
        double maxEval = -std::numeric_limits<double>::infinity();
=======
    // Ordenar movimientos para mejorar poda
    orderMoves(model, validMoves, maximizingPlayer);
>>>>>>> 2e2afc2e35ec3a62511aa59954803de0c47af064

        for (const auto& move : validMoves) {
            GameModel newModel = model;
            playMove(newModel, move);

            double eval = minimax(newModel, depth - 1, alpha, beta, false, originalPlayer);
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);

            if (beta <= alpha)
                break;  // Poda beta
        }

        return maxEval;
    }
    else {
        double minEval = std::numeric_limits<double>::infinity();

        for (const auto& move : validMoves) {
            GameModel newModel = model;
            playMove(newModel, move);

            double eval = minimax(newModel, depth - 1, alpha, beta, true, originalPlayer);
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);

            if (beta <= alpha)
                break;  // Poda alpha
        }

        return minEval;
    }
}

// ==================== FUNCIÓN PRINCIPAL DE LA IA ====================

Square getBestMove(GameModel& model) {
    Moves validMoves;
    getValidMoves(model, validMoves);

    if (validMoves.empty())
        return GAME_INVALID_SQUARE;

    // Ajustar profundidad según la fase del juego
    int phase = getGamePhase(model);
    int depth;

    if (phase == 0) {
        depth = 5;  // Early game: búsqueda más profunda
    }
    else if (phase == 1) {
        depth = 6;  // Mid game: profundidad media
    }
    else {
        depth = 8;  // End game: búsqueda muy profunda
    }

    Square bestMove = validMoves[0];
    double bestValue = -std::numeric_limits<double>::infinity();

<<<<<<< HEAD
    for (const auto& move : validMoves) {
        GameModel newModel = model;
        playMove(newModel, move);
=======
    // BÚSQUEDA ITERATIVA EN PROFUNDIDAD
    for (int depth = 1; depth <= 20; depth++)
    {
        // Detenerse al 90% del límite para evitar cortes abruptos
        if (nodesExplored >= MAX_NODES * 0.9)
            break;
>>>>>>> 2e2afc2e35ec3a62511aa59954803de0c47af064

        double value = minimax(newModel, depth - 1,
            -std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::infinity(),
            false, model.currentPlayer);

<<<<<<< HEAD
        if (value > bestValue) {
            bestValue = value;
            bestMove = move;
=======
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
>>>>>>> 2e2afc2e35ec3a62511aa59954803de0c47af064
        }
    }

    return bestMove;
}