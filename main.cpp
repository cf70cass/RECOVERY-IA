#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

using json = nlohmann::json;

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* user_data) {
    const size_t total = size * nmemb;
    auto* output = static_cast<std::string*>(user_data);
    output->append(static_cast<char*>(contents), total);
    return total;
}

static bool http_request(const std::string& url,
                         const std::string* post_body,
                         std::string& response,
                         long& http_status) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Não foi possível inicializar libcurl.\n";
        return false;
    }

    response.clear();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 35L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

    if (post_body != nullptr) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body->c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(post_body->size()));
    }

    const CURLcode result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);

    if (result != CURLE_OK) {
        std::cerr << "Falha de conexão HTTPS: " << curl_easy_strerror(result) << "\n";
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_cleanup(curl);
    return true;
}

static std::string url_encode(CURL* curl, const std::string& value) {
    char* encoded = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.size()));
    if (!encoded) return "";
    std::string result(encoded);
    curl_free(encoded);
    return result;
}

static bool telegram_call(const std::string& token,
                          const std::string& method,
                          const std::string* post_body,
                          json& result) {
    const std::string url = "https://api.telegram.org/bot" + token + "/" + method;
    std::string response;
    long status = 0;

    if (!http_request(url, post_body, response, status)) return false;

    try {
        result = json::parse(response);
    } catch (const std::exception& e) {
        std::cerr << "Resposta JSON inválida (HTTP " << status << ").\n";
        return false;
    }

    if (status < 200 || status >= 300 || !result.value("ok", false)) {
        std::cerr << "Telegram API retornou erro (HTTP " << status << "): "
                  << result.value("description", "erro não especificado") << "\n";
        return false;
    }
    return true;
}

static bool send_message(const std::string& token,
                         long long chat_id,
                         const std::string& text) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    const std::string body =
        "chat_id=" + url_encode(curl, std::to_string(chat_id)) +
        "&text=" + url_encode(curl, text);

    curl_easy_cleanup(curl);

    json response;
    return telegram_call(token, "sendMessage", &body, response);
}

static std::string command_name(const std::string& text) {
    const auto space = text.find_first_of(" \t\r\n");
    std::string command = text.substr(0, space);
    const auto at = command.find('@');
    if (at != std::string::npos) command.resize(at);
    return command;
}

static void handle_message(const std::string& token, const json& message) {
    if (!message.contains("chat") || !message["chat"].contains("id") ||
        !message.contains("text") || !message["text"].is_string()) {
        return;
    }

    if (message.contains("from") && message["from"].value("is_bot", false)) {
        return;
    }

    const long long chat_id = message["chat"]["id"].get<long long>();
    const std::string text = message["text"].get<std::string>();
    const std::string command = command_name(text);

    std::cout << "Mensagem recebida no chat " << chat_id << "\n";

    if (command == "/start") {
        send_message(token, chat_id,
            "Olá! Eu sou o RECOVERY-IA.\n\n"
            "Envie /help para ver os comandos disponíveis.");
    } else if (command == "/help") {
        send_message(token, chat_id,
            "RECOVERY-IA — comandos\n\n"
            "/start - iniciar o bot\n"
            "/help - mostrar esta ajuda\n\n"
            "Você também pode enviar um endereço HTTPS para receber "
            "uma confirmação de recebimento. Os módulos de análise serão "
            "integrados nas próximas etapas.");
    } else if (text.rfind("https://", 0) == 0 || text.rfind("http://", 0) == 0) {
        send_message(token, chat_id,
            "Endereço recebido:\n" + text +
            "\n\nNesta versão, o bot apenas recebe o link. "
            "A análise real será adicionada nos próximos módulos.");
    } else {
        send_message(token, chat_id, "Não reconheci esse comando. Envie /help.");
    }
}

int main() {
    const char* token_env = std::getenv("TELEGRAM_TOKEN");
    if (token_env == nullptr || std::string(token_env).empty()) {
        std::cerr << "ERRO: a variável de ambiente TELEGRAM_TOKEN não foi configurada.\n";
        return 1;
    }

    const std::string token(token_env);
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        std::cerr << "Não foi possível inicializar libcurl.\n";
        return 1;
    }

    std::cout << "====================================\n"
              << "          RECOVERY-IA BOT\n"
              << "====================================\n"
              << "Iniciando conexão com o Telegram...\n" << std::flush;

    // getMe valida o token antes de iniciar o polling.
    json me;
    if (!telegram_call(token, "getMe", nullptr, me)) {
        std::cerr << "Não foi possível validar o bot. Confira o token e a conexão.\n";
        curl_global_cleanup();
        return 1;
    }

    std::cout << "Conectado como @" << me["result"].value("username", "bot")
              << ". Aguardando mensagens...\n" << std::flush;

    long long offset = 0;

    while (true) {
        const std::string method =
            "getUpdates?timeout=25&offset=" + std::to_string(offset);

        json updates;
        if (!telegram_call(token, method, nullptr, updates)) {
            std::cerr << "Nova tentativa em 5 segundos. Verifique se há webhook ativo "
                         "ou outra instância do bot.\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        if (!updates.contains("result") || !updates["result"].is_array()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        for (const auto& update : updates["result"]) {
            if (!update.contains("update_id")) continue;
            const long long update_id = update["update_id"].get<long long>();
            if (update_id >= offset) offset = update_id + 1;

            if (update.contains("message")) {
                handle_message(token, update["message"]);
            }
        }
    }

    curl_global_cleanup();
    return 0;
}
