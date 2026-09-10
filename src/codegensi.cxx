#include "codegensi.hxx"

#include <format>
#include <fstream>
#include <string>

namespace {

std::string temporatyName()
{
    static unsigned int index = 0;
    return std::format("avium_temp_{}", ++index);
}

std::string toSiType(avium::ScalarType& type)
{
    switch( type._name ) {
        case avium::ScalarType::Name::Real:
            return "double";
        case avium::ScalarType::Name::Text:
            return "char*";
        case avium::ScalarType::Name::Bool:
            return "bool";
    }
}

std::string toSiType(avium::ArrayType& type)
{
    return "array_descriptor*";
}

} // namespace

namespace avium {

CodeGeneratorSi::CodeGeneratorSi()
{}

CodeGeneratorSi::~CodeGeneratorSi()
{}

bool CodeGeneratorSi::generate(Program& program, const SymbolTable& symbols, const SemanticModel& model)
{
    visit(program);
    return true;
}

void CodeGeneratorSi::save(std::filesystem::path p)
{
    if( std::ofstream f{p}; f )
        f << _out.str() << '\n';
}

void CodeGeneratorSi::visit(Program& p)
{
    for( auto& subroutine : p._subroutines )    
        visit(*subroutine);
}

void CodeGeneratorSi::visit(Sequence& q)
{
    _out << "{\n";
    for( auto& s : q._items )
        visit(*s);
    _out << "\n}\n";
}

void CodeGeneratorSi::visit(Subroutine& s)
{
    std::string returnType{"void"};
    if( s._returnType != nullptr )
        returnType = toSiType(*s._returnType);

    _out << returnType << ' ' << s._name << "( ";
    for( auto& p : s._parameters )
        _out << p->_name << ' ';
    _out << ')';

    visit(*s._body);
}

void CodeGeneratorSi::visit(Dim& d)
{
}

} // namespace avium
