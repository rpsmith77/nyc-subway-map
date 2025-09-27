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
        const char* ssid;
        const char* password;
        const char* host;
        const char* port;
        bool wifiConnectionStatus;
        websockets::WebsocketsClient wsClient;
        const unsigned long maxBackoffMs;
};

#endif // NETWORK_MANAGER_H