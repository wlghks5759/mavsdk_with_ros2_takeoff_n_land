#include <chrono>
#include <cstdint>
#include <memory>
#include <future>
#include <thread>
#include <iostream>
#include <sstream>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float32.hpp"
#include "std_srvs/srv/trigger.hpp" // For simple services

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>

using namespace mavsdk;
using std::chrono::seconds;
using std::this_thread::sleep_for;

class TakeoffAndLandNode : public rclcpp::Node
{
public:
    TakeoffAndLandNode() : Node("takeoff_and_land_node")
    {
        // Declare and get parameters
        this->declare_parameter<std::string>("connection_url", "udpin://0.0.0.0:14540");
        this->get_parameter("connection_url", connection_url_);

        RCLCPP_INFO(this->get_logger(), "Connecting to MAVSDK with URL: %s", connection_url_.c_str());

        // Initialize MAVSDK
        mavsdk_ = std::make_unique<Mavsdk>(Mavsdk::Configuration{ComponentType::GroundStation});
        ConnectionResult connection_result = mavsdk_->add_any_connection(connection_url_);

        if (connection_result != ConnectionResult::Success) {
            RCLCPP_ERROR(this->get_logger(), "MAVSDK connection failed: %s", connection_result_str(connection_result).c_str());
            rclcpp::shutdown();
            return;
        }

        RCLCPP_INFO(this->get_logger(), "Waiting for system to connect...");
        auto prom = std::promise<std::shared_ptr<System>>{};
        auto fut = prom.get_future();
        mavsdk_->subscribe_on_new_system([&prom, this]() {
            auto system = mavsdk_->systems().front(); // Get the first system
            if (system && system->is_connected()) {
                 RCLCPP_INFO(this->get_logger(), "System discovered.");
                prom.set_value(system);
            }
        });

        if (fut.wait_for(seconds(5)) == std::future_status::timeout) { // Increased timeout
            RCLCPP_ERROR(this->get_logger(), "Timed out waiting for system to connect.");
            rclcpp::shutdown();
            return;
        }
        system_ = fut.get();

        if (!system_) {
            RCLCPP_ERROR(this->get_logger(), "Failed to get system.");
            rclcpp::shutdown();
            return;
        }
        
        RCLCPP_INFO(this->get_logger(), "System connected.");

        // Instantiate plugins
        telemetry_ = std::make_shared<Telemetry>(system_);
        action_ = std::make_shared<Action>(system_);

        // Setup telemetry subscription for altitude
        const auto set_rate_result = telemetry_->set_rate_position(1.0);
        if (set_rate_result != Telemetry::Result::Success) {
            std::stringstream ss;
            ss << set_rate_result; // Telemetry::Result 열거형을 직접 스트림에 넣음
            RCLCPP_WARN(this->get_logger(), "Setting position rate failed: %s", ss.str().c_str());
        }

        telemetry_->subscribe_position([this](Telemetry::Position position) {
            auto msg = std_msgs::msg::Float32();
            msg.data = position.relative_altitude_m;
            altitude_publisher_->publish(msg);
            // RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "Altitude: %.2f m", position.relative_altitude_m);
        });

        // Create publishers
        altitude_publisher_ = this->create_publisher<std_msgs::msg::Float32>("altitude", 10);

        // Create services
        arm_service_ = this->create_service<std_srvs::srv::Trigger>(
            "arm_vehicle",
            std::bind(&TakeoffAndLandNode::handle_arm, this, std::placeholders::_1, std::placeholders::_2));

        takeoff_service_ = this->create_service<std_srvs::srv::Trigger>(
            "takeoff_vehicle",
            std::bind(&TakeoffAndLandNode::handle_takeoff, this, std::placeholders::_1, std::placeholders::_2));

        land_service_ = this->create_service<std_srvs::srv::Trigger>(
            "land_vehicle",
            std::bind(&TakeoffAndLandNode::handle_land, this, std::placeholders::_1, std::placeholders::_2));
        
        RCLCPP_INFO(this->get_logger(), "Node initialized. Ready to receive commands.");
    }

private:
    void handle_arm(const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
                    std::shared_ptr<std_srvs::srv::Trigger::Response> response)
    {
        (void)request; // Unused
        if (!system_ || !action_ || !telemetry_) {
            response->success = false;
            response->message = "MAVSDK system not initialized.";
            RCLCPP_ERROR(this->get_logger(), "Arming failed: %s", response->message.c_str());
            return;
        }

        RCLCPP_INFO(this->get_logger(), "Waiting for vehicle to be ready to arm...");
        while (telemetry_->health_all_ok() != true) {
            RCLCPP_INFO(this->get_logger(), "Vehicle is getting ready to arm...");
            if (!rclcpp::ok()) {
                 response->success = false; response->message = "Shutdown requested."; return;
            }
            sleep_for(seconds(1));
        }

        RCLCPP_INFO(this->get_logger(), "Arming...");
        const Action::Result arm_result = action_->arm();
        if (arm_result != Action::Result::Success) {
            std::stringstream ss;
            ss << arm_result; // Action::Result 열거형을 직접 스트림에 넣음
            response->message = "Arming failed: " + ss.str();
            RCLCPP_ERROR(this->get_logger(), "%s", response->message.c_str());
        } else {
            response->success = true;
            response->message = "Vehicle armed successfully.";
            RCLCPP_INFO(this->get_logger(), "%s", response->message.c_str());
        }
    }

