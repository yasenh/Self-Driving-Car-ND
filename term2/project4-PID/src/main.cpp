#include <uWS/uWS.h>
#include <iostream>
#include "json.hpp"
#include "PID.h"
#include <math.h>


// for convenience
using json = nlohmann::json;

// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }
double deg2rad(double x) { return x * pi() / 180; }
double rad2deg(double x) { return x * 180 / pi(); }

const double kMaxSteerValue = 1.0;
const double kMinSteerValue = -1.0;

const double kMaxThrottleValue = 1.0;
const double kMinThrottleValue = -1.0;

const double kMaxSpeed = 40;


// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
std::string hasData(std::string s) {
    auto found_null = s.find("null");
    auto b1 = s.find_first_of("[");
    auto b2 = s.find_last_of("]");
    if (found_null != std::string::npos) {
        return "";
    }
    else if (b1 != std::string::npos && b2 != std::string::npos) {
        return s.substr(b1, b2 - b1 + 1);
    }
    return "";
}

int main() {
    uWS::Hub h;

    PID pid_steer, pid_speed;
    // TODO: Initialize the pid variable.

//    double Kp_steer = 0.1;
//    double Ki_steer = 0.001;
//    double Kd_steer = 0.8;

    double Kp_steer = 0.1;
    double Ki_steer = 0.001;
    double Kd_steer = 1.0;

    pid_steer.Init(Kp_steer, Ki_steer, Kd_steer);


    double Kp_speed = 0.7;
    double Ki_speed = 0.0;
    double Kd_speed = 0.1;

    pid_speed.Init(Kp_speed, Ki_speed, Kd_speed);

    h.onMessage([&pid_steer, &pid_speed](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
        // "42" at the start of the message means there's a websocket message event.
        // The 4 signifies a websocket message
        // The 2 signifies a websocket event
        if (length && length > 2 && data[0] == '4' && data[1] == '2') {
            auto s = hasData(std::string(data));
            if (s != "") {
                auto j = json::parse(s);
                std::string event = j[0].get<std::string>();
                if (event == "telemetry") {
                    // j[1] is the data JSON object
                    double cte = std::stod(j[1]["cte"].get<std::string>());
                    double speed = std::stod(j[1]["speed"].get<std::string>());
                    double angle = std::stod(j[1]["steering_angle"].get<std::string>());

                    double steer_value;
                    double throttle_value;

                    /*
                    * TODO: Calcuate steering value here, remember the steering value is
                    * [-1, 1].
                    * NOTE: Feel free to play around with the throttle and speed. Maybe use
                    * another PID controller to control the speed!
                    */

                    pid_steer.UpdateError(cte);
                    steer_value = -pid_steer.TotalError();

                    if (steer_value > kMaxSteerValue) {
                        steer_value = kMaxSteerValue;
                    }
                    else if (steer_value < kMinSteerValue) {
                        steer_value = kMinSteerValue;
                    }

                    // no matter with the orientation, decrease speed if cte is too large
                    pid_speed.UpdateError(fabs(cte));
                    throttle_value = 1.0 - pid_speed.TotalError();

                    if (throttle_value > kMaxThrottleValue) {
                        throttle_value = kMaxThrottleValue;
                    }
                    else if (throttle_value <= 0 && speed <= 30) {
                        throttle_value = 0.3;
                    }
                    else if (throttle_value < kMinThrottleValue) {
                        throttle_value = kMinThrottleValue;
                    }

                    if (speed >= kMaxSpeed) {
                        throttle_value = 0;
                    }

                    // DEBUG
                    std::cout << "CTE: " << cte << " Speed: " << speed <<" Steering Value: " << steer_value << std::endl;

                    json msgJson;
                    msgJson["steering_angle"] = steer_value;
                    msgJson["throttle"] = throttle_value;
                    auto msg = "42[\"steer\"," + msgJson.dump() + "]";
                    std::cout << msg << std::endl;
                    ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
                }
            } else {
                // Manual driving
                std::string msg = "42[\"manual\",{}]";
                ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
            }
        }
    });

    // We don't need this since we're not using HTTP but if it's removed the program
    // doesn't compile :-(
    h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t, size_t) {
        const std::string s = "<h1>Hello world!</h1>";
        if (req.getUrl().valueLength == 1) {
            res->end(s.data(), s.length());
        }
        else {
            // i guess this should be done more gracefully?
            res->end(nullptr, 0);
        }
    });

    h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
        std::cout << "Connected!!!" << std::endl;
    });

    h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code, char *message, size_t length) {
        ws.close();
        std::cout << "Disconnected" << std::endl;
    });

    int port = 4567;
    if (h.listen(port)) {
        std::cout << "Listening to port " << port << std::endl;
    }
    else {
        std::cerr << "Failed to listen to port" << std::endl;
        return -1;
    }
    h.run();
}
