#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Cartridge {
public:
    explicit Cartridge(const std::string& path);

    bool loaded() const;
    const std::vector<std::uint8_t>& prg() const;
    const std::vector<std::uint8_t>& chr() const;

private:
    void load_from_file(const std::string& path);

    std::vector<std::uint8_t> prg_;
    std::vector<std::uint8_t> chr_;
    bool loaded_ = false;
};
