#include <iostream>
#include <vector>
#include <queue>
#include <functional>
#include <algorithm>
#include <string>
#include <sstream>
#include <unordered_set>
#include <set>
#include <thread>
#include <chrono>
#include <climits>
#include <map>
#include <deque>
#include <ctime>
#include <iomanip> 
#include "sqlite3.h"

using namespace std;


// Edge class definition
class Edge {
public:
    int source, destination, weight;

    Edge(int source, int destination, int weight) {
        this->source = source;
        this->destination = destination;
        this->weight = weight;
    }
};

// Graph class definition
class Graph {
public:
    vector<vector<Edge>> adjList;
    vector<string> locations;
    unordered_set<int> rickshawLocations;

    Graph(int size) {
        for (int i = 0; i < size; i++) {
            adjList.push_back(vector<Edge>());
            locations.push_back("");
        }
    }

    void addLocation(int index, const string &name) {
        locations[index] = name;
    }

    void addEdge(int from, int to, int weight) {
        adjList[from].push_back(Edge(from, to, weight));
        adjList[to].push_back(Edge(to, from, weight));
    }

    void addRickshaw(int location) {
        rickshawLocations.insert(location);
    }

    unordered_set<int> getRickshawLocations() {
        return rickshawLocations;
    }

    int getLocationIndex(const string &name) {
        auto it = find(locations.begin(), locations.end(), name);
        if (it != locations.end()) {
            return distance(locations.begin(), it);
        }
        return -1;
    }

    void printRickshawLocations() {
        cout << "\n Current E-Rickshaw Locations:" << endl;
        if (rickshawLocations.empty()) {
            cout << "No e-rickshaws available!" << endl;
            return;
        }
        for (int loc : rickshawLocations) {
            cout << getLocationName(loc) << endl;
        }
    }

    string getLocationName(int index) {
        return locations[index];
    }

    vector<int> dijkstra(int source, vector<int> &parent) {
        int V = adjList.size();
        vector<int> distance(V, INT_MAX);
        parent.assign(V, -1);
        distance[source] = 0;

        auto cmp = [](const Edge &a, const Edge &b) {
            return a.weight > b.weight;
        };
        priority_queue<Edge, vector<Edge>, decltype(cmp)> pq(cmp);
        pq.push(Edge(-1, source, 0));

        while (!pq.empty()) {
            Edge current = pq.top();
            pq.pop();
            int node = current.destination;
            int dist = current.weight;

            if (dist > distance[node])
                continue;

            parent[node] = current.source;

            for (auto &edge : adjList[node]) {
                int newDist = distance[node] + edge.weight;
                if (newDist < distance[edge.destination]) {
                    distance[edge.destination] = newDist;
                    pq.push(Edge(node, edge.destination, newDist));
                }
            }
        }
        return distance;
    }

    vector<string> reconstructPath(vector<int> &parent, int start, int end) {
        vector<string> path;
        for (int at = end; at != -1; at = parent[at]) {
            path.push_back(getLocationName(at));
        }
        reverse(path.begin(), path.end());
        return path;
    }
};

// Base class for pricing strategy
class PricingStrategy {
public:
    virtual double calculateFare(int distance) = 0;
    virtual ~PricingStrategy() {}
};

// Morning pricing strategy (less price)
class MorningPricing : public PricingStrategy {
public:
    double calculateFare(int distance) override {
        return 5.0 + distance * 0.01; 
    }
};

// Afternoon pricing strategy (peak pricing)
class AfternoonPricing : public PricingStrategy {
public:
    double calculateFare(int distance) override {
        return 10.0 + distance * 0.03; 
    }
};

// Evening pricing strategy (intermediate price)
class EveningPricing : public PricingStrategy {
public:
    double calculateFare(int distance) override {
        return 8.0 + distance * 0.02; 
    }
};

// BookingSystem class definition
class BookingSystem {
private:
    Graph *graph;
    sqlite3 *db;

public:
    BookingSystem(Graph &graph, sqlite3 *db) {
        this->graph = &graph;
        this->db = db;
    }

