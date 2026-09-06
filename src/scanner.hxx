#pragma once

#include "lexeme.hxx"

#include <istream>

namespace avium {

// Բառային վերլուծիչ։ Նոր տողերը վերադարձվում են առանձին Lexeme-ներով,
// քանի որ դրանք Կեռասի շարահյուսության նշանակալի տարրեր են։
class Scanner {
public:
    explicit Scanner(std::istream& source);

    Lexeme scan();

private:
    Lexeme scanNumber();
    Lexeme scanText();
    Lexeme scanIdentifier();

    std::istream& _source;
    unsigned int _line{1};
};

} // namespace avium
