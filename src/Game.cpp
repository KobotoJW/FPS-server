#include "Game.h"

const std::string CAP_POST = "5t0p";


Game::Game() : window(sf::VideoMode(800, 600), "2D Client") {
    window.setFramerateLimit(30);
    gameState = GameState::Joining;
}

void Game::run() {
    while (window.isOpen()) {

        switch (gameState) {
        case GameState::Joining:
            if (connectToServer()) {
                std::cout << "Connected to server (joining)" << std::endl;
                recieveMapFromServer();
                std::cout << "Received map from server" << std::endl;
                player.getPlayerIdFromServer(socket);
                std::cout << "Requested player id from server" << std::endl;

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
sf::TcpSocket& Game::getSocket() {
    return socket;
}

bool Game::connectToServer() {
    //connect to server
    sf::Socket::Status status = socket.connect("127.0.0.1", 5000);
    if (status != sf::Socket::Done) {
        std::cout << "Error connecting to server" << std::endl;
        return false;
    }
    return true;
}

void Game::receiveDataFromServer(){
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
        //std::cout << "Buffer: " << buffer << std::endl;

        std::vector<std::string> dataV;
        std::string dataString = buffer;
        size_t start = 0;
        size_t end = 0;

        //std::cout << "Received data from server pre decap: " << dataString << std::endl;

        while ((end = dataString.find(CAP_POST, start)) != std::string::npos) {
            std::string data = dataString.substr(start, end - start);
            dataString = dataString.substr(end + CAP_POST.size());
            start = end + CAP_POST.size();
            dataV.push_back(data);
        }

        for (std::string d : dataV) {
            //std::cout << "Received data from server cap: " << d << std::endl;
            handleReceivedData(d.c_str(), d.size());
        }
    }
}

void Game::handleReceivedData(const char* data, ssize_t dataSize) {
    // Handle the received data from the server

    if (dataSize == 4 && strncmp(data, "pong", 4) == 0) {
        //std::cout << "Received ping from server" << std::endl;
        recievedPongFromServer();
        return;
    }

    //std::cout << "Received data from server: " << data << std::endl;
    nlohmann::json dataJson;
    try
    {
        dataJson = nlohmann::json::parse(data);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    

    //std::cout << "Received data from server (JSON): " << dataJson << std::endl;

    // Bullets
    if (dataJson.contains("type") && dataJson["type"] == "bullet") {
        // Add the bullet to the list
        std::cout << "Received bullet from server" << std::endl;
        Bullet bullet(dataJson["position"][0], dataJson["position"][1], dataJson["velocity"][0], dataJson["velocity"][1]);
        bullets.push_back(bullet);
    } 
    // Players
    else if (dataJson.contains("type") && dataJson["type"] == "player") {
        int recievedId = dataJson["id"];
        std::cout << "Received player from server, id: " << recievedId << std::endl;
        if (recievedId == player.getPlayerId()) {
            return;
        }

        for (auto& player : players) {
            if (player.getPlayerId() == recievedId) {
                player.setPosition(sf::Vector2f(dataJson["position"][0], dataJson["position"][1]));
                return;
            }
        }
        std::cout << "Creating new player" << std::endl;
        Player newPlayer;
        newPlayer.setPosition(sf::Vector2f(dataJson["position"][0], dataJson["position"][1]));
        newPlayer.setPlayerId(recievedId);
        newPlayer.setFillColor(sf::Color::Red);
        players.push_back(newPlayer);
        players.back().setPlayerIdText();
    } 
    
    else if (dataJson.contains("type") && dataJson["type"] == "playerId") {
        std::cout << "Received player id from server" << std::endl;
        player.setPlayerId(dataJson["id"]);
        player.setPlayerIdText();
    }
}

void Game::pingServer() {
    size_t sent;
    std::string pingMsg = "ping" + CAP_POST;
    //std::cout << "PingMsg: " << pingMsg << std::endl;
    if (socket.getRemoteAddress() == sf::IpAddress::None) {
        std::cout << "Not connected to server in pingServer" << std::endl;
        return;
    }
    else{
        pingClock.restart();
        if (socket.send(pingMsg.c_str(), pingMsg.size(), sent) != sf::Socket::Done) {
            std::cout << "Error sending ping to server" << std::endl;
            return;
        }
    }
}

void Game::recievedPongFromServer() {
    disconnectClock.restart();
}

void Game::disconnectFromServer() {
    socket.disconnect();
    std::cout << "Disconnected from server (disconnectFromServer function)" << std::endl;
}

//---------------------------------------------------------------------------


GameState gameState;
sf::Clock disconnectClock;
sf::Clock pingClock;


void Game::recieveMapFromServer(){
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

void Game::processEvents() {
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
        //player.sendPlayerToServer(socket);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)){
        player.shoot(0, 20, socket);
        //player.sendPlayerToServer(socket);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)){
        player.shoot(-20, 0, socket);
        //player.sendPlayerToServer(socket);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)){
        player.shoot(20, 0, socket);
        //player.sendPlayerToServer(socket);

    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
        gameState = GameState::Disconnecting;
    }
}

void Game::update() {
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

void Game::render() {
    window.clear();

    window.draw(floor);
    for (const auto& wall : walls) {
        window.draw(wall.getShape());
    }

    window.draw(player.getShape());
    window.draw(player.getIdText());

    // Draw bullets
    for (const auto& bullet : bullets) {
        window.draw(bullet.getShape());
    }
    // Draw players
    for (auto& player : players) {
        window.draw(player.getShape());
        window.draw(player.getIdText());
    }

    window.display();
}