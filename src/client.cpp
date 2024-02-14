#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>

//const std::string CAP_PRE = "5t4rt";
const std::string CAP_POST = "5t0p";


enum class GameState {
    Joining,
    Playing,
    Disconnecting
};

class Wall {
public:
    Wall(float x, float y, float width, float height, const sf::Color& color) : wallShape(sf::Vector2f(width, height)) {
        wallShape.setPosition(x, y);
        wallShape.setFillColor(color);
    }

    const sf::RectangleShape& getShape() const {
        return wallShape;
    }

private:
    sf::RectangleShape wallShape;
};

class Bullet {
public:
    Bullet(float x, float y, float velocityX, float velocityY) : bulletShape(5), bulletVelocity(velocityX, velocityY) {
        bulletShape.setFillColor(sf::Color::Blue);
        bulletShape.setPosition(x, y);
    }

    void move() {
        bulletShape.move(bulletVelocity);
    }

    const sf::CircleShape& getShape() const {
        return bulletShape;
    }

    bool checkCollision(const Wall& wall) const {
        return bulletShape.getGlobalBounds().intersects(wall.getShape().getGlobalBounds());
    }

    // NETWORKING ---------------------------------------------------------------
    nlohmann::json toJson() {
        return {
            {"type", "bullet"},
            {"position", {bulletShape.getPosition().x, bulletShape.getPosition().y}},
            {"velocity", {bulletVelocity.x, bulletVelocity.y}}
        };
    }

    void sendBulletToServer(sf::TcpSocket& socket) {
        if (socket.getRemoteAddress() == sf::IpAddress::None) {
            std::cout << "Not connected to server in send bullet" << std::endl;
            return;
        }
        else{
            //convert bullet data to json
            nlohmann::json bulletJson = toJson();
            //send json to server
            std::string bulletJsonMsg = bulletJson.dump() + CAP_POST;
            if (socket.send(bulletJsonMsg.c_str(), bulletJsonMsg.size()) != sf::Socket::Done) {
                std::cout << "Error sending json bullet data to server" << std::endl;
                return;
            }
        }
    }

    //--------------------------------------------------------------------------

private:
    sf::CircleShape bulletShape;
    sf::Vector2f bulletVelocity;
};

class Player {
public:
    Player() : playerShape(sf::Vector2f(50, 50)), playerVelocity(5.0f) {
        playerShape.setFillColor(sf::Color::Green);
        playerShape.setPosition(400, 300);
        playerShape.getPosition();
    }

    void move(float offsetX, float offsetY) {
        playerShape.move(offsetX * playerVelocity, offsetY * playerVelocity);
    }

    const sf::RectangleShape& getShape() const {
        return playerShape;
    }

    void shoot(float velocityX, float velocityY, sf::TcpSocket& socket) {
        if (shootClock.getElapsedTime().asSeconds() >= shootCooldown){
            // bullets.emplace_back(playerShape.getPosition().x + playerShape.getSize().x / 2,
            //                     playerShape.getPosition().y + playerShape.getSize().y / 2,
            //                     velocityX, velocityY);
        Bullet bullet(playerShape.getPosition().x + playerShape.getSize().x / 2,
                                playerShape.getPosition().y + playerShape.getSize().y / 2,
                                velocityX, velocityY);
            bullet.sendBulletToServer(socket);
            shootClock.restart();
        }
    }

    bool checkCollision(const Wall& wall) const {
        return playerShape.getGlobalBounds().intersects(wall.getShape().getGlobalBounds());
    }

    sf::Vector2f getPosition() {
        return playerShape.getPosition();
    }

    void setPosition(sf::Vector2f position) {
        playerShape.setPosition(position);
    }

    void setFillColor(sf::Color color) {
        playerShape.setFillColor(color);
    }

    void restartShootClock() {
        shootClock.restart();
    }

    // NETWORKING ---------------------------------------------------------------
    nlohmann::json toJson() {
        return {
            {"type", "player"},
            {"position", {playerShape.getPosition().x, playerShape.getPosition().y}}
        };
    }

    void sendPlayerToServer(sf::TcpSocket& socket) {
        if (socket.getRemoteAddress() == sf::IpAddress::None) {
            std::cout << "Not connected to server in send player" << std::endl;
            return;
        }
        else{
            // Convert player data to json
            nlohmann::json playerJson = toJson();
            // Send json to server
            std::string playerJsonMsg = playerJson.dump() + CAP_POST;
            if (socket.send(playerJsonMsg.c_str(), playerJsonMsg.size()) != sf::Socket::Done) {
                std::cout << "Error sending json player data to server" << std::endl;
                return;
            }
        }
    }

    //--------------------------------------------------------------------------

private:
    sf::RectangleShape playerShape;
    float playerVelocity;

    sf::Clock shootClock;
    float shootCooldown = 0.5f;
};

