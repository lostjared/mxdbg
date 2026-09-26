#include "mxdbg/ai_config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <memory>

namespace {
    std::string environment_value(const char* name) {
        const char* value = std::getenv(name);
        return value == nullptr ? std::string{} : std::string(value);
    }

    std::string lowercase(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
        return value;
    }
}

namespace mx {
    std::optional<AiConfiguration> load_ai_configuration(std::string& error) {
        error.clear();
        AiConfiguration configuration;

        std::string provider = lowercase(environment_value("MXDBG_PROVIDER"));
        if (provider.empty() || provider == "ollama") {
            configuration.provider = Provider::Ollama;
            configuration.provider_name = "Ollama";
            configuration.host = environment_value("MXDBG_HOST");
            if (configuration.host.empty()) {
                configuration.host = "localhost";
            }
        } else if (provider == "openai") {
            configuration.provider = Provider::OpenAI;
            configuration.provider_name = "OpenAI";
            configuration.host = environment_value("MXDBG_BASE_URL");
            if (environment_value("OPENAI_API_KEY").empty()) {
                error = "OPENAI_API_KEY is required when MXDBG_PROVIDER=openai";
                return std::nullopt;
            }
        } else if (provider == "anthropic") {
            configuration.provider = Provider::Anthropic;
            configuration.provider_name = "Anthropic";
            configuration.host = environment_value("MXDBG_BASE_URL");
            if (environment_value("ANTHROPIC_API_KEY").empty()) {
                error = "ANTHROPIC_API_KEY is required when MXDBG_PROVIDER=anthropic";
                return std::nullopt;
            }
        } else {
            error = "unsupported MXDBG_PROVIDER '" + provider +
                    "' (expected ollama, openai, or anthropic)";
            return std::nullopt;
        }

        configuration.model = environment_value("MXDBG_MODEL");
        if (configuration.model.empty()) {
            error = "MXDBG_MODEL is required when AI integration is enabled";
            return std::nullopt;
        }
        return configuration;
    }

    std::unique_ptr<ObjectRequest> create_ai_request(const AiConfiguration& configuration) {
        if (configuration.provider == Provider::Ollama) {
            return std::make_unique<ObjectRequest>(configuration.host, configuration.model);
        }
        if (configuration.host.empty()) {
            return std::make_unique<ObjectRequest>(configuration.provider, configuration.model);
        }
        return std::make_unique<ObjectRequest>(
            configuration.provider, configuration.model, configuration.host);
    }
}
