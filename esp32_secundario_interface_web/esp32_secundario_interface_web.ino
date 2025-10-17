#include <Arduino.h>
#include <WiFi.h>
#include <WiFiServer.h>
#include <WiFiClient.h>

// Configurações WiFi - USAR A MESMA REDE DO ESP32 #1
const char* ssid = "lab120";
const char* password = "labredes120";
const int serverPort = 80;

WiFiServer server(serverPort);

// Estrutura para armazenar resultado recebido
struct PredictionResult {
    int predicted_digit;
    float confidence;
    bool success;
    unsigned long timestamp;
};

PredictionResult ultimo_resultado = {-1, 0.0f, false, 0};

// Declarações de funções
bool connect_wifi();
void handle_client();
int extrair_int_json(const String& json, const String& key);
float extrair_float_json(const String& json, const String& key);
bool extrair_bool_json(const String& json, const String& key);
void processar_resultado(int digit, float confidence);
String criar_pagina_web();

// Conectar ao WiFi
bool connect_wifi() {
    Serial.println("=== Conectando ao WiFi ===");
    Serial.printf("SSID: %s\n", ssid);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ WiFi conectado!");
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Porta: %d\n", serverPort);
        Serial.println("============================\n");
        return true;
    } else {
        Serial.println("\n✗ Falha na conexão WiFi!");
        return false;
    }
}

// Extrair valor inteiro do JSON
int extrair_int_json(const String& json, const String& key) {
    String search = "\"" + key + "\":";
    int start = json.indexOf(search);
    if (start == -1) return -1;
    
    start += search.length();
    while (start < json.length() && json.charAt(start) == ' ') start++;
    
    int end = json.indexOf(',', start);
    if (end == -1) end = json.indexOf('}', start);
    if (end == -1) end = json.indexOf('\n', start);
    
    String value = json.substring(start, end);
    value.trim();
    return value.toInt();
}

// Extrair valor float do JSON
float extrair_float_json(const String& json, const String& key) {
    String search = "\"" + key + "\":";
    int start = json.indexOf(search);
    if (start == -1) return -1.0f;
    
    start += search.length();
    while (start < json.length() && json.charAt(start) == ' ') start++;
    
    int end = json.indexOf(',', start);
    if (end == -1) end = json.indexOf('}', start);
    if (end == -1) end = json.indexOf('\n', start);
    
    String value = json.substring(start, end);
    value.trim();
    return value.toFloat();
}

// Extrair valor booleano do JSON
bool extrair_bool_json(const String& json, const String& key) {
    String search = "\"" + key + "\":";
    int start = json.indexOf(search);
    if (start == -1) return false;
    
    start += search.length();
    while (start < json.length() && json.charAt(start) == ' ') start++;
    
    int end = json.indexOf(',', start);
    if (end == -1) end = json.indexOf('}', start);
    
    String value = json.substring(start, end);
    value.trim();
    value.toLowerCase();
    return (value == "true");
}

// Processar resultado recebido
void processar_resultado(int digit, float confidence) {
    Serial.println("\n╔════════════════════════════════╗");
    Serial.println("║   RESULTADO PROCESSADO         ║");
    Serial.println("╚════════════════════════════════╝");
    Serial.printf("  Dígito predito: %d\n", digit);
    Serial.printf("  Confiança: %.2f%%\n", confidence * 100);
    Serial.printf("  Timestamp: %lu ms\n", millis());
    Serial.println("─────────────────────────────────\n");
    
    // Aqui você pode adicionar qualquer lógica personalizada:
    // - Salvar em SD card
    // - Enviar via Serial para outro dispositivo
    // - Controlar atuadores
    // - Enviar para cloud (AWS, Firebase, etc.)
    // - Acionar alarme se confiança baixa
    
    if (confidence < 0.5f) {
        Serial.println("⚠️  AVISO: Confiança baixa na predição!");
    }
}

// Criar página web para visualização
String criar_pagina_web() {
    String html = "<!DOCTYPE html><html lang='pt-BR'>";
    html += "<head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<meta http-equiv='refresh' content='5'>";  // Auto-refresh a cada 5 segundos
    html += "<title>ESP32 - Receptor MNIST</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
    html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
    html += "h1 { color: #333; text-align: center; }";
    html += ".status { padding: 15px; margin: 20px 0; border-radius: 5px; }";
    html += ".success { background: #d4edda; border-left: 4px solid #28a745; }";
    html += ".waiting { background: #fff3cd; border-left: 4px solid #ffc107; }";
    html += ".info { display: flex; justify-content: space-between; padding: 10px; background: #f8f9fa; margin: 10px 0; border-radius: 5px; }";
    html += ".digit { font-size: 72px; text-align: center; color: #007bff; font-weight: bold; margin: 20px 0; }";
    html += ".confidence { text-align: center; font-size: 24px; color: #28a745; }";
    html += ".footer { text-align: center; color: #666; font-size: 12px; margin-top: 20px; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>🔢 Receptor MNIST</h1>";
    
    if (ultimo_resultado.success) {
        html += "<div class='status success'>";
        html += "<strong>✓ Último resultado recebido:</strong><br>";
        
        unsigned long tempo_decorrido = (millis() - ultimo_resultado.timestamp) / 1000;
        html += "Há " + String(tempo_decorrido) + " segundos";
        html += "</div>";
        
        html += "<div class='digit'>" + String(ultimo_resultado.predicted_digit) + "</div>";
        html += "<div class='confidence'>Confiança: " + String(ultimo_resultado.confidence * 100, 1) + "%</div>";
        
        html += "<div class='info'><span>Dígito:</span><strong>" + String(ultimo_resultado.predicted_digit) + "</strong></div>";
        html += "<div class='info'><span>Confiança:</span><strong>" + String(ultimo_resultado.confidence, 6) + "</strong></div>";
        html += "<div class='info'><span>Timestamp:</span><strong>" + String(ultimo_resultado.timestamp) + " ms</strong></div>";
        
    } else {
        html += "<div class='status waiting'>";
        html += "<strong>⏳ Aguardando dados...</strong><br>";
        html += "Nenhuma predição recebida ainda.";
        html += "</div>";
    }
    
    html += "<div class='info'><span>IP:</span><strong>" + WiFi.localIP().toString() + "</strong></div>";
    html += "<div class='info'><span>Heap livre:</span><strong>" + String(ESP.getFreeHeap()) + " bytes</strong></div>";
    html += "<div class='info'><span>Uptime:</span><strong>" + String(millis() / 1000) + " segundos</strong></div>";
    
    html += "<div class='footer'>Página atualiza automaticamente a cada 5 segundos</div>";
    html += "</div></body></html>";
    
    return html;
}

