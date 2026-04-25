#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <queue>
#include <climits>
#include <sstream>
#include <string>
#include <cmath>
#include <set>
#include <tuple>

using namespace std;

// ============================================
// BUSINESS LOGIC LAYER
// ============================================

// Product structure (BST node)
struct Product
{
    int id;
    string name;
    double price;
    Product* left;
    Product* right;

    Product(int id, const string& name, double price)
        : id(id), name(name), price(price), left(nullptr), right(nullptr) {}
};

// Cart item structure
struct CartItem
{
    int id;
    string name;
    double price;
};

// Business logic class
class ECartSystem
{
private:
    unordered_map<string, string> users;
    Product* productRoot;
    vector<CartItem> cart;
    vector<pair<int, int>> graph[100];
    bool loggedIn;
    string currentUser;

    Product* insertProduct(Product* root, int id, const string& name, double price)
    {
        if (root == nullptr)
            return new Product(id, name, price);

        if (price < root->price)
            root->left = insertProduct(root->left, id, name, price);
        else
            root->right = insertProduct(root->right, id, name, price);

        return root;
    }

    Product* searchProduct(Product* root, int id)
    {
        if (root == nullptr)
            return nullptr;

        if (root->id == id)
            return root;

        Product* left = searchProduct(root->left, id);
        if (left != nullptr)
            return left;

        return searchProduct(root->right, id);
    }

    void getPriceFilterProducts(Product* root, double low, double high, vector<Product*>& results)
    {
        if (root == nullptr)
            return;

        if (root->price >= low && root->price <= high)
            results.push_back(root);

        getPriceFilterProducts(root->left, low, high, results);
        getPriceFilterProducts(root->right, low, high, results);
    }

    void getAllProducts(Product* root, vector<Product*>& results)
    {
        if (root == nullptr)
            return;

        getAllProducts(root->left, results);
        results.push_back(root);
        getAllProducts(root->right, results);
    }

public:
    ECartSystem() : productRoot(nullptr), loggedIn(false), currentUser("") {}

    void loadUsers()
    {
        ifstream file("users.txt");
        if (!file.is_open())
            return;

        string username, password;
        while (file >> username >> password)
            users[username] = password;

        file.close();
    }

    void loadCatalog()
    {
        ifstream file("catalog.txt");
        if (!file.is_open())
            return;

        int id;
        string name;
        double price;

        while (file >> id >> name >> price)
            productRoot = insertProduct(productRoot, id, name, price);

        file.close();
    }

    // Authentication methods
    pair<bool, string> registerUser(const string& username, const string& password)
    {
        if (username.empty() || password.empty())
            return {false, "Username and password cannot be empty!"};

        if (users.find(username) != users.end())
            return {false, "User already exists!"};

        users[username] = password;

        ofstream file("users.txt", ios::app);
        file << username << " " << password << endl;
        file.close();

        return {true, "Registration successful!"};
    }

    pair<bool, string> loginUser(const string& username, const string& password)
    {
        auto it = users.find(username);

        if (it != users.end() && it->second == password)
        {
            loggedIn = true;
            currentUser = username;
            return {true, "Login successful!"};
        }

        return {false, "Invalid username or password!"};
    }

    void logout()
    {
        loggedIn = false;
        currentUser = "";
        cart.clear();
    }

    bool isLoggedIn() const { return loggedIn; }
    string getCurrentUser() const { return currentUser; }

    // Product methods
    vector<Product*> getAllProducts()
    {
        vector<Product*> result;
        getAllProducts(productRoot, result);
        return result;
    }

    Product* getProduct(int id)
    {
        return searchProduct(productRoot, id);
    }

    vector<Product*> getFilteredProducts(double low, double high)
    {
        vector<Product*> result;
        getPriceFilterProducts(productRoot, low, high, result);
        return result;
    }

    // Cart methods
    pair<bool, string> addToCart(int productId)
    {
        if (!loggedIn)
            return {false, "You must login first!"};

        Product* p = getProduct(productId);
        if (!p)
            return {false, "Product not found!"};

        cart.push_back({p->id, p->name, p->price});
        return {true, "Product added to cart!"};
    }

    vector<CartItem> getCart() const { return cart; }

    double getCartTotal() const
    {
        double total = 0;
        for (const auto& item : cart)
            total += item.price;
        return total;
    }

    void clearCart()
    {
        cart.clear();
    }

    void removeFromCart(int index)
    {
        if (index >= 0 && index < (int)cart.size())
            cart.erase(cart.begin() + index);
    }

    // Routing methods (Dijkstra)
    void addRoute(int u, int v, int w)
    {
        graph[u].push_back({v, w});
        graph[v].push_back({u, w});
    }

    pair<bool, pair<int, vector<int>>> findShortestPath(int src, int dest, int n)
    {
        vector<int> dist(n, INT_MAX);
        vector<int> parent(n, -1);

        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;

        dist[src] = 0;
        pq.push({0, src});

        while (!pq.empty())
        {
            int u = pq.top().second;
            pq.pop();

            for (auto edge : graph[u])
            {
                int v = edge.first;
                int w = edge.second;

                if (dist[v] > dist[u] + w)
                {
                    dist[v] = dist[u] + w;
                    parent[v] = u;
                    pq.push({dist[v], v});
                }
            }
        }

        if (dist[dest] == INT_MAX)
            return {false, {-1, {}}};

        vector<int> path;
        for (int v = dest; v != -1; v = parent[v])
            path.push_back(v);

        return {true, {dist[dest], path}};
    }
};

