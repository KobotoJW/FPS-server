#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

#include "Wall.h"
#include "Bullet.h"
#include "Player.h"
#include "Game.h"

int main() {
    Game game;
    game.run();

    return 0;
}