class Game {
public:
    Game() : window(sf::VideoMode(800, 600), "2D Client") {
        window.setFramerateLimit(30);
        gameState = GameState::Joining;
    }

    void run() {
        while (window.isOpen()) {

            switch (gameState) {
            case GameState::Joining:
                if (connectToServer()) {
                    std::cout << "Connected to server (joining)" << std::endl;
                    recieveMapFromServer();
                    std::cout << "Received map from server" << std::endl;

                    gameState = GameState::Playing;
                    break;
                }

            case GameState::Playing:
                socket.setBlocking(false);
                processEvents();
                update();
                render();
                break;

            case GameState::Disconnecting:
                std::cout << "GameState Disconnecting" << std::endl;
                disconnectFromServer();
                return;
            }
            
        }
    }
    
    // NETWORKING ---------------------------------------------------------------
    sf::TcpSocket& getSocket() {
        return socket;
    }
    
    bool connectToServer() {
        //connect to server
        sf::Socket::Status status = socket.connect("127.0.0.1", 5000);
        if (status != sf::Socket::Done) {
            std::cout << "Error connecting to server" << std::endl;
            return false;
        }
        return true;
    }

    void sendDataToServer() {
        if (socket.getRemoteAddress() == sf::IpAddress::None) {
            std::cout << "Not connected to server in send json" << std::endl;
            return;
        }
        else{
            //convert player data to json
            nlohmann::json playerJson = player.toJson();
            //send json to server
            if (socket.send(playerJson.dump().c_str(), playerJson.dump().size()) != sf::Socket::Done) {
                std::cout << "Error sending json data to server" << std::endl;
                return;
            }
        }
    }

    void receiveDataFromServer(){
        if (socket.getRemoteAddress() == sf::IpAddress::None) {
            std::cout << "Not connected to server in receive" << std::endl;
            return;
        }
        else{
            //recieve data from server
            char buffer[1024] = {0};
            std::size_t received;
            if (socket.receive(buffer, sizeof(buffer), received) != sf::Socket::Done) {
                return;
            }
            
            // Handle the received data
            // Decapsulate the data and add them to list
            std::cout << "Buffer: " << buffer << std::endl;

            std::vector<std::string> dataV;
            std::string dataString = buffer;
            size_t start = 0;
            size_t end = 0;

            std::cout << "Received data from server pre decap: " << dataString << std::endl;

            while ((end = dataString.find(CAP_POST, start)) != std::string::npos) {
                std::string data = dataString.substr(start, end - start);
                dataString = dataString.substr(end + CAP_POST.size());
                start = end + CAP_POST.size();
                dataV.push_back(data);
            }

            for (std::string d : dataV) {
                std::cout << "Received data from server cap: " << d << std::endl;
                handleReceivedData(d.c_str(), d.size());
            }
        }
    }

