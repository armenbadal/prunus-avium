#include "codegensi.hxx"

#include <fstream>
#include <string>

namespace {

std::string toSiType(Type& type)
{
    if( auto& s = dynamic_cast<ScalarType&>(type) ) {
        return [&s] {
            switch( s._name ) {
                case ScalarType::Name::Real:
                    return "double";
                case ScalarType::Name::Text:
                    return "char*";
                case ScalarType::Name::Bool:
                    return "bool";
            }
        }();
    }
    else if( auto& a = dynamic_cast<ArrayType&>(type) ) {
        auto b = toSiType(a._base);
        
    }
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

void CodeGeneratorSi::visit(Subroutine& s)
{
    std::string returnType{"void"};
    if( s._returnType != nullptr ) {
        returnType = [&] {
            switch( s._returnType->_name ) {
                case ScalarType::Name::Real:
                    return "double";
                case ScalarType::Name::Text:
                    return "char*";
                case ScalarType::Name::Bool:
                    return "bool";
            }
        }();
    }

    _out << returnType << ' ' << s._name << "( ";
    for( auto& p : s._parameters )
        _out << p->_name << ' ';
    _out << ')';
}

} // namespace avium
