#include "mxdbg/ai_config.hpp"

#include <cassert>
#include <cstdlib>
#include <string>

namespace {
    void setEnvironment(const char* name, const char* value) {
        const int result = setenv(name, value, 1);
        assert(result == 0);
    }

    void unsetEnvironment(const char* name) {
        const int result = unsetenv(name);
        assert(result == 0);
    }
}

int main() {
    std::string error;

    unsetEnvironment("MXDBG_PROVIDER");
    unsetEnvironment("MXDBG_HOST");
    unsetEnvironment("MXDBG_BASE_URL");
    setEnvironment("MXDBG_MODEL", "local-model");
    auto configuration = mx::load_ai_configuration(error);
    assert(configuration);
    assert(configuration->provider == mx::Provider::Ollama);
    assert(configuration->host == "localhost");
    assert(mx::create_ai_request(*configuration));

    setEnvironment("MXDBG_PROVIDER", "openai");
    unsetEnvironment("OPENAI_API_KEY");
    configuration = mx::load_ai_configuration(error);
    assert(!configuration);
    assert(error == "OPENAI_API_KEY is required when MXDBG_PROVIDER=openai");

    setEnvironment("OPENAI_API_KEY", "test-key");
    setEnvironment("MXDBG_BASE_URL", "https://example.invalid");
    configuration = mx::load_ai_configuration(error);
    assert(configuration);
    assert(configuration->provider == mx::Provider::OpenAI);
    assert(configuration->host == "https://example.invalid");
    assert(mx::create_ai_request(*configuration));

    setEnvironment("MXDBG_PROVIDER", "AnThRoPiC");
    unsetEnvironment("MXDBG_BASE_URL");
    setEnvironment("ANTHROPIC_API_KEY", "test-key");
    configuration = mx::load_ai_configuration(error);
    assert(configuration);
    assert(configuration->provider == mx::Provider::Anthropic);
    assert(configuration->host.empty());
    assert(mx::create_ai_request(*configuration));

    setEnvironment("MXDBG_PROVIDER", "unknown");
    configuration = mx::load_ai_configuration(error);
    assert(!configuration);
    assert(error.find("unsupported MXDBG_PROVIDER") != std::string::npos);
}
