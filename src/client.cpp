    #include <SFML/Network.hpp>
    #include <SFML/Graphics.hpp>
    #include <iostream>
    #include <nlohmann/json.hpp>

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
                if (socket.send(bulletJson.dump().c_str(), bulletJson.dump().size()) != sf::Socket::Done) {
                    std::cout << "Error sending json bullet data to server" << std::endl;
                    return;
                }
            }
        }

        void receiveBulletFromServer(sf::TcpSocket& socket){
            if (socket.getRemoteAddress() == sf::IpAddress::None) {
                std::cout << "Not connected to server in receive bullet" << std::endl;
                return;
            }
            else{
                //recieve data from server
                char buffer[1024] = {0};
                std::size_t received;
                if (socket.receive(buffer, sizeof(buffer), received) != sf::Socket::Done) {
                    std::cout << "Error receiving json bullet data from server" << std::endl;
                    return;
                }
                //print parsed json
                nlohmann::json receivedBulletJson = nlohmann::json::parse(buffer);
                std::cout << "Parsed: " << receivedBulletJson << std::endl;
                
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
            playerShape.setPosition(375, 275);
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

        void restartShootClock() {
            shootClock.restart();
        }

        // NETWORKING ---------------------------------------------------------------
        nlohmann::json toJson() {
            return {
                {"type", "player"},
                {"position", {playerShape.getPosition().x, playerShape.getPosition().y}},
                {"velocity", {playerVelocity}}
            };
        }
        //---------------------------------------------------------------------------

    private:
        sf::RectangleShape playerShape;
        float playerVelocity;

        sf::Clock shootClock;
        float shootCooldown = 0.5f;
    };

    class Game {
    public:
        Game() : window(sf::VideoMode(800, 600), "2D Client") {
            window.setFramerateLimit(60);
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
                char buffer[1024];
                std::size_t received;
                if (socket.receive(buffer, sizeof(buffer), received) != sf::Socket::Done) {
                    std::cout << "Error receiving json data from server" << std::endl;
                    return;
                }
                std::cout << "Received: " << buffer << std::endl;

                if (received == 4 && strncmp(buffer, "pong", 4) == 0) {
                    std::cout << "Received pong from server" << std::endl;
                    recievedPongFromServer();
                    return;
                    
                } else {
                    //parsed json
                    nlohmann::json receivedJson = nlohmann::json::parse(buffer);

                    if (receivedJson["type"] == "bullet") {
                        std::cout << "Received bullet" << std::endl;
                        Bullet bullet(receivedJson["position"][0], receivedJson["position"][1], receivedJson["velocity"][0], receivedJson["velocity"][1]);
                        bullets.push_back(bullet);
                    }
                }

                
            }
        }

        void pingServer() {
            if (socket.getRemoteAddress() == sf::IpAddress::None) {
                std::cout << "Not connected to server in pingServer" << std::endl;
                return;
            }
            else{
                pingClock.restart();
                if (socket.send("ping", 4) != sf::Socket::Done) {
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

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && player.getShape().getPosition().y > 0)
                player.move(0, -1);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && player.getShape().getPosition().y < window.getSize().y - player.getShape().getSize().y)
                player.move(0, 1);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && player.getShape().getPosition().x > 0)
                player.move(-1, 0);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && player.getShape().getPosition().x < window.getSize().x - player.getShape().getSize().x)
                player.move(1, 0);

            // Check for collisions with walls
            for (const auto& wall : walls) {
                if (player.checkCollision(wall)) {
                    //can't move into the wall
                    player.setPosition(prevPosition);
                    break;
                }
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                player.shoot(0, -10, socket);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                player.shoot(0, 10, socket);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
                player.shoot(-10, 0, socket);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                player.shoot(10, 0, socket);

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

            window.display();
        }

        sf::RenderWindow window;
        Player player;
        std::vector<Bullet> bullets;
        std::vector<Wall> walls;
        sf::RectangleShape floor;

        sf::TcpSocket socket;
    };

    int main() {
        Game game;
        game.run();

        return 0;
    }