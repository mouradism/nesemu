#include "mapper.hpp"
#include "mapper_000.hpp"
#include "mapper_001.hpp"
#include "mapper_002.hpp"
#include "mapper_003.hpp"

#include <iostream>

std::unique_ptr<Mapper> create_mapper(std::uint8_t mapper_id, 
                                       std::uint8_t prg_banks, 
                                       std::uint8_t chr_banks,
                                       MirrorMode initial_mirror) {
    std::unique_ptr<Mapper> mapper;
    
    switch (mapper_id) {
        case 0:
            mapper = std::make_unique<Mapper000>(prg_banks, chr_banks);
            break;
        case 1:
            mapper = std::make_unique<Mapper001>(prg_banks, chr_banks);
            break;
        case 2:
            mapper = std::make_unique<Mapper002>(prg_banks, chr_banks);
            break;
        case 3:
            mapper = std::make_unique<Mapper003>(prg_banks, chr_banks);
            break;
        default:
            std::cerr << "Unsupported mapper: " << static_cast<int>(mapper_id) 
                      << ", falling back to NROM\n";
            mapper = std::make_unique<Mapper000>(prg_banks, chr_banks);
            break;
    }
    
    mapper->set_mirror_mode(initial_mirror);
    return mapper;
}