    // Function to determine the current time of day
    string getTimeOfDay() {
        time_t now = time(0);
        tm *ltm = localtime(&now);
        int hour = ltm->tm_hour;
        if (hour >= 6 && hour < 10) {
            return "morning";
        } else if (hour >= 10 && hour < 17) {
            return "afternoon";
        } else {
            return "evening";
        }
    }

    // Function to get the appropriate pricing strategy based on the time of day
    PricingStrategy* getCurrentPricingStrategy() {
        string timeOfDay = getTimeOfDay();
        if (timeOfDay == "morning") {
            return new MorningPricing();
        } else if (timeOfDay == "afternoon") {
            return new AfternoonPricing();
        } else {
            return new EveningPricing();
        }
    }

    string findNearestRickshaw(const string &userLocation) {
        int start = graph->getLocationIndex(userLocation);
        if (start == -1)
            return "Invalid location!";

        vector<int> parent(graph->adjList.size(), -1);
        vector<int> distances = graph->dijkstra(start, parent);

        int minDistance = INT_MAX;
        int nearestRickshaw = -1;

        unordered_set<int> rickshawSet = graph->getRickshawLocations();
        for (int rickshaw : rickshawSet) {
            if (distances[rickshaw] < minDistance) {
                minDistance = distances[rickshaw];
                nearestRickshaw = rickshaw;
            }
        }

        if (nearestRickshaw == -1)
            return "No e-rickshaw available!";

        ostringstream oss;
        oss << "Nearest e-rickshaw is at: " << graph->getLocationName(nearestRickshaw)
            << " with a distance of " << minDistance << " meters.";
        return oss.str();
    }

    string bookRickshaw(const string &userLocation, const string &destination, string &user) {
        int userLoc = graph->getLocationIndex(userLocation);
        int destLoc = graph->getLocationIndex(destination);

        if (userLoc == -1 || destLoc == -1)
            return "Invalid locations!";

        vector<int> parent(graph->adjList.size(), -1);
        vector<int> distances = graph->dijkstra(userLoc, parent);

        int minDistance = INT_MAX;
        int nearestRickshaw = -1;

        unordered_set<int> rickshawSet = graph->getRickshawLocations();
        for (int rickshaw : rickshawSet) {
            if (distances[rickshaw] < minDistance) {
                minDistance = distances[rickshaw];
                nearestRickshaw = rickshaw;
            }
        }

        if (nearestRickshaw == -1)
            return "No e-rickshaw available!";

        vector<int> parentToUser(graph->adjList.size(), -1);
        vector<int> distToUser = graph->dijkstra(nearestRickshaw, parentToUser);

        if (distToUser[userLoc] == INT_MAX)
            return "No path found to user!";

        vector<string> pathToUser = graph->reconstructPath(parentToUser, nearestRickshaw, userLoc);

        cout << "\n🛺 The nearest e-rickshaw from '" << graph->getLocationName(nearestRickshaw)
            << "' is on its way...\n" << endl;

        for (auto &step : pathToUser) {
            cout << "Rickshaw reached: " << step << endl;
            try {
                this_thread::sleep_for(chrono::milliseconds(2000));
            } catch (...) {
                cerr << "Sleep interrupted" << endl;
            }
        }

        cout << "\nRickshaw has arrived at '" << userLocation << "'!\n" << endl;

        vector<int> parentToDest(graph->adjList.size(), -1);
        vector<int> distToDest = graph->dijkstra(userLoc, parentToDest);

        if (distToDest[destLoc] == INT_MAX)
            return "No path found to destination!";

        vector<string> pathToDest = graph->reconstructPath(parentToDest, userLoc, destLoc);

        cout << "\nStarting journey towards '" << destination << "'...\n" << endl;

        for (auto &step : pathToDest) {
            cout << "Rickshaw reached: " << step << endl;
            try {
                this_thread::sleep_for(chrono::milliseconds(2000));
            } catch (...) {
                cerr << "Sleep interrupted" << endl;
            }
        }

        graph->rickshawLocations.erase(nearestRickshaw);
        graph->rickshawLocations.insert(destLoc);

        PricingStrategy* pricingStrategy = getCurrentPricingStrategy();
        double fare = pricingStrategy->calculateFare(distToDest[destLoc]);
        delete pricingStrategy; // Important to avoid memory leaks

        ostringstream oss;
        oss << "\nFULL ROUTE TAKEN: \n"
            << "FROM: " << graph->getLocationName(nearestRickshaw) << "\n"
            << "ROUTE: ";
        for (size_t i = 0; i < pathToDest.size(); i++) {
            oss << pathToDest[i];
            if (i != pathToDest.size() - 1)
                oss << " -> ";
        }
        oss << "\nTOTAL DISTANCE: " << distToDest[destLoc] << " meters.\n"
            << "Rickshaw is now at: " << graph->getLocationName(destLoc)
            << "\nTotal fare: Rs" << fixed << setprecision(2) << fare << "\n\n\n";

        int rating;
        cout << "\nRate the driver (1-5): ";
        cin >> rating;
        cin.ignore();

        logRide(db, user, nearestRickshaw, userLocation, destination, distToDest[destLoc], static_cast<int>(fare), rating);

        return oss.str();
    }