// ============================================
// GUI LAYER - SFML Components
// ============================================

// Button class for SFML
class Button
{
public:
    sf::RectangleShape shape;
    sf::Text text;
    bool hovered;

    Button(float x, float y, float width, float height, const string& label, sf::Font& font)
        : hovered(false)
    {
        shape.setPosition(x, y);
        shape.setSize({width, height});
        shape.setFillColor(sf::Color(70, 130, 180));
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(2);

        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(16);
        text.setFillColor(sf::Color::White);

        // Center text in button
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition(
            x + (width - textBounds.width) / 2,
            y + (height - textBounds.height) / 2 - 5
        );
    }

    bool isClicked(sf::RenderWindow& window)
    {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        return shape.getGlobalBounds().contains(mousePos);
    }

    bool isHovered(sf::RenderWindow& window)
    {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        hovered = shape.getGlobalBounds().contains(mousePos);

        if (hovered)
            shape.setFillColor(sf::Color(100, 150, 200));
        else
            shape.setFillColor(sf::Color(70, 130, 180));

        return hovered;
    }

    void draw(sf::RenderWindow& window)
    {
        window.draw(shape);
        window.draw(text);
    }
};

// Input field class for SFML
class InputField
{
public:
    sf::RectangleShape shape;
    sf::Text text;
    string input;
    bool active;
    sf::Font& font;

    InputField(float x, float y, float width, float height, sf::Font& fnt)
        : active(false), font(fnt), input("")
    {
        shape.setPosition(x, y);
        shape.setSize({width, height});
        shape.setFillColor(sf::Color::White);
        shape.setOutlineColor(sf::Color::Black);
        shape.setOutlineThickness(2);

        text.setFont(font);
        text.setCharacterSize(16);
        text.setFillColor(sf::Color::Black);
        text.setPosition(x + 5, y + 5);
    }

    void handleEvent(sf::Event& event)
    {
        if (event.type == sf::Event::MouseButtonPressed)
        {
            active = shape.getGlobalBounds().contains(
                sf::Vector2f(event.mouseButton.x, event.mouseButton.y)
            );
        }

        if (active && event.type == sf::Event::TextEntered)
        {
            if (event.text.unicode == '\b' && !input.empty())
                input.pop_back();
            else if (event.text.unicode >= 32 && event.text.unicode < 127)
                input += static_cast<char>(event.text.unicode);

            text.setString(input);
        }
    }

    void draw(sf::RenderWindow& window)
    {
        window.draw(shape);
        window.draw(text);
    }

    void clear()
    {
        input = "";
        text.setString("");
    }
};

// GUI Manager class
class GUIManager
{
private:
    sf::RenderWindow window;
    sf::Font font;
    ECartSystem& system;
    
    // Current screen state
    enum Screen
    {
        MAIN_MENU,
        LOGIN,
        REGISTER,
        PRODUCTS,
        SEARCH,
        PRICE_FILTER,
        FILTER_RESULTS,
        CART,
        ADD_TO_CART,
        DELIVERY_ROUTES,
        ADD_ROUTE_SCREEN,
        SHORTEST_PATH,
        MESSAGE_BOX
    };

    Screen currentScreen;
    Screen previousScreen;  // Track previous screen for MESSAGE_BOX
    string messageTitle;
    string messageContent;
    sf::Clock messageClock;

    // Search and Route results
    Product* searchResult;
    bool searchFound;
    InputField* srcInput;
    InputField* destInput;
    InputField* distanceInput;
    vector<int> routePath;
    int routeDistance;
    bool routeFound;
    vector<vector<pair<int, int>>> adjacencyList;  // For storing graph edges

    // Route building inputs
    InputField* fromNodeInput;
    InputField* toNodeInput;
    InputField* edgeDistanceInput;
    vector<tuple<int, int, int>> userDefinedEdges;  // User entered edges: (from, to, distance)
    int routeSourceNode;
    int routeDestNode;
    int routeTotalNodes;
    int edgeCount;

    // Filter results
    vector<Product*> filteredProducts;
    bool filterApplied;

