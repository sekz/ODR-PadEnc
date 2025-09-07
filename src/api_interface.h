/*
    StreamDAB Integration API Interface
    Copyright (C) 2024 StreamDAB Project
    
    RESTful HTTP API (port 8008)
    WebSocket real-time updates with MessagePack
    StreamDAB-ContentManager integration
    Remote content management
    Emergency content override
*/

#ifndef API_INTERFACE_H_
#define API_INTERFACE_H_

#include "common.h"
#include "enhanced_mot.h"
#include "thai_rendering.h"
#include "smart_dls.h"
#include "security_utils.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <chrono>
#include <queue>

// HTTP server dependencies (would use a library like cpp-httplib or similar)
// For this implementation, we'll define the interface

namespace StreamDAB {

// API response structure
struct APIResponse {
    int status_code = 200;
    std::string content_type = "application/json";
    std::vector<uint8_t> body;
    std::map<std::string, std::string> headers;
    bool success = true;
    std::string error_message;
};

// WebSocket message types
enum class WebSocketMessageType {
    STATUS_UPDATE,
    CONTENT_NOTIFICATION,
    EMERGENCY_ALERT,
    STATISTICS_UPDATE,
    CONFIGURATION_CHANGE,
    HEALTH_CHECK
};

// WebSocket message structure
struct WebSocketMessage {
    WebSocketMessageType type;
    std::vector<uint8_t> payload; // MessagePack encoded data
    std::chrono::system_clock::time_point timestamp;
    std::string client_id;
    bool requires_acknowledgment = false;
};

// API endpoint information
struct APIEndpoint {
    std::string path;
    std::string method; // GET, POST, PUT, DELETE
    std::function<APIResponse(const std::map<std::string, std::string>& params,
                             const std::vector<uint8_t>& body)> handler;
    bool requires_authentication = false;
    std::vector<std::string> required_permissions;
    std::string description;
};

// Client connection information
struct ClientConnection {
    std::string client_id;
    std::string ip_address;
    std::chrono::system_clock::time_point connected_at;
    std::chrono::system_clock::time_point last_activity;
    bool is_websocket = false;
    std::string user_agent;
    std::vector<std::string> subscriptions; // WebSocket subscriptions
    std::atomic<bool> is_active{true};
};

// API configuration
struct APIConfig {
    uint16_t port = 8008;
    std::string bind_address = "0.0.0.0";
    bool enable_ssl = false;
    std::string ssl_cert_path;
    std::string ssl_key_path;
    size_t max_connections = 100;
    std::chrono::seconds connection_timeout{300}; // 5 minutes
    size_t max_request_size = 10 * 1024 * 1024; // 10MB
    std::string cors_origin = "*";
    bool enable_authentication = false;
    std::string api_key;
    bool enable_rate_limiting = true;
    size_t max_requests_per_minute = 60;
};

// Real-time status information
struct SystemStatus {
    bool is_running = false;
    std::chrono::system_clock::time_point started_at;
    size_t active_connections = 0;
    size_t total_requests = 0;
    size_t failed_requests = 0;
    
    // MOT status
    size_t total_images = 0;
    size_t active_images = 0;
    std::string current_image;
    double average_image_quality = 0.0;
    
    // DLS status
    size_t total_messages = 0;
    size_t queued_messages = 0;
    std::string current_message;
    MessagePriority highest_priority = MessagePriority::BACKGROUND;
    
    // Performance metrics
    double cpu_usage = 0.0;
    size_t memory_usage = 0;
    std::chrono::microseconds avg_response_time{0};
    
    // Thai language status
    size_t thai_messages_processed = 0;
    bool buddhist_calendar_active = false;
    
    std::chrono::system_clock::time_point last_updated;
};

// HTTP server interface
class HTTPServer {
private:
    APIConfig config_;
    std::vector<APIEndpoint> endpoints_;
    std::map<std::string, std::shared_ptr<ClientConnection>> connections_;
    std::atomic<bool> server_running_{false};
    std::thread server_thread_;
    std::mutex connections_mutex_;
    
    // Rate limiting
    std::map<std::string, std::queue<std::chrono::steady_clock::time_point>> rate_limit_data_;
    std::mutex rate_limit_mutex_;
    
