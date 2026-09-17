#include "codegensi.hxx"

#include <fstream>
#include <format>
#include <iomanip>
#include <string>
#include <string_view>

namespace {

std::string temporaryName(std::string_view type)
{
    static unsigned int index = 0;
    return std::format("avium_temp_{}_{}", type, ++index);
}

bool producesOwnedText(const avium::Expression& expression)
{
    if( expression.kind == avium::NodeKind::Apply )
        return true;
    if( expression.kind != avium::NodeKind::Binary )
        return false;

    const auto& binary = static_cast<const avium::Binary&>(expression);
    return binary._operation == avium::Operation::Conc;
}

} // namespace

namespace avium {

CodeGeneratorSi::CodeGeneratorSi(Program& program, const SemanticModel& model)
    : _program{program}
    , _model{model}
{}

CodeGeneratorSi::~CodeGeneratorSi()
{}

void CodeGeneratorSi::emitIncludes(std::ostream& output) const
{
    output << "#include \"arrays.h\"\n";
    output << "#include \"io.h\"\n";
    output << "#include \"texts.h\"\n\n";
}

void CodeGeneratorSi::emitTextLiterals(std::ostream& output) const
{
    output << "static const avium_text avium_empty_text = {\"\", 0, false};\n";
    for( const auto& [text, name] : _textLiterals ) {
        output << "static const avium_text " << name << " = {"
               << std::quoted(text) << ", " << text.size() << ", false};\n";
    }
    output << '\n';
}

void CodeGeneratorSi::emitDefaultValue(const Type& type)
{
    switch( type.base()._name ) {
        case ScalarType::Name::Text:
            _out << "avium_empty_text";
            break;
        case ScalarType::Name::Real:
            _out << "0.0";
            break;
        case ScalarType::Name::Bool:
            _out << "false";
            break;
    }
}

void CodeGeneratorSi::emitArguments(const std::vector<Expression::Ptr>& arguments)
{
    for( std::size_t index = 0; index < arguments.size(); ++index ) {
        if( index != 0 )
            _out << ", ";
        visit(*arguments[index]);
    }
}

void CodeGeneratorSi::emitStoredText(Expression& expression, Position line)
{
    const auto needsCopy = !producesOwnedText(expression);
    if( needsCopy )
        _out << "avium_text_copy(";
    visit(expression);
    if( needsCopy )
        _out << ", " << line << ')';
}

void CodeGeneratorSi::emitTextAssignment(Let& statement)
{
    const auto temporary = temporaryName("text");
    _out << "{\navium_text " << temporary << " = ";
    emitStoredText(*statement._value, statement.line);
    _out << ";\navium_text_move_assign(";

    if( statement._index ) {
        _out << "avium_text_array_at(";
        visit(*statement._variable);
        _out << ", ";
        visit(*statement._index);
        _out << ", " << statement.line << ')';
    }
    else {
        _out << '&';
        visit(*statement._variable);
    }

    _out << ", &" << temporary << ");\n}\n";
}

void CodeGeneratorSi::emitLocalCleanup()
{
    for( auto object = _localObjects.rbegin(); object != _localObjects.rend(); ++object ) {
        if( object->kind == LocalKind::Text )
            _out << "avium_text_destroy(&" << object->name << ");\n";
        else
            _out << "avium_array_destroy(" << object->name << ");\n";
    }
}

bool CodeGeneratorSi::generate(std::filesystem::path p)
{
    _out.str({});
    _out.clear();
    _textLiterals.clear();
    visit(_program);

    std::ofstream f{p};
    if( !f )
        return false;

    emitIncludes(f);
    emitTextLiterals(f);
    f << _out.str();
    f << "\nint main(){ avium_Main(); return 0; }\n";

    return !f.fail();
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
    _localObjects.clear();

    if( s._returnType )
        visit(*s._returnType);
    else
        _out << "void";

    _out << " avium_" << s._name << '(';
    for( std::size_t index = 0; index < s._parameters.size(); ++index ) {
        if( index != 0 )
            _out << ", ";
        const auto& parameter = s._parameters[index];
        visit(*parameter->_type);
        _out << ' ' << parameter->_name;
    }
    _out << ") {\n";
    for( auto& statement : s._body->_items )
        visit(*statement);
    if( !s._returnType )
        emitLocalCleanup();
    _out << "\n}\n\n";
}

void CodeGeneratorSi::visit(Dim& d)
{
    if( d._type->isArray() ) {
        const auto& t = static_cast<const ArrayType&>(*d._type);
        _out << "avium_array* " << d._name << " = avium_array_create(";
        switch( t._base->_name ) {
            case ScalarType::Name::Text: _out << "AVIUM_ARRAY_TEXT"; break;
            case ScalarType::Name::Real: _out << "AVIUM_ARRAY_REAL"; break;
            case ScalarType::Name::Bool: _out << "AVIUM_ARRAY_BOOL"; break;
        }
        _out << ", ";
        visit(*t._size);
        _out << ", " << d.line << ");";
        _localObjects.push_back({d._name, LocalKind::Array});
    }
    else {
        visit(static_cast<ScalarType&>(*d._type));
        _out << ' ' << d._name << " = ";
        emitDefaultValue(*d._type);
        _out << ';';
        if( d._type->base()._name == ScalarType::Name::Text )
            _localObjects.push_back({d._name, LocalKind::Text});
    }
    _out << '\n';
}

void CodeGeneratorSi::visit(Let& l)
{
    const auto& target = _model.type(l._variable->id());
    const auto& base = target->base();
    if( base._name == ScalarType::Name::Text ) {
        emitTextAssignment(l);
        return;
    }

    if( l._index ) {
        if( base._name == ScalarType::Name::Real )
            _out << "*avium_real_array_at(";
        else
            _out << "*avium_bool_array_at(";
        visit(*l._variable);
        _out << ", ";
        visit(*l._index);
        _out << ", " << l.line << ')';
        _out << " = ";
        visit(*l._value);
        _out << ";\n";
        return;
    }

    visit(*l._variable);
    _out << " = ";
    visit(*l._value);
    _out << ";\n";
}

void CodeGeneratorSi::visit(If&) {}

void CodeGeneratorSi::visit(IfBranch&) {}

void CodeGeneratorSi::visit(While&) {}

void CodeGeneratorSi::visit(For&) {}

void CodeGeneratorSi::visit(Call&) {}

void CodeGeneratorSi::visit(Return& statement)
{
    const auto type = _model.type(statement._value->id());
    const auto& base = type->base();
    const auto temporary = temporaryName("return");

    switch( base._name ) {
        case ScalarType::Name::Real:
            _out << "double ";
            break;
        case ScalarType::Name::Text:
            _out << "avium_text ";
            break;
        case ScalarType::Name::Bool:
            _out << "bool ";
            break;
    }
    _out << temporary << " = ";
    if( base._name == ScalarType::Name::Text )
        emitStoredText(*statement._value, statement.line);
    else
        visit(*statement._value);
    _out << ";\n";

    emitLocalCleanup();
    _out << "return " << temporary << ";\n";
}

void CodeGeneratorSi::visit(Apply& a)
{
    if( a._callee == "Input" ) {
        _out << "avium_input(" << a.line << ')';
        return;
    }
    if( a._callee == "NUM" ) {
        _out << "avium_num(";
        visit(*a._arguments.front());
        _out << ", " << a.line << ')';
        return;
    }
    if( a._callee == "STR" ) {
        _out << "avium_str(";
        visit(*a._arguments.front());
        _out << ", " << a.line << ')';
        return;
    }
    if( a._callee == "LEN" ) {
        const auto argumentType = _model.type(a._arguments.front()->id());
        _out << (argumentType != nullptr && argumentType->isArray()
                ? "avium_array_length("
                : "avium_text_length(");
        visit(*a._arguments.front());
        _out << ')';
        return;
    }
    _out << "avium_" << a._callee << '(';
    emitArguments(a._arguments);
    _out << ')';
}

void CodeGeneratorSi::visit(ScalarType& t)
{
    switch( t._name ) {
        case avium::ScalarType::Name::Real:
            _out << "double";
            break;
        case avium::ScalarType::Name::Text:
            _out << "avium_text";
            break;
        case avium::ScalarType::Name::Bool:
            _out << "bool";
            break;
    }
}

void CodeGeneratorSi::visit(ArrayType&)
{
    _out << "avium_array*";
}

void CodeGeneratorSi::visit(Binary& b)
{
    if( b._operation == Operation::Index ) {
        const auto& arrayType = _model.type(b._left->id());
        const auto& base = arrayType->base();
        if( base._name == ScalarType::Name::Text )
            _out << "*avium_text_array_at(";
        else if( base._name == ScalarType::Name::Real )
            _out << "*avium_real_array_at(";
        else
            _out << "*avium_bool_array_at(";
        visit(*b._left);
        _out << ", ";
        visit(*b._right);
        _out << ", " << b.line << ')';
        return;
    }

    _out << '(';
    visit(*b._left);
    switch( b._operation ) {
        case Operation::Add:
            _out << " + ";
            break;
        case Operation::Sub:
            _out << " - ";
            break;
        case Operation::Mul:
            _out << " * ";
            break;
        case Operation::Div:
            _out << " / ";
            break;
        case Operation::Eq:
            _out << " == ";
            break;
        case Operation::Ne:
            _out << " != ";
            break;
        case Operation::Lt:
            _out << " < ";
            break;
        case Operation::Le:
            _out << " <= ";
            break;
        case Operation::Gt:
            _out << " > ";
            break;
        case Operation::Ge:
            _out << " >= ";
            break;
    }
    visit(*b._right);
    _out << ')';
}

void CodeGeneratorSi::visit(Unary& u)
{
    if( u._operation == Operation::Not )
        _out << '!';
    else if( u._operation == Operation::Sub )
        _out << '-';
    else if( u._operation == Operation::Add )
        _out << '+';
    _out << '(';
    visit(*u._operand);
    _out << ')';
}

void CodeGeneratorSi::visit(Variable& v)
{
    _out << v._name;
}

void CodeGeneratorSi::visit(Text& t)
{
    if( t._value.empty() ) {
        _out << "avium_empty_text";
        return;
    }

    auto [literal, inserted] = _textLiterals.try_emplace(t._value);
    if( inserted )
        literal->second = temporaryName("text");
    _out << literal->second;
}

void CodeGeneratorSi::visit(Number& n)
{
    _out << n._value;
}

void CodeGeneratorSi::visit(Boolean& b)
{
    _out << (b._value ? "true" : "false");
}

} // namespace avium
