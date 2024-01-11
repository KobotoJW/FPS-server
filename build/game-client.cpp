#include <SFML/Graphics.hpp>

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

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Simple Shooter Game");
    window.setFramerateLimit(60);

    Player player(400, 300);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

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
    }

    return 0;
}