    // Request processing
    APIResponse ProcessRequest(const std::string& method,
                              const std::string& path,
                              const std::map<std::string, std::string>& headers,
                              const std::map<std::string, std::string>& params,
                              const std::vector<uint8_t>& body);
    
    bool IsRateLimited(const std::string& client_ip);
    bool AuthenticateRequest(const std::map<std::string, std::string>& headers);
    
    // Server implementation (would use actual HTTP library)
    void ServerLoop();
    
public:
    explicit HTTPServer(const APIConfig& config = APIConfig{});
    ~HTTPServer();
    
    // Server control
    bool Start();
    void Stop();
    bool IsRunning() const { return server_running_; }
    
    // Endpoint management
    void RegisterEndpoint(const APIEndpoint& endpoint);
    void UnregisterEndpoint(const std::string& path, const std::string& method);
    
    // Connection management
    std::vector<std::shared_ptr<ClientConnection>> GetActiveConnections() const;
    void DisconnectClient(const std::string& client_id);
    
    // Statistics
    struct ServerStatistics {
        std::chrono::system_clock::time_point start_time;
        size_t total_requests = 0;
        size_t successful_requests = 0;
        size_t failed_requests = 0;
        size_t active_connections = 0;
        size_t peak_connections = 0;
        std::chrono::microseconds average_response_time{0};
        std::map<std::string, size_t> endpoint_usage;
    };
    ServerStatistics GetStatistics() const;
};

// WebSocket server for real-time communication
class WebSocketServer {
private:
    std::map<std::string, std::shared_ptr<ClientConnection>> websocket_clients_;
    std::mutex clients_mutex_;
    std::atomic<bool> server_running_{false};
    std::thread broadcast_thread_;
    std::queue<WebSocketMessage> broadcast_queue_;
    std::mutex broadcast_mutex_;
    std::condition_variable broadcast_condition_;
    
    // MessagePack encoding/decoding
    std::vector<uint8_t> EncodeMessagePack(const std::map<std::string, std::string>& data);
    std::map<std::string, std::string> DecodeMessagePack(const std::vector<uint8_t>& data);
    
    void BroadcastLoop();
    void HandleClientMessage(const std::string& client_id, const WebSocketMessage& message);
    
public:
    WebSocketServer();
    ~WebSocketServer();
    
    // Server control
    bool Start(uint16_t port = 8009); // Different port from HTTP
    void Stop();
    
    // Client management
    void AddClient(const std::string& client_id, std::shared_ptr<ClientConnection> connection);
    void RemoveClient(const std::string& client_id);
    std::vector<std::string> GetConnectedClients() const;
    
    // Message broadcasting
    void BroadcastMessage(const WebSocketMessage& message);
    void SendToClient(const std::string& client_id, const WebSocketMessage& message);
    void SendToSubscribers(const std::string& topic, const WebSocketMessage& message);
    
    // Subscription management
    void SubscribeClient(const std::string& client_id, const std::string& topic);
    void UnsubscribeClient(const std::string& client_id, const std::string& topic);
    
    // Status broadcasting
    void BroadcastStatusUpdate(const SystemStatus& status);
    void BroadcastEmergencyAlert(const std::string& message);
};

// Main API service that integrates all components
class StreamDABAPIService {
private:
    HTTPServer http_server_;
    WebSocketServer websocket_server_;
    
    // Component references
    std::unique_ptr<EnhancedMOTProcessor> mot_processor_;
    std::unique_ptr<ThaiLanguageProcessor> thai_processor_;
    std::unique_ptr<SmartDLSProcessor> dls_processor_;
    std::unique_ptr<SecurePathValidator> path_validator_;
    std::unique_ptr<ContentSecurityScanner> security_scanner_;
    
    // System status
    SystemStatus current_status_;
    std::mutex status_mutex_;
    std::atomic<bool> service_running_{false};
    std::thread status_update_thread_;
    
    // Emergency override
    std::atomic<bool> emergency_mode_{false};
    std::string emergency_message_;
    std::chrono::system_clock::time_point emergency_start_time_;
    
    // Configuration
    APIConfig api_config_;
    
    // Initialize API endpoints
    void InitializeEndpoints();
    void UpdateSystemStatus();
    void StatusUpdateLoop();
    