    void startBooking() {
        string user;
        cout << "Enter your username: ";
        getline(cin, user);

        while (true) {
            graph->printRickshawLocations();
            cout << "\n----------- E-RICKSHAW BOOKING SYSTEM------------" << endl;
            cout << "1- Find Nearest E-Rickshaw\n" << endl;
            cout << "2- Book an E-Rickshaw\n" << endl;
            cout << "3- View Ride History\n" << endl;
            cout << "4- Exit\n" << endl;
            cout << "Enter your choice:  \n";
            int choice;
            cin >> choice;
            cin.ignore();

            if (choice == 4) {
                cout << " Exiting... Thank you for using the service!" << endl;
                break;
            }

            if (choice == 3) {
                showRideHistory(db, user);
                continue;
            }

            cout << "Enter your location: \n";
            string userLocation;
            getline(cin, userLocation);

            if (choice == 1) {
                cout << findNearestRickshaw(userLocation) << endl;
            } else if (choice == 2) {
                cout << "Enter your destination: \n";
                string destination;
                getline(cin, destination);
                cout << bookRickshaw(userLocation, destination, user) << endl;
                cout << "Enter your username: \n";
                getline(cin, user);
            } else {
                cout << " Invalid choice! Try again." << endl;
                break;
            }
        }
    }
    void logRide(sqlite3 *db, const string &user, int driverID, const string &source, const string &destination, int distance, int fare, int rating) {
        sqlite3_stmt *stmt;
        const char *sql = "INSERT INTO Rides (username, driver_id, source, destination, distance, fare, rating) VALUES (?, ?, ?, ?, ?, ?, ?);";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, driverID);
            sqlite3_bind_text(stmt, 3, source.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, destination.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 5, distance);
            sqlite3_bind_int(stmt, 6, fare);
            sqlite3_bind_int(stmt, 7, rating);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                cerr << "Error inserting ride: " << sqlite3_errmsg(db) << endl;
            }
            sqlite3_finalize(stmt);
        } else {
            cerr << "Error preparing statement: " << sqlite3_errmsg(db) << endl;
        }
    }
    void showRideHistory(sqlite3 *db, const string &user) {
        sqlite3_stmt *stmt;
        const char *sql = "SELECT source, destination, distance, fare, rating FROM Rides WHERE username = ? ORDER BY timestamp DESC LIMIT 10;";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
            cout << "\nRide History for " << user << ":\n";
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                cout << "Source: " << sqlite3_column_text(stmt, 0) << endl;
                cout << "Destination: " << sqlite3_column_text(stmt, 1) << endl;
                cout << "Distance: " << sqlite3_column_int(stmt, 2) << " meters" << endl;
                cout << "Fare: Rs" << sqlite3_column_int(stmt, 3) << endl;
                cout << "Rating: " << sqlite3_column_int(stmt, 4) << endl;
                cout << "------------------------" << endl;
            }
            sqlite3_finalize(stmt);
        } else {
            cerr << "Error preparing statement: " << sqlite3_errmsg(db) << endl;
        }
    }
};

