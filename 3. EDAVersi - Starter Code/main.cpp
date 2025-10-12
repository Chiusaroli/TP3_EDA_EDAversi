/**
 * @brief Reversi game
 * @author Marc S. Ressl, Francisco Chiusarolli, Tomas Agustin Garcilazo, Juan Luis Brusasca, Luca Mateo Forchiassin
 *
 * @copyright Copyright (c) 2023-2024
 */

#include "model.h"
#include "view.h"
#include "controller.h"

int main()
{
    GameModel model;

    initModel(model);
    initView();

    while (updateView(model))
        ;

    freeView();
}
