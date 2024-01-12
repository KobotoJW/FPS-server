#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <queue>
#include <mutex>

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

    Player(float x, float y) {
        shape.setSize(sf::Vector2f(50, 50));
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color::Green);
    }

    void update() {
        shape.move(velocity);
    }
};

void networkThread(std::string serverIp, int port) {
    std::thread networkThread([serverIp, port]() {
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            std::cerr << "Error creating socket" << std::endl;
            return 1;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);

        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            std::cerr << "Error connecting to server" << std::endl;
            close(clientSocket);
            return 1;
        }

        std::cout << "Connected to server" << std::endl;

        while (true) {
            char buffer[1024];
            // Receive and display the server's response
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0) {
                std::cerr << "Server disconnected" << std::endl;
                break;
            }
            messageQueue.push(buffer);
        }
        return 0;
    });
    networkThread.detach();
}



int main(int argc, char** argv) {

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server-ip> <port>" << std::endl;
        return 1;
    }

    std::string serverIp = argv[1];
    int port = std::stoi(argv[2]);

    GameState state = GameState::ServerConnection;

    std::string ipAddress;
    sf::Text ipText;
    sf::Font font;

    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Error loading font" << std::endl;
        return 1;
    }

    ipText.setFont(font);
    ipText.setCharacterSize(24); // in pixels, not points!
    ipText.setFillColor(sf::Color::White);
    ipText.setPosition(10, 10); // Optional: position the text

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
            printf("Server connection screen\n");
            window.clear();

            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::KeyPressed) {
                    // Check if the key code corresponds to a printable character
                    if (event.key.code >= sf::Keyboard::A && event.key.code <= sf::Keyboard::Z) {
                        // Convert the key code to a character and append it to ipAddress
                        char character = 'A' + (event.key.code - sf::Keyboard::A);
                        ipAddress += character;
                        ipText.setString("IP Address: " + ipAddress);
                        std::cout << "Entered character: " << character << '\n';
                        std::cout << "Current IP address: " << ipAddress << '\n';
                    }
                }
            }

            window.draw(ipText);
            window.display();

            // Handle user input
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
                printf("Connecting to %s\n", ipAddress.c_str());
                state = GameState::Playing;
            }
        }
    }
    return 0;
}
