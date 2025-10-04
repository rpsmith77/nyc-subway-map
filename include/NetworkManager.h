#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <ArduinoWebsockets.h>

class NetworkManager {
    public:
        NetworkManager(const char* ssid, const char* password, const char* host, const char* port);
        void initializeWifi();
        bool checkWifiConnection();
        void initializeWebsocket();
        bool checkWebsocketConnection();
        void onWebSocketMessage(websockets::WebsocketsMessage msg);
        void poll();

    private:
        void handleWifiConnected();
        void handleWifiDisconnected();
        void attemptWifiReconnect();
        void handleMaxWifiFailures();

        void handleWebsocketDisconnect();
        void attemptWebsocketReconnect();
        bool isServerPingable();
        void handleWebsocketConnected();

        const char* ssid;
        const char* password;
        const char* host;
        const char* port;
        bool wifiConnectionStatus;
        websockets::WebsocketsClient wsClient;
        const unsigned long maxBackoffMs;

        // State for wifi connection management
        unsigned long wifiLastAttempt = 0;
        unsigned long wifiBackoffMs = 1000;
        int wifiFailedAttempts = 0;

        // State for websocket connection management
        unsigned long websocketLastAttempt = 0;
        unsigned long websocketBackoffMs = 1000;
        int websocketFailedAttempts = 0;
        unsigned long websocketLastPing = 0;
        bool websocketWasConnected = false;
        bool websocketEverConnected = false;
};

#endif // NETWORK_MANAGER_H