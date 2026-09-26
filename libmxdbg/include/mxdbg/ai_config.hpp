#ifndef MXDBG_AI_CONFIG_HPP
#define MXDBG_AI_CONFIG_HPP

#include <mx2-ollama.hpp>

#include <memory>
#include <optional>
#include <string>

namespace mx {
    struct AiConfiguration {
        Provider provider = Provider::Ollama;
        std::string provider_name = "Ollama";
        std::string model;
        std::string host;
    };

    // Reads MXDBG_PROVIDER, MXDBG_MODEL, and the provider-specific settings.
    // Returns no configuration and describes the problem in error when setup is incomplete.
    [[nodiscard]] std::optional<AiConfiguration> load_ai_configuration(std::string& error);

    [[nodiscard]] std::unique_ptr<ObjectRequest>
    create_ai_request(const AiConfiguration& configuration);
}

#endif
