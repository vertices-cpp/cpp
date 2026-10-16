#include <iostream>
#include <string>
#include <vector>
#include "ZipHelper.h"

static std::string readLine(const std::string& prompt)
{
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' '))
        s.pop_back();
    return s;
}

static unsigned char readXorKey(const std::string& prompt)
{
    std::string s = readLine(prompt);
    if (s.empty()) return 0;
    try { return (unsigned char)(std::stoi(s, nullptr, 0) & 0xFF); }
    catch (...) { return (unsigned char)s[0]; }
}

int main()
{
    while (true)
    {
        std::cout << "\n==============================\n";
        std::cout << "  1. Compress\n";
        std::cout << "  2. Decompress\n";
        std::cout << "  0. Exit\n";
        std::cout << "==============================\n";

        std::string c = readLine("Choice: ");
        if (c == "0") break;

        if (c == "1")
        {
            std::string src = readLine("Source dir: ");
            std::string dst = readLine("Output zip: ");
            std::string pwd = readLine("Password (Enter = none): ");
            unsigned char key = readXorKey("Xor key (Enter = none): ");

            std::vector<std::string> srcs = { src };
            if (zip::compress(srcs, dst, pwd, key))
                std::cout << "[OK] compress done\n";
            else
                std::cout << "[ERROR] compress failed\n";
        }
        else if (c == "2")
        {
            std::string zf  = readLine("Zip file: ");
            std::string out = readLine("Output dir: ");
            std::string pwd = readLine("Password (Enter = none): ");
            unsigned char key = readXorKey("Xor key (Enter = none): ");

            if (zip::decompress(zf, out, pwd, key))
                std::cout << "[OK] decompress done\n";
            else
                std::cout << "[ERROR] decompress failed\n";
        }
    }
    return 0;
}