/**
 * @brief Implements the Reversi game controller
 * @author Marc S. Ressl, Francisco Chiusarolli, Tomas Agustin Garcilazo, Juan Luis Brusasca, Luca Mateo Forchiassin
 *
 * @copyright Copyright (c) 2023-2024
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "model.h"

/**
 * @brief Updates the game view.
 *
 * @param The game model.
 * @return Should the view be closed?
 */
bool updateView(GameModel &model);

#endif
