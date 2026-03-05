#include <iostream>
#include <regex>
#include <string>

int main(int argc, char const *argv[])
{
    std::string str = "Hello, World! 12345";
    std::regex re("\\d+");
    std::smatch match;
    if (std::regex_search(str, match, re))
    {
        std::cout << "Found a number: " << match.str() << std::endl;
    }
    return 0;
}
