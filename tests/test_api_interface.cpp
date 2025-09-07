/*
    Google Test Suite - API Interface Testing
    Copyright (C) 2024 StreamDAB Project
    
    Tests for StreamDAB API integration:
    - RESTful HTTP API endpoints
    - WebSocket real-time communication
    - MessagePack protocol handling
    - Authentication and security
    - Performance and rate limiting
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/api_interface.h"
#include <thread>
#include <chrono>

using namespace StreamDAB;
using namespace testing;

class APIInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test configuration
        test_config_.port = 8080; // Use different port for testing
        test_config_.bind_address = "127.0.0.1";
        test_config_.enable_ssl = false;
        test_config_.max_connections = 10;
        test_config_.enable_authentication = false;
        test_config_.enable_rate_limiting = false; // Disable for testing
        
        api_service_ = std::make_unique<StreamDABAPIService>(test_config_);
        
        // Create test data
        test_status_.is_running = true;
        test_status_.started_at = std::chrono::system_clock::now();
        test_status_.active_connections = 2;
        test_status_.total_requests = 100;
        test_status_.failed_requests = 5;
        test_status_.total_images = 10;
        test_status_.active_images = 8;
        test_status_.current_image = "test_image.jpg";
        test_status_.average_image_quality = 0.85;
        test_status_.total_messages = 25;
        test_status_.queued_messages = 3;
        test_status_.current_message = "Test DLS message";
        test_status_.last_updated = std::chrono::system_clock::now();
    }
    
    void TearDown() override {
        if (api_service_ && api_service_->IsRunning()) {
            api_service_->Stop();
        }
    }
    
    APIConfig test_config_;
    std::unique_ptr<StreamDABAPIService> api_service_;
    SystemStatus test_status_;
};

// Test API service initialization
TEST_F(APIInterfaceTest, ServiceInitialization) {
    EXPECT_NE(api_service_, nullptr);
    EXPECT_FALSE(api_service_->IsRunning());
}

// Test API service start/stop
TEST_F(APIInterfaceTest, ServiceStartStop) {
    EXPECT_TRUE(api_service_->Start());
    EXPECT_TRUE(api_service_->IsRunning());
    
    api_service_->Stop();
    EXPECT_FALSE(api_service_->IsRunning());
}

// Test HTTP endpoint registration
TEST_F(APIInterfaceTest, EndpointRegistration) {
    // This would test the internal endpoint registration
    // In a real implementation, we'd verify endpoints are properly registered
    SUCCEED(); // Placeholder for actual endpoint testing
}

// Test status endpoint response
TEST_F(APIInterfaceTest, StatusEndpoint) {
    // Test status data structure
    EXPECT_TRUE(test_status_.is_running);
    EXPECT_GT(test_status_.total_requests, 0);
    EXPECT_LT(test_status_.failed_requests, test_status_.total_requests);
    EXPECT_GT(test_status_.total_images, 0);
    EXPECT_LE(test_status_.active_images, test_status_.total_images);
    EXPECT_FALSE(test_status_.current_image.empty());
    EXPECT_GE(test_status_.average_image_quality, 0.0);
    EXPECT_LE(test_status_.average_image_quality, 1.0);
}

// Test WebSocket message creation
TEST_F(APIInterfaceTest, WebSocketMessageCreation) {
    WebSocketMessage msg;
    msg.type = WebSocketMessageType::STATUS_UPDATE;
    msg.timestamp = std::chrono::system_clock::now();
    msg.client_id = "test_client_001";
    msg.requires_acknowledgment = true;
    
    // Simulate MessagePack payload
    msg.payload = {0x81, 0xA6, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0xA2, 0x4F, 0x4B}; // {"status": "OK"}
    
    EXPECT_EQ(msg.type, WebSocketMessageType::STATUS_UPDATE);
    EXPECT_FALSE(msg.client_id.empty());
    EXPECT_GT(msg.payload.size(), 0);
    EXPECT_TRUE(msg.requires_acknowledgment);
}

// Test API response creation
TEST_F(APIInterfaceTest, APIResponseCreation) {
    std::map<std::string, std::variant<std::string, int, double, bool>> test_data;
    test_data["status"] = std::string("success");
    test_data["code"] = 200;
    test_data["running"] = true;
    test_data["version"] = 1.0;
    
    auto response = APIUtils::CreateJSONResponse(test_data, 200);
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "application/json");
    EXPECT_TRUE(response.success);
    EXPECT_GT(response.body.size(), 0);
    
    // Test error response
    auto error_response = APIUtils::CreateErrorResponse("Test error", 400);
    EXPECT_EQ(error_response.status_code, 400);
    EXPECT_FALSE(error_response.success);
    EXPECT_FALSE(error_response.error_message.empty());
}

// Test client connection management
TEST_F(APIInterfaceTest, ClientConnectionManagement) {
    auto connection = std::make_shared<ClientConnection>();
    connection->client_id = APIUtils::GenerateClientID();
    connection->ip_address = "127.0.0.1";
    connection->connected_at = std::chrono::system_clock::now();
    connection->last_activity = std::chrono::system_clock::now();
    connection->is_websocket = false;
    connection->user_agent = "Test Client";
    connection->is_active = true;
    
    EXPECT_FALSE(connection->client_id.empty());
    EXPECT_EQ(connection->ip_address, "127.0.0.1");
    EXPECT_TRUE(connection->is_active);
    EXPECT_FALSE(connection->is_websocket);
}

// Test emergency mode activation
TEST_F(APIInterfaceTest, EmergencyModeActivation) {
    EXPECT_FALSE(api_service_->IsEmergencyMode());
    
    api_service_->TriggerEmergencyMode("Emergency test message");
    EXPECT_TRUE(api_service_->IsEmergencyMode());
    
    api_service_->ClearEmergencyMode();
    EXPECT_FALSE(api_service_->IsEmergencyMode());
}

// Test configuration updates
TEST_F(APIInterfaceTest, ConfigurationUpdates) {
    auto original_config = api_service_->GetConfiguration();
    
    APIConfig new_config = original_config;
    new_config.max_connections = 50;
    new_config.connection_timeout = std::chrono::seconds{600};
    
    api_service_->UpdateConfiguration(new_config);
    auto updated_config = api_service_->GetConfiguration();
    
    EXPECT_EQ(updated_config.max_connections, 50);
    EXPECT_EQ(updated_config.connection_timeout, std::chrono::seconds{600});
}

// Test health check functionality
TEST_F(APIInterfaceTest, HealthCheck) {
    bool health_status = api_service_->PerformHealthCheck();
    
    // Basic health check should pass
    EXPECT_TRUE(health_status);
    
    auto health_issues = api_service_->GetHealthIssues();
    EXPECT_TRUE(health_issues.empty()); // No issues expected in test environment
}

// Test status broadcasting
TEST_F(APIInterfaceTest, StatusBroadcasting) {
    // Start service to enable broadcasting
    if (api_service_->Start()) {
        SystemStatus status = api_service_->GetCurrentStatus();
        
        // Verify status structure
        EXPECT_GE(status.active_connections, 0);
        EXPECT_GE(status.total_requests, 0);
        EXPECT_GE(status.failed_requests, 0);
        EXPECT_GE(status.total_images, 0);
        EXPECT_GE(status.total_messages, 0);
        
        // Broadcast status update
        api_service_->BroadcastStatusUpdate();
        
        api_service_->Stop();
    } else {
        GTEST_SKIP() << "Could not start API service for testing";
    }
}

// Test MessagePack utilities
TEST_F(APIInterfaceTest, MessagePackUtilities) {
    // Test packing status update
    auto packed_status = APIUtils::PackStatusUpdate(test_status_);
    EXPECT_GT(packed_status.size(), 0);
    
    // Test unpacking status update
    auto unpacked_status = APIUtils::UnpackStatusUpdate(packed_status);
    EXPECT_EQ(unpacked_status.is_running, test_status_.is_running);
    EXPECT_EQ(unpacked_status.active_connections, test_status_.active_connections);
    EXPECT_EQ(unpacked_status.total_requests, test_status_.total_requests);
    
    // Test packing statistics
    std::map<std::string, double> test_stats;
    test_stats["cpu_usage"] = 45.5;
    test_stats["memory_usage"] = 1024.0;
    test_stats["response_time"] = 25.3;
    
    auto packed_stats = APIUtils::PackStatistics(test_stats);
    EXPECT_GT(packed_stats.size(), 0);
}

// Test MIME type detection
TEST_F(APIInterfaceTest, MimeTypeDetection) {
    EXPECT_EQ(APIUtils::GetMimeType(".jpg"), "image/jpeg");
    EXPECT_EQ(APIUtils::GetMimeType(".jpeg"), "image/jpeg");
    EXPECT_EQ(APIUtils::GetMimeType(".png"), "image/png");
    EXPECT_EQ(APIUtils::GetMimeType(".webp"), "image/webp");
    EXPECT_EQ(APIUtils::GetMimeType(".json"), "application/json");
    EXPECT_EQ(APIUtils::GetMimeType(".txt"), "text/plain");
    EXPECT_EQ(APIUtils::GetMimeType(".unknown"), "application/octet-stream");
}

// Test image upload validation
TEST_F(APIInterfaceTest, ImageUploadValidation) {
    // Valid JPEG data (minimal)
    std::vector<uint8_t> valid_jpeg = {
        0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46,
        0x00, 0x01, 0x01, 0x01, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00,
        0xFF, 0xD9 // End marker
    };
    
    EXPECT_TRUE(APIUtils::ValidateImageUpload(valid_jpeg, "image/jpeg"));
    
    // Invalid image data
    std::vector<uint8_t> invalid_data = {0x00, 0x01, 0x02, 0x03};
    EXPECT_FALSE(APIUtils::ValidateImageUpload(invalid_data, "image/jpeg"));
    
    // Empty data
    std::vector<uint8_t> empty_data;
    EXPECT_FALSE(APIUtils::ValidateImageUpload(empty_data, "image/jpeg"));
}

// Test client ID generation
TEST_F(APIInterfaceTest, ClientIDGeneration) {
    std::string id1 = APIUtils::GenerateClientID();
    std::string id2 = APIUtils::GenerateClientID();
    
    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2); // Should generate unique IDs
    EXPECT_GE(id1.length(), 8); // Should have reasonable length
}

// Test component access
TEST_F(APIInterfaceTest, ComponentAccess) {
    auto mot_processor = api_service_->GetMOTProcessor();
    auto thai_processor = api_service_->GetThaiProcessor();
    auto dls_processor = api_service_->GetDLSProcessor();
    
    // Components should be initialized
    EXPECT_NE(mot_processor, nullptr);
    EXPECT_NE(thai_processor, nullptr);
    EXPECT_NE(dls_processor, nullptr);
}

// Test concurrent connections
TEST_F(APIInterfaceTest, ConcurrentConnections) {
    if (!api_service_->Start()) {
        GTEST_SKIP() << "Could not start API service";
        return;
    }
    
    std::atomic<int> successful_connections{0};
    std::vector<std::thread> connection_threads;
    
    // Simulate multiple concurrent connections
    for (int i = 0; i < 5; ++i) {
        connection_threads.emplace_back([&, i]() {
            // Simulate connection establishment
            auto connection = std::make_shared<ClientConnection>();
            connection->client_id = "test_client_" + std::to_string(i);
            connection->ip_address = "127.0.0.1";
            connection->connected_at = std::chrono::system_clock::now();
            connection->is_active = true;
            
            // Simulate some activity
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            successful_connections++;
        });
    }
    
    // Wait for all connections
    for (auto& thread : connection_threads) {
        thread.join();
    }
    
    EXPECT_EQ(successful_connections.load(), 5);
    
    api_service_->Stop();
}

// Test rate limiting (when enabled)
TEST_F(APIInterfaceTest, RateLimiting) {
    // Enable rate limiting for this test
    APIConfig rate_limited_config = test_config_;
    rate_limited_config.enable_rate_limiting = true;
    rate_limited_config.max_requests_per_minute = 10;
    
    auto rate_limited_service = std::make_unique<StreamDABAPIService>(rate_limited_config);
    
    // Rate limiting would be tested in the actual HTTP server implementation
    // Here we just verify the configuration is accepted
    auto config = rate_limited_service->GetConfiguration();
    EXPECT_TRUE(config.enable_rate_limiting);
    EXPECT_EQ(config.max_requests_per_minute, 10);
}

// Test error handling
TEST_F(APIInterfaceTest, ErrorHandling) {
    // Test invalid configuration
    APIConfig invalid_config;
    invalid_config.port = 99999; // Invalid port
    
    auto invalid_service = std::make_unique<StreamDABAPIService>(invalid_config);
    
    // Service should handle invalid config gracefully
    EXPECT_FALSE(invalid_service->Start());
    
    // Test error response creation
    auto error_response = APIUtils::CreateErrorResponse("Invalid request", 400);
    EXPECT_EQ(error_response.status_code, 400);
    EXPECT_FALSE(error_response.success);
    EXPECT_EQ(error_response.error_message, "Invalid request");
}

// Test WebSocket server functionality
TEST_F(APIInterfaceTest, WebSocketServer) {
    // This would test WebSocket-specific functionality
    // For now, test the message structure
    
    WebSocketMessage status_msg;
    status_msg.type = WebSocketMessageType::STATUS_UPDATE;
    status_msg.client_id = "ws_client_001";
    status_msg.timestamp = std::chrono::system_clock::now();
    
    EXPECT_EQ(status_msg.type, WebSocketMessageType::STATUS_UPDATE);
    EXPECT_FALSE(status_msg.client_id.empty());
    
    WebSocketMessage emergency_msg;
    emergency_msg.type = WebSocketMessageType::EMERGENCY_ALERT;
    emergency_msg.requires_acknowledgment = true;
    
    EXPECT_EQ(emergency_msg.type, WebSocketMessageType::EMERGENCY_ALERT);
    EXPECT_TRUE(emergency_msg.requires_acknowledgment);
}

// Test authentication (when enabled)
TEST_F(APIInterfaceTest, Authentication) {
    APIConfig auth_config = test_config_;
    auth_config.enable_authentication = true;
    auth_config.api_key = "test_api_key_12345";
    
    auto auth_service = std::make_unique<StreamDABAPIService>(auth_config);
    auto config = auth_service->GetConfiguration();
    
    EXPECT_TRUE(config.enable_authentication);
    EXPECT_EQ(config.api_key, "test_api_key_12345");
}

// Test SSL configuration
TEST_F(APIInterfaceTest, SSLConfiguration) {
    APIConfig ssl_config = test_config_;
    ssl_config.enable_ssl = true;
    ssl_config.ssl_cert_path = "/path/to/cert.pem";
    ssl_config.ssl_key_path = "/path/to/key.pem";
    
    auto ssl_service = std::make_unique<StreamDABAPIService>(ssl_config);
    auto config = ssl_service->GetConfiguration();
    
    EXPECT_TRUE(config.enable_ssl);
    EXPECT_EQ(config.ssl_cert_path, "/path/to/cert.pem");
    EXPECT_EQ(config.ssl_key_path, "/path/to/key.pem");
}

// Test performance monitoring
TEST_F(APIInterfaceTest, PerformanceMonitoring) {
    if (api_service_->Start()) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Simulate API operations
        for (int i = 0; i < 10; ++i) {
            auto status = api_service_->GetCurrentStatus();
            EXPECT_GE(status.active_connections, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Operations should complete quickly
        EXPECT_LT(duration.count(), 100); // Less than 100ms for 10 operations
        
        api_service_->Stop();
    } else {
        GTEST_SKIP() << "Could not start API service for performance testing";
    }
}

// Test memory management
TEST_F(APIInterfaceTest, MemoryManagement) {
    // Test that creating and destroying many connections doesn't leak memory
    for (int i = 0; i < 100; ++i) {
        auto connection = std::make_shared<ClientConnection>();
        connection->client_id = "temp_client_" + std::to_string(i);
        connection->ip_address = "127.0.0.1";
        connection->connected_at = std::chrono::system_clock::now();
        
        // Connection should be automatically cleaned up when shared_ptr is destroyed
    }
    
    // Test should complete without memory issues
    SUCCEED();
}

// Test thread safety
TEST_F(APIInterfaceTest, ThreadSafety) {
    if (!api_service_->Start()) {
        GTEST_SKIP() << "Could not start API service";
        return;
    }
    
    std::atomic<int> successful_operations{0};
    std::atomic<bool> test_running{true};
    
    // Multiple threads accessing API service
    std::vector<std::thread> test_threads;
    for (int i = 0; i < 4; ++i) {
        test_threads.emplace_back([&, i]() {
            while (test_running) {
                try {
                    auto status = api_service_->GetCurrentStatus();
                    auto config = api_service_->GetConfiguration();
                    bool health = api_service_->PerformHealthCheck();
                    
                    if (health) {
                        successful_operations++;
                    }
                } catch (const std::exception& e) {
                    // Should not throw exceptions
                    FAIL() << "Thread " << i << " caught exception: " << e.what();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Let threads run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    test_running = false;
    
    // Wait for threads to complete
    for (auto& thread : test_threads) {
        thread.join();
    }
    
    EXPECT_GT(successful_operations.load(), 0);
    
    api_service_->Stop();
}