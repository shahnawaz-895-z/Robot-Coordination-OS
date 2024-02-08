#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <cmath>

using namespace std;

#define NUM_ROBOTS 50
#define ROOM_WIDTH 100
#define ROOM_HEIGHT 100
#define EXIT_WIDTH_MIN 16
#define EXIT_WIDTH_MAX 26
#define NEIGHBOR_DISTANCE 5
#define ACCURACY_THRESHOLD_1 0.95
#define ACCURACY_THRESHOLD_2 0.88

pthread_mutex_t robot_mutex;
pthread_mutex_t total_width_mutex;

struct Robot {
    int id;
    int x;
    int y;
    double estimatedWidth;
};

double Total_width = 0.0;

Robot robots[NUM_ROBOTS];

// Function to calculate accuracy based on distance
double calculateAccuracy(int distance) {
    if (distance <= 5) {
        return ACCURACY_THRESHOLD_1;
    } else if (distance <= 10) {
        return ACCURACY_THRESHOLD_2 + (ACCURACY_THRESHOLD_1 - ACCURACY_THRESHOLD_2) * (distance - 5) / 5;
    } else {
        return 0.0;
    }
}

// Function to check if two robots are neighbors
bool isNeighbor(const Robot& robot1, const Robot& robot2) {
    int distance = abs(robot1.x - robot2.x) + abs(robot1.y - robot2.y);
    return distance <= NEIGHBOR_DISTANCE;
}

// Function to compute average estimate based on neighbors
void computeAverageEstimate(Robot* robot) {
    double sum = robot->estimatedWidth;
    int count = 1;

    for (int i = 0; i < NUM_ROBOTS; ++i) {
        if (i != robot->id && isNeighbor(*robot, robots[i])) {
            sum += robots[i].estimatedWidth;
            count++;
        }
    }

    robot->estimatedWidth = sum / count;
}

// Function to simulate the behavior of a robot
void* robotBehavior(void* arg) {
    Robot* robot = (Robot*)arg;

    for (int i = 0; i < 10; ++i) {
        int distanceToExit = abs(robot->x - (ROOM_WIDTH / 2)) + abs(robot->y - ROOM_HEIGHT);
        double accuracy = calculateAccuracy(distanceToExit);

        double trueWidth = EXIT_WIDTH_MIN + rand() % (EXIT_WIDTH_MAX - EXIT_WIDTH_MIN + 1);
        double estimation = trueWidth + (rand() % 21 - 10) * accuracy;

        pthread_mutex_lock(&robot_mutex);

       cout << "Robot " << robot->id << " at (" << robot->x << ", " << robot->y
            << ") estimates exit width: " << robot->estimatedWidth
            << ", True width: " << trueWidth
             << ", Absolute Difference: " << fabs(robot->estimatedWidth - trueWidth) << endl;

        for (int j = 0; j < NUM_ROBOTS; ++j) {
            if (j != robot->id && isNeighbor(*robot, robots[j])) {
                robots[j].estimatedWidth = estimation;  // Update estimation of neighbors
            }
        }
        
        cout << "Robot " << robot->id << " updated its own estimated width to " << robot->estimatedWidth << endl;
       
        computeAverageEstimate(robot);

        

        pthread_mutex_unlock(&robot_mutex);

        sleep(1);
    }

    pthread_exit(NULL);
}

// Function for global aggregation
void* globalAggregation(void* arg) {
    sleep(10);

    pthread_mutex_lock(&total_width_mutex);
    Total_width = 0.0;
    int count =0;
    for (int i = 0; i < NUM_ROBOTS; ++i) {
       
        if(robots[i].estimatedWidth !=0)
        {
           Total_width += robots[i].estimatedWidth;
           count++;
        }
    }

    double averageWidth = Total_width / count;
    cout << "\nGlobal Aggregation: Total Width = " << Total_width << ", Average Width = " << averageWidth << endl;

    pthread_mutex_unlock(&total_width_mutex);

    pthread_exit(NULL);
}

int main() {
    srand(time(nullptr));

    pthread_t threads[NUM_ROBOTS];
    pthread_t aggregationThread;

    pthread_mutex_init(&robot_mutex, NULL);
    pthread_mutex_init(&total_width_mutex, NULL);

    // Initialize robots
    for (int i = 0; i < NUM_ROBOTS; ++i) {
        robots[i] = {i, rand() % ROOM_WIDTH, rand() % ROOM_HEIGHT, 0.0};
        pthread_create(&threads[i], NULL, robotBehavior, (void*)&robots[i]);
    }

    pthread_create(&aggregationThread, NULL, globalAggregation, NULL);

    for (int i = 0; i < NUM_ROBOTS; ++i) {
        pthread_join(threads[i], NULL);
    }

    pthread_join(aggregationThread, NULL);

    pthread_mutex_destroy(&robot_mutex);
    pthread_mutex_destroy(&total_width_mutex);

    return 0;
}