// Lidar com clientes HTTP
void handle_client() {
    WiFiClient client = server.available();
    if (!client) return;
    
    Serial.println("=== Cliente conectado ===");
    
    client.setTimeout(5000);
    
    String request = "";
    String headers = "";
    String body = "";
    bool reading_body = false;
    int content_length = 0;
    
    unsigned long start_time = millis();
    const unsigned long timeout_ms = 10000;
    
    // Ler headers
    while (client.connected() && (millis() - start_time < timeout_ms)) {
        if (!client.available()) {
            delay(1);
            continue;
        }
        
        String line = client.readStringUntil('\n');
        line.trim();
        
        if (!reading_body) {
            if (line.length() == 0) {
                reading_body = true;
                break;
            }
            
            if (request.length() == 0) {
                request = line;
            }
            
            headers += line + "\n";
            
            if (line.startsWith("Content-Length:")) {
                content_length = line.substring(15).toInt();
            }
        }
    }
    
    // Ler body
    if (content_length > 0 && content_length < 50000) {
        body.reserve(content_length + 100);
        
        unsigned long body_start = millis();
        while (body.length() < content_length && client.connected() && 
               (millis() - body_start < 5000)) {
            if (client.available()) {
                char c = client.read();
                body += c;
            } else {
                delay(1);
            }
        }
    }
    
    Serial.println("Requisição: " + request);
    
    String response_body = "";
    String content_type = "text/html";
    
    // Processar requisição
    if (request.startsWith("POST /receive")) {
        content_type = "application/json";
        
        Serial.println("Body recebido: " + body);
        
        // Extrair valores do JSON
        int digit = extrair_int_json(body, "predicted_digit");
        float confidence = extrair_float_json(body, "confidence");
        bool success = extrair_bool_json(body, "success");
        
        if (success && digit >= 0 && digit <= 9) {
            Serial.println("\n=== DADOS VÁLIDOS RECEBIDOS ===");
            
            // Atualizar último resultado
            ultimo_resultado.predicted_digit = digit;
            ultimo_resultado.confidence = confidence;
            ultimo_resultado.success = true;
            ultimo_resultado.timestamp = millis();
            
            // Processar resultado
            processar_resultado(digit, confidence);
            
            // Resposta de sucesso
            response_body = "{\"status\":\"received\",\"message\":\"Dados processados com sucesso\"}";
        } else {
            Serial.println("✗ Dados inválidos recebidos");
            response_body = "{\"status\":\"error\",\"message\":\"Dados inválidos\"}";
        }
        
    } else if (request.startsWith("GET /status")) {
        content_type = "application/json";
        
        response_body = "{";
        response_body += "\"last_prediction\":" + String(ultimo_resultado.predicted_digit) + ",";
        response_body += "\"confidence\":" + String(ultimo_resultado.confidence, 6) + ",";
        response_body += "\"has_data\":" + String(ultimo_resultado.success ? "true" : "false") + ",";
        response_body += "\"uptime\":" + String(millis() / 1000) + ",";
        response_body += "\"heap_free\":" + String(ESP.getFreeHeap());
        response_body += "}";
        
    } else {
        // Página web de visualização
        response_body = criar_pagina_web();
    }
    
    // Enviar resposta
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: " + content_type);
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println("Content-Length: " + String(response_body.length()));
    client.println();
    client.print(response_body);
    client.flush();
    
    delay(100);
    client.stop();
    Serial.println("Cliente desconectado\n");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║  ESP32 - RECEPTOR MNIST           ║");
    Serial.println("╚════════════════════════════════════╝");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    
    // Conectar WiFi
    if (!connect_wifi()) {
        Serial.println("Falha na conexão WiFi - reiniciando...");
        delay(5000);
        ESP.restart();
    }
    
    // Iniciar servidor
    server.begin();
    Serial.println("=== Servidor HTTP iniciado ===");
    Serial.println("Endpoints:");
    Serial.println("  POST /receive - Receber predições");
    Serial.println("  GET /status   - Status JSON");
    Serial.println("  GET /         - Página web");
    Serial.println("================================\n");
    Serial.println("✓ Pronto para receber dados!");
}

void loop() {
    // Verificar conexão WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi desconectado - reconectando...");
        if (!connect_wifi()) {
            delay(5000);
            return;
        }
    }
    
    // Lidar com clientes
    handle_client();
    
    delay(10);
}
