#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <iostream>
#include "Player.h"
#include "Bullet.h"
#include "Wall.h"

enum class GameState {
    Joining,
    Playing,
    Dead,
    Disconnecting
};

class Game {
public:
    Game();
    void run();
    sf::TcpSocket& getSocket();
    bool connectToServer();
    void receiveDataFromServer();
    void handleReceivedData(const char* data, ssize_t dataSize);
    void pingServer();
    void recievedPongFromServer();
    void disconnectFromServer();

private:
    GameState gameState;
    sf::Clock disconnectClock;
    sf::Clock pingClock;
    void recieveMapFromServer();
    void processEvents();
    void update();
    void render();
    void processDeadState();
    void respawnPlayer();

    sf::RenderWindow window;
    Player player;
    std::vector<Bullet> bullets;
    std::vector<Player> players;
    std::vector<Wall> walls;
    sf::RectangleShape floor;

    sf::TcpSocket socket;
};