    void handleReceivedData(const char* data, ssize_t dataSize) {
        // Handle the received data from the server

        if (dataSize == 4 && strncmp(data, "pong", 4) == 0) {
            std::cout << "Received ping from server" << std::endl;
            recievedPongFromServer();
            return;
        }

        std::cout << "Received data from server: " << data << std::endl;
        nlohmann::json dataJson;
        try
        {
            dataJson = nlohmann::json::parse(data);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        

        std::cout << "Received data from server (JSON): " << dataJson << std::endl;

        if (dataJson.contains("type") && dataJson["type"] == "bullet") {
            // Add the bullet to the list
            std::cout << "Received bullet from server" << std::endl;
            Bullet bullet(dataJson["position"][0], dataJson["position"][1], dataJson["velocity"][0], dataJson["velocity"][1]);
            bullets.push_back(bullet);
        } else if (dataJson.contains("type") && dataJson["type"] == "player"){
            std::cout << "Received player from server" << std::endl;
            Player player;
            player.setPosition(sf::Vector2f(dataJson["position"][0], dataJson["position"][1]));
            player.setFillColor(sf::Color::Red);
            players.push_back(player);
        }
    }

    void pingServer() {
        std::string pingMsg = "ping" + CAP_POST;
        std::cout << "PingMsg: " << pingMsg << std::endl;
        if (socket.getRemoteAddress() == sf::IpAddress::None) {
            std::cout << "Not connected to server in pingServer" << std::endl;
            return;
        }
        else{
            pingClock.restart();
            if (socket.send(pingMsg.c_str(), pingMsg.size()) != sf::Socket::Done) {
                std::cout << "Error sending ping to server" << std::endl;
                return;
            }
        }
    }

    void recievedPongFromServer() {
        disconnectClock.restart();
    }

    void disconnectFromServer() {
        socket.disconnect();
        std::cout << "Disconnected from server (disconnectFromServer function)" << std::endl;
    }
    
    //---------------------------------------------------------------------------

private:
    GameState gameState;
    sf::Clock disconnectClock;
    sf::Clock pingClock;
    

    void recieveMapFromServer(){
        if (socket.getRemoteAddress() == sf::IpAddress::None) {
            std::cout << "Not connected to server in receive map" << std::endl;
            return;
        }
        else{
            //recieve data from server
            char buffer[1024] = {0};
            std::size_t received;
            if (socket.receive(buffer, sizeof(buffer), received) != sf::Socket::Done) {
                std::cout << "Error receiving json map data from server" << std::endl;
                gameState = GameState::Disconnecting;
                return;
            }
            buffer[received] = '\0';
            //parse buffer to json
            nlohmann::json mapJson = nlohmann::json::parse(buffer);

            floor.setFillColor(sf::Color(mapJson["floor"]["color"]["r"], mapJson["floor"]["color"]["g"], mapJson["floor"]["color"]["b"]));
            floor.setPosition(mapJson["floor"]["position"]["x"], mapJson["floor"]["position"]["y"]);
            floor.setSize(sf::Vector2f(mapJson["floor"]["dimensions"]["width"], mapJson["floor"]["dimensions"]["height"]));

            for (int i = 0; i < mapJson["walls"].size(); i++){
                if (!mapJson["walls"][i].is_object() || !mapJson["walls"][i]["position"].is_object() || !mapJson["walls"][i]["dimensions"].is_object() || !mapJson["walls"][i]["color"].is_object()) {
                    std::cout << "Invalid JSON structure in walls array" << std::endl;
                    //std::cout << mapJson["walls"][i] << std::endl;
                    gameState = GameState::Disconnecting;
                    return;
                }
                walls.emplace_back(mapJson["walls"][i]["position"]["x"], mapJson["walls"][i]["position"]["y"], mapJson["walls"][i]["dimensions"]["width"], mapJson["walls"][i]["dimensions"]["height"], sf::Color(mapJson["walls"][i]["color"]["r"], mapJson["walls"][i]["color"]["g"], mapJson["walls"][i]["color"]["b"]));
            }
        }
    }

    void processEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Store the player's previous position before moving
        sf::Vector2f prevPosition = player.getPosition();

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && player.getShape().getPosition().y > 0){
            player.move(0, -2);
            player.sendPlayerToServer(socket);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && player.getShape().getPosition().y < window.getSize().y - player.getShape().getSize().y){
            player.move(0, 2);
            player.sendPlayerToServer(socket);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && player.getShape().getPosition().x > 0){
            player.move(-2, 0);
            player.sendPlayerToServer(socket);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && player.getShape().getPosition().x < window.getSize().x - player.getShape().getSize().x){
            player.move(2, 0);
            player.sendPlayerToServer(socket);
        }

        // Check for collisions with walls
        for (const auto& wall : walls) {
            if (player.checkCollision(wall)) {
                //can't move into the wall
                player.setPosition(prevPosition);
                break;
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)){
            player.shoot(0, -20, socket);
            player.sendPlayerToServer(socket);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)){
            player.shoot(0, 20, socket);
            player.sendPlayerToServer(socket);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)){
            player.shoot(-20, 0, socket);
            player.sendPlayerToServer(socket);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)){
            player.shoot(20, 0, socket);
            player.sendPlayerToServer(socket);

        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
            gameState = GameState::Disconnecting;
        }
    }

    void update() {
        receiveDataFromServer();
        // Update bullets
        for (auto& bullet : bullets) {
            bullet.move();

            // Check for collisions with walls
            for (const auto& wall : walls) {
                if (bullet.checkCollision(wall)) {
                    bullet = bullets.back();
                    bullets.pop_back();
                }
            }
        }
        // Remove bullets that are out of the screen
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& bullet) {
            return bullet.getShape().getPosition().x < 0 || bullet.getShape().getPosition().x > 800 ||
                bullet.getShape().getPosition().y < 0 || bullet.getShape().getPosition().y > 600;
        }), bullets.end());

        if (pingClock.getElapsedTime().asSeconds() >= 1) {
            pingServer();
        }

        if (disconnectClock.getElapsedTime().asSeconds() >= 3) {
            gameState = GameState::Disconnecting;
        }
    }

    void render() {
        window.clear();

        window.draw(floor);
        for (const auto& wall : walls) {
            window.draw(wall.getShape());
        }

        window.draw(player.getShape());
        // Draw bullets
        for (const auto& bullet : bullets) {
            window.draw(bullet.getShape());
        }

        for (auto& player : players) {
            window.draw(player.getShape());
        }

        window.display();
    }

    sf::RenderWindow window;
    Player player;
    std::vector<Bullet> bullets;
    std::vector<Player> players;
    std::vector<Wall> walls;
    sf::RectangleShape floor;

    sf::TcpSocket socket;
};

int main() {
    Game game;
    game.run();

    return 0;
}