    // Input fields
    InputField* usernameInput;
    InputField* passwordInput;
    InputField* productIdInput;
    InputField* minPriceInput;
    InputField* maxPriceInput;
    InputField* routeInput;

public:
    GUIManager(ECartSystem& sys)
        : window(sf::VideoMode(1000, 700), "E-Cart System - SFML GUI"),
          system(sys),
          currentScreen(MAIN_MENU),
          previousScreen(MAIN_MENU),
          usernameInput(nullptr),
          passwordInput(nullptr),
          productIdInput(nullptr),
          minPriceInput(nullptr),
          maxPriceInput(nullptr),
          srcInput(nullptr),
          destInput(nullptr),
          distanceInput(nullptr),
          searchResult(nullptr),
          searchFound(false),
          routeDistance(0),
          routeFound(false),
          filterApplied(false),
          fromNodeInput(nullptr),
          toNodeInput(nullptr),
          edgeDistanceInput(nullptr),
          routeSourceNode(-1),
          routeDestNode(-1),
          routeTotalNodes(0),
          edgeCount(0),
          routeInput(nullptr)
    {
        window.setFramerateLimit(60);

        // Load font
        if (!font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf"))
        {
            cerr << "Error loading font! Trying alternative path...\n";
            // Alternative: use a system font path
        }

        // Initialize input fields
        usernameInput = new InputField(300, 200, 400, 40, font);
        passwordInput = new InputField(300, 260, 400, 40, font);
        productIdInput = new InputField(300, 250, 400, 40, font);
        minPriceInput = new InputField(300, 200, 400, 40, font);
        maxPriceInput = new InputField(300, 260, 400, 40, font);
        routeInput = new InputField(300, 250, 400, 40, font);
        srcInput = new InputField(250, 200, 300, 40, font);
        destInput = new InputField(250, 270, 300, 40, font);
        distanceInput = new InputField(250, 340, 300, 40, font);
        fromNodeInput = new InputField(250, 200, 150, 40, font);
        toNodeInput = new InputField(250, 260, 150, 40, font);
        edgeDistanceInput = new InputField(250, 320, 150, 40, font);
    }

    ~GUIManager()
    {
        delete usernameInput;
        delete passwordInput;
        delete productIdInput;
        delete minPriceInput;
        delete maxPriceInput;
        delete routeInput;
        delete srcInput;
        delete destInput;
        delete distanceInput;
        delete fromNodeInput;
        delete toNodeInput;
        delete edgeDistanceInput;
    }

    void run()
    {
        while (window.isOpen())
        {
            handleEvents();
            update();
            render();
        }
    }

private:
    void handleEvents()
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (currentScreen == LOGIN)
            {
                usernameInput->handleEvent(event);
                passwordInput->handleEvent(event);
            }
            else if (currentScreen == REGISTER)
            {
                usernameInput->handleEvent(event);
                passwordInput->handleEvent(event);
            }
            else if (currentScreen == ADD_TO_CART)
            {
                productIdInput->handleEvent(event);
            }
            else if (currentScreen == PRICE_FILTER)
            {
                minPriceInput->handleEvent(event);
                maxPriceInput->handleEvent(event);
            }
            else if (currentScreen == SEARCH)
            {
                productIdInput->handleEvent(event);
            }
            else if (currentScreen == DELIVERY_ROUTES)
            {
                srcInput->handleEvent(event);
                destInput->handleEvent(event);
                distanceInput->handleEvent(event);
            }
            else if (currentScreen == ADD_ROUTE_SCREEN)
            {
                fromNodeInput->handleEvent(event);
                toNodeInput->handleEvent(event);
                edgeDistanceInput->handleEvent(event);
            }
        }