int main() {
    Graph graph(40);
    sqlite3 *db;
    if (sqlite3_open("erickshaw.db", &db) != SQLITE_OK) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        return 1;
    }

    const char *createTable = "CREATE TABLE IF NOT EXISTS Rides (username TEXT, driver_id INTEGER, source TEXT, destination TEXT, distance INTEGER, fare INTEGER, rating INTEGER, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    char *errMsg = nullptr;
    if (sqlite3_exec(db, createTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
    BookingSystem bookingSystem(graph, db);

    string locationsArray[] = {
        "NIT POINT", "ADMIN", "GUEST HOUSE", "OLD GALLERY", "T-POINT", "BH8",
        "GH4", "KENDRIYA VIDYALAYA", "CC BUILDING", "LIBRARY", "ARYABHATTA",
        "BH9", "CRICKET GROUND", "FOOTBALL GROUND", "BH6", "BH7", "SPORTS COMPLEX",
        "SAC BUILDING", "GH3", "DISPENSARY", "BH1", "CHILDREN'S PARK", "GH1(FRONT GATE)", "EARTHQUAKE BUILDING",
        "NITS CAFE", "WORKSHOP", "NEW GALLERY", "STARTUP CENTRE", "BH2", "LAKE VIEWPOINT 3", "NURSERY1",
        "NURSERY2", "LP SCHOOL", "LAKE VIEWPOINT 2", "POWER GRID", "GH2(BACK GATE)", "INDIE VIEW POINT", "BH4", "PROF. QUARTERS"
    };

    int locationsLength = sizeof(locationsArray) / sizeof(locationsArray[0]);
    for (int i = 0; i < locationsLength; i++) {
        graph.addLocation(i, locationsArray[i]);
    }

    int distancesArr[][3] = {{14, 15, 10}, {10, 20, 20}, {20, 28, 20}, {22, 21, 20}, {19, 22, 50},
        {0, 1, 100}, {1, 2, 150}, {2, 24, 150}, {24, 3, 50}, {3, 25, 10},
        {25, 8, 30}, {4, 37, 65}, {37, 5, 15}, {5, 6, 10}, {6, 7, 30}, {7, 30, 300}, {30, 1, 400}, {9, 4, 100},
        {4, 10, 40}, {28, 11, 20}, {12, 13, 120}, {11, 12, 10}, {3, 25, 10}, {8, 26, 30}, {32, 35, 80}, {35, 17, 120},
        {26, 27, 45}, {27, 9, 15}, {13, 14, 130}, {13, 16, 200}, {4, 17, 100}, {17, 18, 200}, {18, 19, 30}, {18, 29, 120},
        {29, 23, 100}, {21, 38, 100}, {38, 23, 250}, {38, 33, 120}, {8, 29, 150}, {12, 31, 200},
        {33, 34, 220}, {34, 1, 80}, {31, 32, 80}, {22, 35, 100}, {36, 16, 40}, {36, 31, 150}};

    int distancesSize = sizeof(distancesArr) / sizeof(distancesArr[0]);
    for (int i = 0; i < distancesSize; i++) {
        int from = distancesArr[i][0];
        int to = distancesArr[i][1];
        int weight = distancesArr[i][2];
        graph.addEdge(from, to, weight);
    }

    graph.addRickshaw(1);
    graph.addRickshaw(21);
    graph.addRickshaw(17);
    graph.addRickshaw(5);
    graph.addRickshaw(8);
    graph.addRickshaw(11);
    graph.addRickshaw(16);
    graph.addRickshaw(14);

    bookingSystem.startBooking();

    sqlite3_close(db);
    return 0;
}