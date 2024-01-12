#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <queue>
#include <mutex>

enum class PacketType {
    Welcome,
    Message,
    PlayerPosition,
    PlayerVelocity,
    PlayerDisconnected
};

struct Packet {
    PacketType type;
    std::string message;
    int playerId;
    float x;
    float y;
    float vx;
    float vy;
};

// Thread-safe queue
class SafeQueue {
private:
    std::queue<std::string> queue;
    mutable std::mutex mtx;

public:
    void push(const std::string& item) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(item);
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }

    std::string pop() {
        std::lock_guard<std::mutex> lock(mtx);
        std::string item = queue.front();
        queue.pop();
        return item;
    }
};

SafeQueue messageQueue;

enum class GameState {
    ServerConnection,
    Playing
};

class Player {
public:
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    int clientId;

    Player(float x, float y) : clientId(-1) {
        shape.setSize(sf::Vector2f(50, 50));
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color::Green);
    }

    void update() {
        shape.move(velocity);
    }
};

void networkThread(const std::string& serverIp, int port, Player& player) {
    std::thread([serverIp, port, &player]() {
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            std::cerr << "Error creating socket" << std::endl;
            return;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);

        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            std::cerr << "Error connecting to server" << std::endl;
            close(clientSocket);
            return;
        }

        std::cout << "Connected to server" << std::endl;

        while (true) {
            Packet packet;
            int bytesRead = recv(clientSocket, &packet, sizeof(packet), 0);
            if (bytesRead <= 0) {
                std::cerr << "Server disconnected" << std::endl;
                break;
            }

            switch (packet.type) {
                case PacketType::Welcome:
                    player.clientId = packet.playerId;
                    std::cout << "Welcome: got id: " << player.clientId << std::endl;
                    break;
                case PacketType::Message:
                    std::cout << "Server: " << packet.message << std::endl;
                    break;
                case PacketType::PlayerPosition:
                    std::cout << "Player " << packet.playerId << " position: (" << packet.x << ", " << packet.y << ")" << std::endl;
                    break;
                case PacketType::PlayerVelocity:
                    std::cout << "Player " << packet.playerId << " velocity: (" << packet.vx << ", " << packet.vy << ")" << std::endl;
                    break;
                case PacketType::PlayerDisconnected:
                    std::cout << "Player " << packet.playerId << " disconnected" << std::endl;
                    break;
            }
        }
    }).detach();
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server-ip> <port>" << std::endl;
        return 1;
    }

    std::string serverIp = argv[1];
    int port = std::stoi(argv[2]);

    GameState state = GameState::ServerConnection;

    sf::RenderWindow window(sf::VideoMode(800, 600), "Simple Shooter Game");
    window.setFramerateLimit(60);

    Player player(400, 300);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        while (!messageQueue.empty()) {
            std::string message = messageQueue.pop();
            std::cout << "Server: " << message << std::endl;
        }

        if (state == GameState::Playing) {
            // Handle player input
            player.velocity = sf::Vector2f(0, 0);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
                player.velocity.y = -3.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                player.velocity.y = 3.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
                player.velocity.x = -3.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
                player.velocity.x = 3.0f;

            // Update game entities
            player.update();

            // Render
            window.clear();
            window.draw(player.shape);
            window.display();
        } else if (state == GameState::ServerConnection) {
            // Render server connection screen
            window.clear();
            // ...

            // Handle user input
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
                state = GameState::Playing;
            }

            window.display();
        }
    }

    return 0;
}
