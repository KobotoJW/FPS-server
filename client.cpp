#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>

const char* SERVER_IP = "127.0.0.1";
const int PORT = 12345;

struct Player {
    int id;
    float x;
    float y;
};

struct Bullet {
    int ownerID;
    float x;
    float y;
    float vx;
    float vy;
};

std::vector<Player> players;
sf::RectangleShape playerShape(sf::Vector2f(50.f, 50.f));

int main() {
    int id = -1;
    sf::TcpSocket socket;
    sf::IpAddress serverIP(SERVER_IP);

    // Connect to the server
    if (socket.connect(serverIP, PORT) != sf::Socket::Done) {
        std::cerr << "Error connecting to server" << std::endl;
        return EXIT_FAILURE;
    }

    // Receive the assigned ID from the server
    std::size_t receivedSize;
    if (socket.receive(&id, sizeof(id), receivedSize) != sf::Socket::Done || receivedSize != sizeof(id)) {
        std::cerr << "Error receiving player ID from server" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Assigned ID: " << id << std::endl;

    sf::RenderWindow window(sf::VideoMode(800, 600), "SFML Game Client");
    window.setFramerateLimit(60);

    // Start a new thread for receiving data
    std::thread receiveThread([&]() {
        while (window.isOpen()) {
            Player updatedPlayer;
            std::size_t receivedSize;
            if (socket.receive(&updatedPlayer, sizeof(updatedPlayer), receivedSize) == sf::Socket::Done &&
                receivedSize == sizeof(updatedPlayer)) {
                // Find the player in the list
                auto it = std::find_if(players.begin(), players.end(), [&updatedPlayer](const Player& player) {
                    return player.id == updatedPlayer.id;
                });

                // If the player is not in the list, add them
                if (it == players.end()) {
                    players.push_back(updatedPlayer);
                } else {
                    // Otherwise, update the player's position
                    it->x = updatedPlayer.x;
                    it->y = updatedPlayer.y;
                }
            }
        }
    });

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // Store the player's direction
            sf::Vector2f playerDirection;

            // Update the player's direction based on the movement keys
            if (event.key.code == sf::Keyboard::W) {
                playerShape.move(0.f, -10.f);
                playerDirection = sf::Vector2f(0.f, -1.f);
            } else if (event.key.code == sf::Keyboard::A) {
                playerShape.move(-10.f, 0.f);
                playerDirection = sf::Vector2f(-1.f, 0.f);
            } else if (event.key.code == sf::Keyboard::S) {
                playerShape.move(0.f, 10.f);
                playerDirection = sf::Vector2f(0.f, 1.f);
            } else if (event.key.code == sf::Keyboard::D) {
                playerShape.move(10.f, 0.f);
                playerDirection = sf::Vector2f(1.f, 0.f);
            }

            // Create a new bullet when the spacebar is pressed
            if (event.key.code == sf::Keyboard::Space) {
                Bullet newBullet;
                newBullet.x = playerShape.getPosition().x;
                newBullet.y = playerShape.getPosition().y;
                newBullet.vx = playerDirection.x * 10;
                newBullet.vy = playerDirection.y * 10;

                // Send the new bullet to the server
                if (socket.send(&newBullet, sizeof(newBullet)) != sf::Socket::Done) {
                    std::cerr << "Error sending bullet to server" << std::endl;
                }
            }
        }

        window.clear();

        // Draw all players
        for (const Player& player : players) {
            playerShape.setPosition(player.x, player.y);
            playerShape.setFillColor(player.id == id ? sf::Color::Blue : sf::Color::Red);
            window.draw(playerShape);
        }

        window.display();
    }

    // Join the receive thread
    receiveThread.join();

    return 0;
}