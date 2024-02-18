#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

#include "Wall.h"
#include "Bullet.h"
#include "Player.h"
#include "Game.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <server-ip>" << std::endl;
        return 1;
    }

    Game game;
    game.run(argv[1]);

    return 0;
}