    // API endpoint handlers
    APIResponse HandleGetStatus(const std::map<std::string, std::string>& params,
                               const std::vector<uint8_t>& body);
    APIResponse HandleGetImages(const std::map<std::string, std::string>& params,
                               const std::vector<uint8_t>& body);
    APIResponse HandleAddImage(const std::map<std::string, std::string>& params,
                              const std::vector<uint8_t>& body);
    APIResponse HandleRemoveImage(const std::map<std::string, std::string>& params,
                                 const std::vector<uint8_t>& body);
    APIResponse HandleGetMessages(const std::map<std::string, std::string>& params,
                                 const std::vector<uint8_t>& body);
    APIResponse HandleAddMessage(const std::map<std::string, std::string>& params,
                                const std::vector<uint8_t>& body);
    APIResponse HandleSetContext(const std::map<std::string, std::string>& params,
                                const std::vector<uint8_t>& body);
    APIResponse HandleEmergencyOverride(const std::map<std::string, std::string>& params,
                                       const std::vector<uint8_t>& body);
    APIResponse HandleGetConfiguration(const std::map<std::string, std::string>& params,
                                      const std::vector<uint8_t>& body);
    APIResponse HandleSetConfiguration(const std::map<std::string, std::string>& params,
                                      const std::vector<uint8_t>& body);
    APIResponse HandleGetStatistics(const std::map<std::string, std::string>& params,
                                   const std::vector<uint8_t>& body);
    APIResponse HandleHealthCheck(const std::map<std::string, std::string>& params,
                                 const std::vector<uint8_t>& body);
    
    // Thai language endpoints
    APIResponse HandleValidateThai(const std::map<std::string, std::string>& params,
                                  const std::vector<uint8_t>& body);
    APIResponse HandleConvertThaiText(const std::map<std::string, std::string>& params,
                                     const std::vector<uint8_t>& body);
    APIResponse HandleGetBuddhistCalendar(const std::map<std::string, std::string>& params,
                                         const std::vector<uint8_t>& body);
    
    // Utility methods
    std::string SerializeJSON(const std::map<std::string, std::variant<std::string, int, double, bool>>& data);
    std::map<std::string, std::string> ParseJSON(const std::vector<uint8_t>& json_data);
    
public:
    explicit StreamDABAPIService(const APIConfig& config = APIConfig{});
    ~StreamDABAPIService();
    
    // Service control
    bool Start();
    void Stop();
    bool IsRunning() const { return service_running_; }
    
    // Component access
    EnhancedMOTProcessor* GetMOTProcessor() { return mot_processor_.get(); }
    ThaiLanguageProcessor* GetThaiProcessor() { return thai_processor_.get(); }
    SmartDLSProcessor* GetDLSProcessor() { return dls_processor_.get(); }
    
    // Emergency management
    void TriggerEmergencyMode(const std::string& message);
    void ClearEmergencyMode();
    bool IsEmergencyMode() const { return emergency_mode_; }
    
    // Status and statistics
    SystemStatus GetCurrentStatus() const;
    void BroadcastStatusUpdate();
    
    // Configuration management
    void UpdateConfiguration(const APIConfig& new_config);
    APIConfig GetConfiguration() const { return api_config_; }
    
    // Integration with StreamDAB-ContentManager
    void RegisterWithContentManager(const std::string& content_manager_url);
    void NotifyContentManager(const std::string& event_type, const std::map<std::string, std::string>& data);
    
    // Health monitoring
    bool PerformHealthCheck();
    std::vector<std::string> GetHealthIssues();
};

// Utility functions for API responses
namespace APIUtils {
    APIResponse CreateJSONResponse(const std::map<std::string, std::variant<std::string, int, double, bool>>& data,
                                   int status_code = 200);
    APIResponse CreateErrorResponse(const std::string& error_message, int status_code = 400);
    APIResponse CreateSuccessResponse(const std::string& message = "OK");
    
    std::string GetMimeType(const std::string& file_extension);
    std::vector<uint8_t> LoadFileContent(const std::string& file_path);
    
    bool ValidateImageUpload(const std::vector<uint8_t>& data, const std::string& content_type);
    std::string GenerateClientID();
    
    // MessagePack utilities
    std::vector<uint8_t> PackStatusUpdate(const SystemStatus& status);
    std::vector<uint8_t> PackStatistics(const std::map<std::string, double>& stats);
    SystemStatus UnpackStatusUpdate(const std::vector<uint8_t>& packed_data);
}

} // namespace StreamDAB

#endif // API_INTERFACE_H_