        // Handle mouse clicks for buttons
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
        {
            handleButtonClicks();
        }
    }

    void handleButtonClicks()
    {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        switch (currentScreen)
        {
        case MAIN_MENU:
            // Check button clicks for main menu
            if (sf::FloatRect(200, 150, 200, 50).contains(mousePos))
                currentScreen = REGISTER;
            else if (sf::FloatRect(600, 150, 200, 50).contains(mousePos))
                currentScreen = LOGIN;
            else if (sf::FloatRect(200, 220, 200, 50).contains(mousePos))
                currentScreen = PRODUCTS;
            else if (sf::FloatRect(600, 220, 200, 50).contains(mousePos))
                currentScreen = SEARCH;
            else if (sf::FloatRect(200, 290, 200, 50).contains(mousePos))
                currentScreen = PRICE_FILTER;
            else if (sf::FloatRect(600, 290, 200, 50).contains(mousePos))
                currentScreen = ADD_TO_CART;
            else if (sf::FloatRect(200, 360, 200, 50).contains(mousePos))
                currentScreen = CART;
            else if (sf::FloatRect(600, 360, 200, 50).contains(mousePos))
                currentScreen = DELIVERY_ROUTES;
            else if (sf::FloatRect(400, 450, 200, 50).contains(mousePos))
                window.close();
            break;

        case LOGIN:
            if (sf::FloatRect(300, 330, 400, 50).contains(mousePos))
            {
                auto result = system.loginUser(usernameInput->input, passwordInput->input);
                messageTitle = result.first ? "Success" : "Error";
                messageContent = result.second;
                messageClock.restart();
                currentScreen = MESSAGE_BOX;
                usernameInput->clear();
                passwordInput->clear();
            }
            else if (sf::FloatRect(300, 400, 400, 50).contains(mousePos))
            {
                usernameInput->clear();
                passwordInput->clear();
                currentScreen = MAIN_MENU;
            }
            break;

        case REGISTER:
            if (sf::FloatRect(300, 330, 400, 50).contains(mousePos))
            {
                auto result = system.registerUser(usernameInput->input, passwordInput->input);
                messageTitle = result.first ? "Success" : "Error";
                messageContent = result.second;
                messageClock.restart();
                currentScreen = MESSAGE_BOX;
                usernameInput->clear();
                passwordInput->clear();
            }
            else if (sf::FloatRect(300, 400, 400, 50).contains(mousePos))
            {
                usernameInput->clear();
                passwordInput->clear();
                currentScreen = MAIN_MENU;
            }
            break;

        case ADD_TO_CART:
            if (sf::FloatRect(300, 330, 400, 50).contains(mousePos))
            {
                try
                {
                    int id = stoi(productIdInput->input);
                    auto result = system.addToCart(id);
                    messageTitle = result.first ? "Success" : "Error";
                    messageContent = result.second;
                    messageClock.restart();
                    currentScreen = MESSAGE_BOX;
                    productIdInput->clear();
                }
                catch (...)
                {
                    messageTitle = "Error";
                    messageContent = "Invalid product ID!";
                    messageClock.restart();
                    currentScreen = MESSAGE_BOX;
                }
            }
            else if (sf::FloatRect(300, 400, 400, 50).contains(mousePos))
            {
                productIdInput->clear();
                currentScreen = MAIN_MENU;
            }
            break;

        case SEARCH:
            if (sf::FloatRect(300, 330, 400, 50).contains(mousePos))
            {
                // SEARCH button - perform actual search
                if (!productIdInput->input.empty())
                {
                    try
                    {
                        int id = stoi(productIdInput->input);
                        searchResult = system.getProduct(id);
                        if (searchResult)
                        {
                            searchFound = true;
                        }
                        else
                        {
                            searchFound = false;
                            messageTitle = "Not Found";
                            messageContent = "Product ID not found!";
                            previousScreen = SEARCH;
                            currentScreen = MESSAGE_BOX;
                            messageClock.restart();
                        }
                    }
                    catch (...)
                    {
                        searchFound = false;
                        messageTitle = "Invalid Input";
                        messageContent = "Please enter a valid product ID!";
                        previousScreen = SEARCH;
                        currentScreen = MESSAGE_BOX;
                        messageClock.restart();
                    }
                }
                else
                {
                    messageTitle = "Error";
                    messageContent = "Please enter a product ID!";
                    previousScreen = SEARCH;
                    currentScreen = MESSAGE_BOX;
                    messageClock.restart();
                }
            }
            else if (sf::FloatRect(300, 400, 400, 50).contains(mousePos))
            {
                productIdInput->clear();
                searchFound = false;
                searchResult = nullptr;
                currentScreen = MAIN_MENU;
            }
            break;

        case PRICE_FILTER:
            if (sf::FloatRect(300, 330, 400, 50).contains(mousePos))
            {
                // Filter button - get filtered products
                if (!minPriceInput->input.empty() && !maxPriceInput->input.empty())
                {
                    try
                    {
                        double minPrice = stod(minPriceInput->input);
                        double maxPrice = stod(maxPriceInput->input);
                        
                        if (minPrice < 0 || maxPrice < 0 || minPrice > maxPrice)
                        {
                            messageTitle = "Invalid Range";
                            messageContent = "Min <= Max and both >= 0!";
                            currentScreen = MESSAGE_BOX;
                            messageClock.restart();
                        }
                        else
                        {
                            filteredProducts = system.getFilteredProducts(minPrice, maxPrice);
                            if (filteredProducts.empty())
                            {
                                messageTitle = "No Results";
                                messageContent = "No products in this price range!";
                                currentScreen = MESSAGE_BOX;
                                messageClock.restart();
                            }
                            else
                            {
                                filterApplied = true;
                                currentScreen = FILTER_RESULTS;
                            }
                        }
                    }
                    catch (...)
                    {
                        messageTitle = "Invalid Input";
                        messageContent = "Please enter valid prices!";
                        currentScreen = MESSAGE_BOX;
                        messageClock.restart();
                    }
                }
                else
                {
                    messageTitle = "Error";
                    messageContent = "Please fill all price fields!";
                    currentScreen = MESSAGE_BOX;
                    messageClock.restart();
                }
            }
            else if (sf::FloatRect(300, 400, 400, 50).contains(mousePos))
            {
                minPriceInput->clear();
                maxPriceInput->clear();
                filterApplied = false;
                filteredProducts.clear();
                currentScreen = MAIN_MENU;
            }
            break;

        case FILTER_RESULTS:
            if (sf::FloatRect(300, 600, 400, 50).contains(mousePos))
            {
                filterApplied = false;
                filteredProducts.clear();
                minPriceInput->clear();
                maxPriceInput->clear();
                currentScreen = MAIN_MENU;
            }
            break;

        case DELIVERY_ROUTES:
            if (sf::FloatRect(300, 420, 400, 50).contains(mousePos))
            {
                // Get route parameters button
                if (!srcInput->input.empty() && !destInput->input.empty() && !distanceInput->input.empty())
                {
                    try
                    {
                        int src = stoi(srcInput->input);
                        int dest = stoi(destInput->input);
                        int numNodes = stoi(distanceInput->input);
                        
                        if (src >= 0 && dest >= 0 && numNodes > 0 && src < numNodes && dest < numNodes)
                        {
                            routeSourceNode = src;
                            routeDestNode = dest;
                            routeTotalNodes = numNodes;
                            edgeCount = 0;
                            userDefinedEdges.clear();
                            adjacencyList.clear();
                            adjacencyList.resize(numNodes);
                            currentScreen = ADD_ROUTE_SCREEN;
                        }
                        else
                        {
                            messageTitle = "Invalid Input";
                            messageContent = "Check source, destination, and node count!";
                            previousScreen = DELIVERY_ROUTES;
                            currentScreen = MESSAGE_BOX;
                            messageClock.restart();
                        }
                    }
                    catch (...)
                    {
                        messageTitle = "Invalid Input";
                        messageContent = "Please enter valid numbers!";
                        previousScreen = DELIVERY_ROUTES;
                        currentScreen = MESSAGE_BOX;
                        messageClock.restart();
                    }
                }
                else
                {
                    messageTitle = "Error";
                    messageContent = "Please fill all fields!";
                    previousScreen = DELIVERY_ROUTES;
                    currentScreen = MESSAGE_BOX;
                    messageClock.restart();
                }
            }
            else if (sf::FloatRect(300, 630, 400, 50).contains(mousePos))
            {
                srcInput->clear();
                destInput->clear();
                distanceInput->clear();
                routeFound = false;
                routePath.clear();
                adjacencyList.clear();
                currentScreen = MAIN_MENU;
            }
            break;

        case ADD_ROUTE_SCREEN:
            if (sf::FloatRect(300, 380, 400, 50).contains(mousePos))
            {
                // ADD EDGE button
                if (!fromNodeInput->input.empty() && !toNodeInput->input.empty() && !edgeDistanceInput->input.empty())
                {
                    try
                    {
                        int from = stoi(fromNodeInput->input);
                        int to = stoi(toNodeInput->input);
                        int dist = stoi(edgeDistanceInput->input);
                        
                        if (from >= 0 && to >= 0 && dist > 0 && from < routeTotalNodes && to < routeTotalNodes)
                        {
                            userDefinedEdges.push_back({from, to, dist});
                            system.addRoute(from, to, dist);
                            adjacencyList[from].push_back({to, dist});
                            adjacencyList[to].push_back({from, dist});
                            
                            edgeCount++;
                            fromNodeInput->clear();
                            toNodeInput->clear();
                            edgeDistanceInput->clear();
                            
                            messageTitle = "Edge Added";
                            messageContent = "Edge " + to_string(edgeCount) + " added successfully!";
                            previousScreen = ADD_ROUTE_SCREEN;
                            currentScreen = MESSAGE_BOX;
                            messageClock.restart();
                        }
                        else
                        {
                            messageTitle = "Invalid Edge";
                            messageContent = "Nodes must be < total nodes and distance > 0!";
                            previousScreen = ADD_ROUTE_SCREEN;
                            currentScreen = MESSAGE_BOX;
                            messageClock.restart();
                        }
                    }
                    catch (...)
                    {
                        messageTitle = "Invalid Input";
                        messageContent = "Please enter valid numbers!";
                        previousScreen = ADD_ROUTE_SCREEN;
                        currentScreen = MESSAGE_BOX;
                        messageClock.restart();
                    }
                }
                else
                {
                    messageTitle = "Error";
                    messageContent = "Please fill all edge fields!";
                    previousScreen = ADD_ROUTE_SCREEN;
                    currentScreen = MESSAGE_BOX;
                    messageClock.restart();
                }
            }
            else if (sf::FloatRect(300, 450, 400, 50).contains(mousePos))
            {
                // FIND SHORTEST PATH button
                if (edgeCount > 0 && routeSourceNode >= 0 && routeDestNode >= 0)
                {
                    auto result = system.findShortestPath(routeSourceNode, routeDestNode, routeTotalNodes);
                    if (result.first)
                    {
                        routeFound = true;
                        routeDistance = result.second.first;
                        routePath = result.second.second;
                    }
                    else
                    {
                        routeFound = false;
                        messageTitle = "No Route Found";
                        messageContent = "Cannot reach destination from source!";
                        previousScreen = ADD_ROUTE_SCREEN;
                        currentScreen = MESSAGE_BOX;
                        messageClock.restart();
                    }
                }
                else
                {
                    messageTitle = "No Edges";
                    messageContent = "Please add at least one edge!";
                    previousScreen = ADD_ROUTE_SCREEN;
                    currentScreen = MESSAGE_BOX;
                    messageClock.restart();
                }
            }
            else if (sf::FloatRect(300, 520, 400, 50).contains(mousePos))
            {
                // BACK button
                fromNodeInput->clear();
                toNodeInput->clear();
                edgeDistanceInput->clear();
                userDefinedEdges.clear();
                currentScreen = DELIVERY_ROUTES;
            }
            break;

        case MESSAGE_BOX:
            if (messageClock.getElapsedTime().asSeconds() > 2)
            {
                currentScreen = previousScreen;
            }
            break;

        case PRODUCTS:
        case CART:
            if (sf::FloatRect(400, 600, 200, 50).contains(mousePos))
                currentScreen = MAIN_MENU;
            break;

        default:
            if (sf::FloatRect(400, 600, 200, 50).contains(mousePos))
                currentScreen = MAIN_MENU;
        }
    }

    void update()
    {
        // Update logic here
    }

    void render()
    {
        window.clear(sf::Color(25, 25, 25));

        switch (currentScreen)
        {
        case MAIN_MENU:
            drawMainMenu();
            break;
        case LOGIN:
            drawLoginScreen();
            break;
        case REGISTER:
            drawRegisterScreen();
            break;
        case PRODUCTS:
            drawProductsScreen();
            break;
        case SEARCH:
            drawSearchScreen();
            break;
        case PRICE_FILTER:
            drawPriceFilterScreen();
            break;
        case FILTER_RESULTS:
            drawFilterResultsScreen();
            break;
        case CART:
            drawCartScreen();
            break;
        case ADD_TO_CART:
            drawAddToCartScreen();
            break;
        case DELIVERY_ROUTES:
            drawDeliveryRoutesScreen();
            break;
        case ADD_ROUTE_SCREEN:
            drawAddRouteScreen();
            break;
        case MESSAGE_BOX:
            drawMessageBox();
            break;
        default:
            drawMainMenu();
        }

        window.display();
    }

    // Drawing functions
    void drawMainMenu()
    {
        // Title
        sf::Text title("E-CART SYSTEM", font, 50);
        title.setFillColor(sf::Color::Yellow);
        title.setPosition(250, 30);
        window.draw(title);

        // Status bar
        sf::Text status("User: " + (system.isLoggedIn() ? system.getCurrentUser() : "Guest"), font, 14);
        status.setFillColor(sf::Color::Green);
        status.setPosition(10, 10);
        window.draw(status);

        // Button coordinates and labels
        vector<pair<sf::FloatRect, string>> buttons = {
            {{200, 150, 200, 50}, "Register"},
            {{600, 150, 200, 50}, "Login"},
            {{200, 220, 200, 50}, "Browse"},
            {{600, 220, 200, 50}, "Search"},
            {{200, 290, 200, 50}, "Filter"},
            {{600, 290, 200, 50}, "Add Cart"},
            {{200, 360, 200, 50}, "View Cart"},
            {{600, 360, 200, 50}, "Routes"},
            {{400, 450, 200, 50}, "EXIT"}
        };

        for (auto& btn : buttons)
        {
            sf::RectangleShape button(sf::Vector2f(btn.first.width, btn.first.height));
            button.setPosition(btn.first.left, btn.first.top);
            button.setFillColor(sf::Color(70, 130, 180));
            button.setOutlineColor(sf::Color::White);
            button.setOutlineThickness(2);
            window.draw(button);

            sf::Text text(btn.second, font, 16);
            text.setFillColor(sf::Color::White);
            sf::FloatRect textBounds = text.getLocalBounds();
            text.setPosition(
                btn.first.left + (btn.first.width - textBounds.width) / 2,
                btn.first.top + (btn.first.height - textBounds.height) / 2 - 5
            );
            window.draw(text);
        }
    }

    void drawLoginScreen()
    {
        drawHeader("LOGIN");

        sf::Text usernameLabel("Username:", font, 18);
        usernameLabel.setPosition(150, 210);
        usernameLabel.setFillColor(sf::Color::White);
        window.draw(usernameLabel);

        usernameInput->draw(window);

        sf::Text passwordLabel("Password:", font, 18);
        passwordLabel.setPosition(150, 270);
        passwordLabel.setFillColor(sf::Color::White);
        window.draw(passwordLabel);

        passwordInput->draw(window);

        // Login button
        drawButton(300, 330, 400, 50, "LOGIN");

        // Back button
        drawButton(300, 400, 400, 50, "BACK");
    }

    void drawRegisterScreen()
    {
        drawHeader("REGISTER");

        sf::Text usernameLabel("Username:", font, 18);
        usernameLabel.setPosition(150, 210);
        usernameLabel.setFillColor(sf::Color::White);
        window.draw(usernameLabel);

        usernameInput->draw(window);

        sf::Text passwordLabel("Password:", font, 18);
        passwordLabel.setPosition(150, 270);
        passwordLabel.setFillColor(sf::Color::White);
        window.draw(passwordLabel);

        passwordInput->draw(window);

        // Register button
        drawButton(300, 330, 400, 50, "REGISTER");

        // Back button
        drawButton(300, 400, 400, 50, "BACK");
    }

    void drawProductsScreen()
    {
        drawHeader("PRODUCT CATALOG");

        auto products = system.getAllProducts();

        int y = 100;
        sf::Text header("ID | Name | Price", font, 14);
        header.setPosition(100, y);
        header.setFillColor(sf::Color::Yellow);
        window.draw(header);

        y += 40;

        for (auto& product : products)
        {
            string info = to_string(product->id) + " | " + product->name + " | $" + to_string((int)product->price);
            sf::Text text(info, font, 14);
            text.setPosition(100, y);
            text.setFillColor(sf::Color::White);
            window.draw(text);
            y += 30;
        }

        drawButton(400, 600, 200, 50, "BACK");
    }

    void drawSearchScreen()
    {
        drawHeader("SEARCH PRODUCT");

        sf::Text idLabel("Product ID:", font, 18);
        idLabel.setPosition(150, 210);
        idLabel.setFillColor(sf::Color::White);
        window.draw(idLabel);

        productIdInput->draw(window);

        // Search button
        drawButton(300, 330, 400, 50, "SEARCH");

        // Display search result if found
        if (searchFound && searchResult)
        {
            sf::Text resultLabel("PRODUCT FOUND:", font, 20);
            resultLabel.setPosition(250, 420);
            resultLabel.setFillColor(sf::Color::Green);
            window.draw(resultLabel);

            string resultText = "ID: " + to_string(searchResult->id) + " | Name: " + searchResult->name + " | Price: " + to_string(searchResult->price);
            sf::Text resultInfo(resultText, font, 16);
            resultInfo.setPosition(150, 470);
            resultInfo.setFillColor(sf::Color::Yellow);
            window.draw(resultInfo);
        }

        // Back button
        drawButton(300, 400, 400, 50, "BACK");
    }

    void drawPriceFilterScreen()
    {
        drawHeader("FILTER BY PRICE");

        sf::Text minLabel("Min Price:", font, 18);
        minLabel.setPosition(150, 160);
        minLabel.setFillColor(sf::Color::White);
        window.draw(minLabel);

        minPriceInput->draw(window);

        sf::Text maxLabel("Max Price:", font, 18);
        maxLabel.setPosition(150, 220);
        maxLabel.setFillColor(sf::Color::White);
        window.draw(maxLabel);

        maxPriceInput->draw(window);

        drawButton(300, 330, 400, 50, "FILTER");
        drawButton(400, 600, 200, 50, "BACK");
    }

    void drawFilterResultsScreen()
    {
        drawHeader("FILTERED PRODUCTS");

        int y = 100;
        sf::Text resultHeader("ID | Name | Price", font, 14);
        resultHeader.setPosition(100, y);
        resultHeader.setFillColor(sf::Color::Yellow);
        window.draw(resultHeader);

        y += 40;
        for (const auto& product : filteredProducts)
        {
            if (product)
            {
                string productStr = to_string(product->id) + " | " + product->name + " | $" + to_string((int)product->price);
                sf::Text productText(productStr, font, 14);
                productText.setPosition(100, y);
                productText.setFillColor(sf::Color::White);
                window.draw(productText);
                y += 30;
            }
        }

        drawButton(300, 600, 400, 50, "BACK");
    }

    void drawCartScreen()
    {
        drawHeader("YOUR SHOPPING CART");

        auto cartItems = system.getCart();
        int y = 100;

        if (cartItems.empty())
        {
            sf::Text empty("Your cart is empty!", font, 18);
            empty.setPosition(350, 300);
            empty.setFillColor(sf::Color::Red);
            window.draw(empty);
        }
        else
        {
            sf::Text header("ID | Name | Price", font, 14);
            header.setPosition(100, y);
            header.setFillColor(sf::Color::Yellow);
            window.draw(header);

            y += 40;

            for (auto& item : cartItems)
            {
                string info = to_string(item.id) + " | " + item.name + " | $" + to_string((int)item.price);
                sf::Text text(info, font, 14);
                text.setPosition(100, y);
                text.setFillColor(sf::Color::White);
                window.draw(text);
                y += 30;
            }

            // Total
            double total = system.getCartTotal();
            sf::Text totalText("Total: $" + to_string((int)total), font, 18);
            totalText.setPosition(100, y + 20);
            totalText.setFillColor(sf::Color::Green);
            window.draw(totalText);
        }

        drawButton(400, 600, 200, 50, "BACK");
    }

    void drawAddToCartScreen()
    {
        drawHeader("ADD TO CART");

        if (!system.isLoggedIn())
        {
            sf::Text warning("You must login first!", font, 18);
            warning.setPosition(300, 300);
            warning.setFillColor(sf::Color::Red);
            window.draw(warning);
        }
        else
        {
            sf::Text idLabel("Product ID:", font, 18);
            idLabel.setPosition(150, 210);
            idLabel.setFillColor(sf::Color::White);
            window.draw(idLabel);

            productIdInput->draw(window);

            drawButton(300, 330, 400, 50, "ADD");
        }

        drawButton(300, 400, 400, 50, "BACK");
    }

    void drawDeliveryRoutesScreen()
    {
        drawHeader("DELIVERY ROUTES");

        sf::Text srcLabel("Source Node:", font, 18);
        srcLabel.setPosition(80, 200);
        srcLabel.setFillColor(sf::Color::White);
        window.draw(srcLabel);
        srcInput->draw(window);

        sf::Text destLabel("Destination Node:", font, 18);
        destLabel.setPosition(80, 270);
        destLabel.setFillColor(sf::Color::White);
        window.draw(destLabel);
        destInput->draw(window);

        sf::Text numLabel("Number of Nodes:", font, 18);
        numLabel.setPosition(80, 340);
        numLabel.setFillColor(sf::Color::White);
        window.draw(numLabel);
        distanceInput->draw(window);

        // Find Route button
        drawButton(300, 420, 400, 50, "FIND ROUTE");

        // Display available routes if adjacency list is populated
        if (!adjacencyList.empty() && adjacencyList.size() > 0)
        {
            sf::Text routesLabel("AVAILABLE ROUTES (Adjacency List):", font, 14);
            routesLabel.setPosition(50, 490);
            routesLabel.setFillColor(sf::Color::Cyan);
            window.draw(routesLabel);

            int y = 515;
            set<pair<int, int>> displayedEdges;
            
            for (int u = 0; u < adjacencyList.size(); u++)
            {
                for (const auto& edge : adjacencyList[u])
                {
                    int v = edge.first;
                    int w = edge.second;
                    
                    if (u <= v)  // Avoid displaying same edge twice
                    {
                        pair<int, int> edgeKey = {min(u, v), max(u, v)};
                        if (displayedEdges.find(edgeKey) == displayedEdges.end())
                        {
                            string edgeStr = to_string(u) + " <-> " + to_string(v) + " (dist: " + to_string(w) + ")";
                            sf::Text edgeText(edgeStr, font, 12);
                            edgeText.setPosition(50, y);
                            edgeText.setFillColor(sf::Color::White);
                            window.draw(edgeText);
                            displayedEdges.insert(edgeKey);
                            y += 20;
                            if (y > 600) break;  // Prevent text overflow
                        }
                    }
                }
                if (y > 600) break;
            }
        }

        // Display route result if found
        if (routeFound && !routePath.empty())
        {
            sf::Text routeLabel("SHORTEST PATH FOUND:", font, 16);
            routeLabel.setPosition(200, 50);
            routeLabel.setFillColor(sf::Color::Green);
            window.draw(routeLabel);

            string pathStr = "Path: ";
            for (int i = routePath.size() - 1; i >= 0; i--)
            {
                if (i < (int)routePath.size() - 1) pathStr += " -> ";
                pathStr += to_string(routePath[i]);
            }

            sf::Text pathInfo(pathStr, font, 14);
            pathInfo.setPosition(50, 80);
            pathInfo.setFillColor(sf::Color::Yellow);
            window.draw(pathInfo);

            string distStr = "Total Distance: " + to_string(routeDistance);
            sf::Text distInfo(distStr, font, 14);
            distInfo.setPosition(50, 110);
            distInfo.setFillColor(sf::Color::Yellow);
            window.draw(distInfo);
        }

        // Back button
        drawButton(300, 630, 400, 50, "BACK");
    }

    void drawAddRouteScreen()
    {
        drawHeader("ADD ROUTES/EDGES");

        sf::Text infoText("From -> To (Distance)", font, 16);
        infoText.setPosition(150, 140);
        infoText.setFillColor(sf::Color::Cyan);
        window.draw(infoText);

        sf::Text fromLabel("From Node:", font, 14);
        fromLabel.setPosition(100, 200);
        fromLabel.setFillColor(sf::Color::White);
        window.draw(fromLabel);
        fromNodeInput->draw(window);

        sf::Text toLabel("To Node:", font, 14);
        toLabel.setPosition(100, 260);
        toLabel.setFillColor(sf::Color::White);
        window.draw(toLabel);
        toNodeInput->draw(window);

        sf::Text distLabel("Distance:", font, 14);
        distLabel.setPosition(100, 320);
        distLabel.setFillColor(sf::Color::White);
        window.draw(distLabel);
        edgeDistanceInput->draw(window);

        drawButton(300, 380, 400, 50, "ADD EDGE");
        drawButton(300, 450, 400, 50, "FIND SHORTEST PATH");
        drawButton(300, 520, 400, 50, "BACK");

        // Display added edges
        sf::Text edgesHeader("Added Edges:", font, 14);
        edgesHeader.setPosition(550, 200);
        edgesHeader.setFillColor(sf::Color::Yellow);
        window.draw(edgesHeader);

        int y = 230;
        for (const auto& edge : userDefinedEdges)
        {
            string edgeStr = to_string(get<0>(edge)) + " -> " + to_string(get<1>(edge)) + " (" + to_string(get<2>(edge)) + ")";
            sf::Text edgeText(edgeStr, font, 12);
            edgeText.setPosition(550, y);
            edgeText.setFillColor(sf::Color::White);
            window.draw(edgeText);
            y += 25;
        }

        // Display shortest path result if found
        if (routeFound && !routePath.empty())
        {
            sf::Text resultLabel("SHORTEST PATH FOUND:", font, 16);
            resultLabel.setPosition(150, 400);
            resultLabel.setFillColor(sf::Color::Green);
            window.draw(resultLabel);

            string pathStr = "Path: ";
            for (int i = routePath.size() - 1; i >= 0; i--)
            {
                if (i < (int)routePath.size() - 1) pathStr += " -> ";
                pathStr += to_string(routePath[i]);
            }

            sf::Text pathInfo(pathStr, font, 14);
            pathInfo.setPosition(150, 430);
            pathInfo.setFillColor(sf::Color::Yellow);
            window.draw(pathInfo);

            string distStr = "Total Distance: " + to_string(routeDistance);
            sf::Text distInfo(distStr, font, 14);
            distInfo.setPosition(150, 460);
            distInfo.setFillColor(sf::Color::Yellow);
            window.draw(distInfo);
        }
    }

    void drawMessageBox()
    {
        // Semi-transparent overlay
        sf::RectangleShape overlay(sf::Vector2f(1000, 700));
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(overlay);

        // Message box
        sf::RectangleShape box(sf::Vector2f(500, 200));
        box.setPosition(250, 250);
        box.setFillColor(sf::Color(50, 50, 50));
        box.setOutlineColor(sf::Color::White);
        box.setOutlineThickness(3);
        window.draw(box);

        // Title
        sf::Text title(messageTitle, font, 24);
        title.setPosition(280, 270);
        title.setFillColor(sf::Color::Yellow);
        window.draw(title);

        // Content
        sf::Text content(messageContent, font, 16);
        content.setPosition(280, 320);
        content.setFillColor(sf::Color::White);
        window.draw(content);
    }

    void drawHeader(const string& title)
    {
        sf::Text headerText(title, font, 36);
        headerText.setPosition(250, 20);
        headerText.setFillColor(sf::Color::Cyan);
        window.draw(headerText);

        // Divider line
        sf::RectangleShape divider(sf::Vector2f(900, 2));
        divider.setPosition(50, 70);
        divider.setFillColor(sf::Color::White);
        window.draw(divider);
    }

    void drawButton(float x, float y, float w, float h, const string& label)
    {
        sf::RectangleShape button(sf::Vector2f(w, h));
        button.setPosition(x, y);
        button.setFillColor(sf::Color(70, 130, 180));
        button.setOutlineColor(sf::Color::White);
        button.setOutlineThickness(2);
        window.draw(button);

        sf::Text text(label, font, 16);
        text.setFillColor(sf::Color::White);
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition(
            x + (w - textBounds.width) / 2,
            y + (h - textBounds.height) / 2 - 5
        );
        window.draw(text);
    }
};

// ============================================
// MAIN FUNCTION
// ============================================

int main()
{
    try
    {
        ECartSystem system;
        system.loadUsers();
        system.loadCatalog();

        GUIManager gui(system);
        gui.run();
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