    void handle_takeoff(const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
                        std::shared_ptr<std_srvs::srv::Trigger::Response> response)
    {
        (void)request; // Unused
        if (!system_ || !action_) {
            response->success = false;
            response->message = "MAVSDK system not initialized.";
            RCLCPP_ERROR(this->get_logger(), "Takeoff failed: %s", response->message.c_str());
            return;
        }

        RCLCPP_INFO(this->get_logger(), "Taking off...");
        const Action::Result takeoff_result = action_->takeoff();
        if (takeoff_result != Action::Result::Success) {
            response->success = false;
            std::stringstream ss;
            ss << takeoff_result; // <<< 수정됨
            response->message = "Takeoff failed: " + ss.str();
            RCLCPP_ERROR(this->get_logger(), "%s", response->message.c_str());
        } else {
            response->success = true;
            response->message = "Takeoff initiated successfully.";
            RCLCPP_INFO(this->get_logger(), "%s", response->message.c_str());
            // Note: The original example had a sleep here.
            // In a ROS service, we typically return after initiating the action.
            // The client can then monitor telemetry or wait if needed.
        }
    }

    void handle_land(const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
                     std::shared_ptr<std_srvs::srv::Trigger::Response> response)
    {
        (void)request; // Unused
        if (!system_ || !action_ || !telemetry_) {
            response->success = false;
            response->message = "MAVSDK system not initialized.";
            RCLCPP_ERROR(this->get_logger(), "Landing failed: %s", response->message.c_str());
            return;
        }

        RCLCPP_INFO(this->get_logger(), "Landing...");
        const Action::Result land_result = action_->land();
        if (land_result != Action::Result::Success) {
            response->success = false;
            std::stringstream ss;
            ss << land_result; // <<< 수정됨
            response->message = "Land command failed: " + ss.str();
            RCLCPP_ERROR(this->get_logger(), "%s", response->message.c_str());
        } else {
            response->success = true;
            response->message = "Landing initiated. Monitoring until landed.";
            RCLCPP_INFO(this->get_logger(), "%s", response->message.c_str());
            
            // Monitor until landed (MAVSDK's land() is blocking, but let's be explicit for ROS service clarity)
            while (telemetry_->in_air()) {
                RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "Vehicle is landing...");
                if (!rclcpp::ok()) {
                     response->success = false; response->message = "Shutdown requested during landing."; return;
                }
                sleep_for(seconds(1)); // This blocks the service callback, consider async for complex nodes
            }
            response->message = "Vehicle landed successfully.";
            RCLCPP_INFO(this->get_logger(), "Landed!");
        }
    }
    
    // Helper to convert ConnectionResult to string for logging
    std::string connection_result_str(ConnectionResult result) {
        switch (result) {
            case ConnectionResult::Success: return "Success";
            case ConnectionResult::Timeout: return "Timeout";
            case ConnectionResult::SocketError: return "SocketError";
            case ConnectionResult::SocketConnectionError: return "SocketConnectionError";
            case ConnectionResult::ConnectionError: return "ConnectionError";
            case ConnectionResult::NotImplemented: return "NotImplemented";
            case ConnectionResult::SystemNotConnected: return "SystemNotConnected";
            case ConnectionResult::SystemBusy: return "SystemBusy";
            case ConnectionResult::CommandDenied: return "CommandDenied";
            case ConnectionResult::DestinationIpUnknown: return "DestinationIpUnknown";
            case ConnectionResult::ConnectionsExhausted: return "ConnectionsExhausted";
            case ConnectionResult::ConnectionUrlInvalid: return "ConnectionUrlInvalid";
            default: return "Unknown";
        }
    }


    std::string connection_url_;
    std::unique_ptr<Mavsdk> mavsdk_;
    std::shared_ptr<System> system_;
    std::shared_ptr<Telemetry> telemetry_;
    std::shared_ptr<Action> action_;

    rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr altitude_publisher_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr arm_service_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr takeoff_service_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr land_service_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<TakeoffAndLandNode>();
    if (rclcpp::ok()) { // Check if initialization was successful
        rclcpp::spin(node);
    }
    rclcpp::shutdown();
    return 0;
}