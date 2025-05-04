#include <string>
#include <iostream>

#include "base/json.hpp"

using json = nlohmann::json;

int main() {
    std::string test_string = R"(
    {
        "pi": 3.141,
        "happy": true
    }
    )";

    json j;
    j = json::parse(test_string);

    // dump to string
    std::cout << j.dump(4) << std::endl;

    // Accessing values
    std::cout << "pi: " << j["pi"] << std::endl;
    std::cout << "happy: " << j["happy"] << std::endl;

    // Modifying values
    j["pi"] = 0;
    j["happy"] = false;
    std::cout << "Modified pi: " << j["pi"] << std::endl;
    std::cout << "Modified happy: " << j["happy"] << std::endl;

    // Serializing to string
    std::string serialized = j.dump();
    std::cout << "Serialized JSON: " << serialized << std::endl;
}
