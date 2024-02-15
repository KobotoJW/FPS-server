#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

#include "Wall.h"
#include "Bullet.h"
#include "Player.h"
#include "Game.h"

const std::string CAP_POST = "5t0p";

int main() {
    Game game;
    game.run();

    return 0;
}