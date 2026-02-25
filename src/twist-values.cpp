
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>
#include <ostream>
#include <any>
#include <iostream>
#include <cassert>
#pragma once

using namespace std;

#define NUMBER_ACCURACY long double


struct Type;
using TypePtr = shared_ptr<const Type>;


struct PrimitiveType {
    string name;
};

struct PointerType {
    TypePtr pointee;
};

struct ArrayType {
    TypePtr element_type;
    optional<size_t> size;  // nullopt означает динамический размер
};

struct FunctionType {
    vector<TypePtr> arg_types;
    TypePtr return_type;  
};

struct UnionType {
    vector<TypePtr> components;
};

struct AutoType {};        // для "auto"
struct PointerAutoType {}; // для "*auto"



inline TypePtr make_type_ptr(const Type& t);



struct Type {

    string pool;

    
    Type() : m_data(monostate{}) {}
    

    Type(const string& str) {
        parse_from_string(str);
    }
    

    explicit Type(PrimitiveType pt) : m_data(std::move(pt)) { update_pool(); }
    explicit Type(PointerType pt) : m_data(std::move(pt)) { update_pool(); }
    explicit Type(ArrayType at) : m_data(std::move(at)) { update_pool(); }
    explicit Type(FunctionType ft) : m_data(std::move(ft)) { update_pool(); }
    explicit Type(UnionType ut) : m_data(std::move(ut)) { update_pool(); }

 
    Type(const Type&) = default;
    Type(Type&&) = default;
    Type& operator=(const Type&) = default;
    Type& operator=(Type&&) = default;

  
    bool operator==(const Type& other) const {
        return compare(*this, other);
    }
    bool operator!=(const Type& other) const {
        return !(*this == other);
    }

 
    bool is_sub_type(const Type& other) const;
    bool is_union_type() const;
    bool is_array_type() const;
    bool is_func() const;
    bool is_pointer() const;
    
    // Парсинг массива (возвращает строки)
    pair<string, string> parse_array_type() const;
    
    // Парсинг функции (возвращает строки)
    pair<vector<Type>, Type> parse_func_type() const;
    
    // Разбиение union на компоненты (возвращает строки)
    vector<string> split_union_components() const;
    
    // Оператор объединения
    Type operator|(const Type& other) const;

    // Вспомогательные методы для внутреннего использования
    const auto& data() const { return m_data; }

public:
    // Внутреннее представление
    using Data = variant<
        PrimitiveType,
        PointerType,
        ArrayType,
        FunctionType,
        UnionType,
        AutoType,
        PointerAutoType,
        monostate
    >;
    Data m_data;

  
    void parse_from_string(const string& str);
    void update_pool();
    static bool compare(const Type& a, const Type& b);
    bool is_sub_type_impl(const Type& other, const UnionType* other_union = nullptr) const;
    
    // Получение компонентов union в виде TypePtr
    vector<TypePtr> get_union_components() const;
};

bool Type::compare(const Type& a, const Type& b) {
    return std::visit([](const auto& lhs, const auto& rhs) -> bool {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;
        
        if constexpr (!std::is_same_v<L, R>) {
            return false;
        } else {
            if constexpr (std::is_same_v<L, std::monostate>) {
                return true;
            }
            else if constexpr (std::is_same_v<L, PrimitiveType>) {
                return lhs.name == rhs.name;
            }
            else if constexpr (std::is_same_v<L, PointerType>) {
                return compare(*lhs.pointee, *rhs.pointee);
            }
            else if constexpr (std::is_same_v<L, ArrayType>) {
                if (!compare(*lhs.element_type, *rhs.element_type)) return false;
                if (lhs.size.has_value() != rhs.size.has_value()) return false;
                if (lhs.size && *lhs.size != *rhs.size) return false;
                return true;
            }
            else if constexpr (std::is_same_v<L, FunctionType>) {
                if (lhs.arg_types.size() != rhs.arg_types.size()) return false;
                for (size_t i = 0; i < lhs.arg_types.size(); ++i) {
                    if (!compare(*lhs.arg_types[i], *rhs.arg_types[i])) return false;
                }
                if (lhs.return_type && rhs.return_type) {
                    return compare(*lhs.return_type, *rhs.return_type);
                } else {
                    return !lhs.return_type && !rhs.return_type;
                }
            }
            else if constexpr (std::is_same_v<L, UnionType>) {
                if (lhs.components.size() != rhs.components.size()) return false;
                for (size_t i = 0; i < lhs.components.size(); ++i) {
                    if (!compare(*lhs.components[i], *rhs.components[i])) return false;
                }
                return true;
            }
            else {
                return false;
            }
        }
    }, a.m_data, b.m_data);
}



inline TypePtr make_type_ptr(const Type& t) {
    return make_shared<Type>(t);
}

void Type::update_pool() {
    struct Visitor {
        string operator()(AutoType) const { return "auto"; }
        string operator()(PointerAutoType) const { return "*auto"; }
        string operator()(monostate) const { return ""; }
        string operator()(const PrimitiveType& pt) const { return pt.name; }
        string operator()(const PointerType& pt) const { return "*" + pt.pointee->pool; }
        string operator()(const ArrayType& at) const {
            string elem = at.element_type ? at.element_type->pool : "";
            if (at.size) {
                return "[" + elem + ", " + to_string(*at.size) + "]";
            } else {
                return "[" + elem + ", ~]";
            }
        }
        string operator()(const FunctionType& ft) const {
            string args;
            for (size_t i = 0; i < ft.arg_types.size(); ++i) {
                if (i > 0) args += ", ";
                args += ft.arg_types[i]->pool;
            }
            if (ft.return_type) {
                return "(Func(" + args + ") -> " + ft.return_type->pool + ")";
            } else {
                return "Func(" + args + ")";
            }
        }
        string operator()(const UnionType& ut) const {
            string res;
            for (size_t i = 0; i < ut.components.size(); ++i) {
                if (i > 0) res += " | ";
                res += ut.components[i]->pool;
            }
            return res;
        }
    };
    pool = visit(Visitor(), m_data);
}

static bool compare(const Type& a, const Type& b) {
    return std::visit([](const auto& lhs, const auto& rhs) -> bool {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;
        
        if constexpr (!std::is_same_v<L, R>) {
            return false;
        } else {
            if constexpr (std::is_same_v<L, std::monostate>) {
                return true;
            }
            else if constexpr (std::is_same_v<L, PrimitiveType>) {
                return lhs.name == rhs.name;
            }
            else if constexpr (std::is_same_v<L, AutoType>) {
                return true; // два AutoType всегда равны
            }
            else if constexpr (std::is_same_v<L, PointerAutoType>) {
                return true;
            }
            else if constexpr (std::is_same_v<L, PointerType>) {
                // pointee никогда не должен быть nullptr
                return compare(*lhs.pointee, *rhs.pointee);
            }
            else if constexpr (std::is_same_v<L, ArrayType>) {
                if (!compare(*lhs.element_type, *rhs.element_type)) return false;
                if (lhs.size.has_value() != rhs.size.has_value()) return false;
                if (lhs.size && *lhs.size != *rhs.size) return false;
                return true;
            }
            else if constexpr (std::is_same_v<L, FunctionType>) {
                if (lhs.arg_types.size() != rhs.arg_types.size()) return false;
                for (size_t i = 0; i < lhs.arg_types.size(); ++i) {
                    if (!compare(*lhs.arg_types[i], *rhs.arg_types[i])) return false;
                }
                // Обработка return_type (может быть nullptr)
                if (lhs.return_type && rhs.return_type) {
                    return compare(*lhs.return_type, *rhs.return_type);
                } else {
                    return !lhs.return_type && !rhs.return_type;
                }
            }
            else if constexpr (std::is_same_v<L, UnionType>) {
                if (lhs.components.size() != rhs.components.size()) return false;
                for (size_t i = 0; i < lhs.components.size(); ++i) {
                    if (!compare(*lhs.components[i], *rhs.components[i])) return false;
                }
                return true;
            }
            else {
                return false; // Неизвестный тип
            }
        }
    }, a.m_data, b.m_data);
}

bool Type::is_union_type() const {
    return holds_alternative<UnionType>(m_data);
}

bool Type::is_array_type() const {
    return holds_alternative<ArrayType>(m_data);
}

bool Type::is_func() const {
    return holds_alternative<FunctionType>(m_data);
}

bool Type::is_pointer() const {
    // Если это прямой указатель
    if (holds_alternative<PointerType>(m_data)) return true;
    
    // Если это объединение (union)
    if (holds_alternative<UnionType>(m_data)) {
        const auto& u = get<UnionType>(m_data);
        // Проверяем, что все компоненты являются указателями
        for (const auto& comp : u.components) {
            if (!comp->is_pointer()) return false;
        }
        
        return !u.components.empty();
    }
    
    return false;
}

pair<string, string> Type::parse_array_type() const {
    if (!is_array_type()) return {"", ""};
    const auto& arr = get<ArrayType>(m_data);
    string elem = arr.element_type ? arr.element_type->pool : "";
    string size = arr.size ? to_string(*arr.size) : "~";
    return {elem, size};
}

pair<vector<Type>, Type> Type::parse_func_type() const {
    vector<Type> args;
    Type ret;
    if (!is_func()) return {args, ret};
    const auto& func = get<FunctionType>(m_data);
    for (const auto& a : func.arg_types) {
        args.push_back(*a);
    }
    if (func.return_type) {
        ret = *func.return_type;
    }
    return {args, ret};
}

vector<string> Type::split_union_components() const {
    vector<string> result;
    if (!is_union_type()) {
        result.push_back(pool);
        return result;
    }
    const auto& u = get<UnionType>(m_data);
    for (const auto& comp : u.components) {
        result.push_back(comp->pool);
    }
    return result;
}

vector<TypePtr> Type::get_union_components() const {
    if (is_union_type()) {
        return get<UnionType>(m_data).components;
    } else {
        return {make_type_ptr(*this)};
    }
}

#include <type_traits> // нужно добавить в начало файла

bool Type::is_sub_type_impl(const Type& other, const UnionType* other_union) const {
    // Если other - union, проверяем, является ли this подтипом любого компонента
    if (other_union) {
        for (const auto& comp : other_union->components) {
            if (is_sub_type_impl(*comp, nullptr)) return true;
        }
        return false;
    }
    
    // Если this - union
    if (is_union_type()) {
        const auto& this_union = get<UnionType>(m_data);
        for (const auto& comp : this_union.components) {
            if (!comp->is_sub_type_impl(other, nullptr)) return false;
        }
        return true;
    }
    
    // Оба не union
    if (m_data.index() != other.m_data.index()) {
        // Проверка на auto
        if (holds_alternative<AutoType>(other.m_data)) return true;
        if (holds_alternative<PointerAutoType>(other.m_data) && is_pointer()) return true;
        return false;
    }
    
    bool result = false;
    std::visit([&](const auto& lhs, const auto& rhs) {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;
        
        if constexpr (!std::is_same_v<L, R>) {
            result = false;
        } else {
            if constexpr (std::is_same_v<L, PrimitiveType>) {
                result = (lhs.name == rhs.name);
            }
            else if constexpr (std::is_same_v<L, PointerType>) {
                result = lhs.pointee->is_sub_type(*rhs.pointee);
            }
            else if constexpr (std::is_same_v<L, ArrayType>) {
                result = lhs.element_type->is_sub_type(*rhs.element_type);
            }
            else if constexpr (std::is_same_v<L, FunctionType>) {
                if (lhs.arg_types.size() != rhs.arg_types.size()) {
                    result = false;
                    return;
                }
                for (size_t i = 0; i < lhs.arg_types.size(); ++i) {
                    // Контравариантность: rhs.arg_types[i] должен быть подтипом lhs.arg_types[i]
                    if (!rhs.arg_types[i]->is_sub_type(*lhs.arg_types[i])) {
                        result = false;
                        return;
                    }
                }
                // Обработка возвращаемого типа (может быть nullptr)
                if (lhs.return_type && rhs.return_type) {
                    result = lhs.return_type->is_sub_type(*rhs.return_type);
                } else {
                    result = (!lhs.return_type && !rhs.return_type);
                }
            }
            else {
                result = false; // Не должно произойти
            }
        }
    }, m_data, other.m_data);
    return result;
}

bool Type::is_sub_type(const Type& other) const {
    // Обработка other как union
    if (other.is_union_type()) {
        return is_sub_type_impl(other, &get<UnionType>(other.m_data));
    }
    return is_sub_type_impl(other, nullptr);
}

Type Type::operator|(const Type& other) const {
    // Получаем компоненты обоих типов
    auto comps1 = get_union_components();
    auto comps2 = other.get_union_components();
    
    // Объединяем, убирая дубликаты
    vector<TypePtr> result;
    for (const auto& c : comps1) {
        bool found = false;
        for (const auto& r : result) {
            if (compare(*c, *r)) { found = true; break; }
        }
        if (!found) result.push_back(c);
    }
    for (const auto& c : comps2) {
        bool found = false;
        for (const auto& r : result) {
            if (compare(*c, *r)) { found = true; break; }
        }
        if (!found) result.push_back(c);
    }
    
    if (result.size() == 1) {
        return *result[0];
    } else {
        return Type(UnionType{result});
    }
}


static string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}


static vector<string> split_top_level(const string& s, char delim) {
    vector<string> result;
    int paren = 0;
    int square = 0;
    size_t start = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '(') paren++;
        else if (s[i] == ')') paren--;
        else if (s[i] == '[') square++;
        else if (s[i] == ']') square--;
        else if (s[i] == delim && paren == 0 && square == 0) {
            result.push_back(trim(s.substr(start, i - start)));
            start = i + 1;
        }
    }
    result.push_back(trim(s.substr(start)));
    return result;
}

void Type::parse_from_string(const string& str) {
    string s = trim(str);
    if (s.empty()) {
        m_data = monostate{};
        update_pool();
        return;
    }

    if (s == "auto") {
        m_data = AutoType{};
        update_pool();
        return;
    }
    if (s == "*auto") {
        m_data = PointerAutoType{};
        update_pool();
        return;
    }

    // Проверка на union (содержит '|' на верхнем уровне)
    {
        int paren = 0, square = 0;
        bool is_union = false;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '(') paren++;
            else if (s[i] == ')') paren--;
            else if (s[i] == '[') square++;
            else if (s[i] == ']') square--;
            else if (s[i] == '|' && paren == 0 && square == 0) {
                is_union = true;
                break;
            }
        }
        if (is_union) {
            vector<string> comp_strs = split_top_level(s, '|');
            vector<TypePtr> comps;
            for (const string& comp_str : comp_strs) {
                comps.push_back(make_type_ptr(Type(comp_str)));
            }
            m_data = UnionType{comps};
            update_pool();
            return;
        }
    }

    // Проверка на указатель
    if (s[0] == '*') {
        string rest = s.substr(1);
        Type pointee(rest);
        m_data = PointerType{make_type_ptr(pointee)};
        update_pool();
        return;
    }

    // Проверка на массив
    if (s[0] == '[') {
        
        size_t close = s.find(']');
        if (close != string::npos) {
            string inner = s.substr(1, close - 1);
            size_t comma = inner.find(',');
            if (comma != string::npos) {
                string elem_str = trim(inner.substr(0, comma));
                string size_str = trim(inner.substr(comma + 1));
                Type elem_type(elem_str);
                optional<size_t> sz;
                if (size_str != "~") {
                    sz = stoull(size_str);
                }
                m_data = ArrayType{make_type_ptr(elem_type), sz};
                update_pool();
                return;
            }
        }
    }

    // Проверка на функцию
    if (s.find("Func") != string::npos) {
        
        size_t lparen = s.find('(');
        if (lparen != string::npos) {
            size_t rparen = s.find(')', lparen);
            if (rparen != string::npos) {
                string args_str = s.substr(lparen + 1, rparen - lparen - 1);
                vector<string> arg_strs = split_top_level(args_str, ',');
                vector<TypePtr> args;
                for (const string& a : arg_strs) {
                    if (!a.empty()) args.push_back(make_type_ptr(Type(a)));
                }
                // Проверяем наличие возвращаемого типа
                size_t arrow = s.find("->", rparen);
                TypePtr ret = nullptr;
                if (arrow != string::npos) {
                    string ret_str = s.substr(arrow + 2);
                    ret_str = trim(ret_str);
                    
                    if (!ret_str.empty() && ret_str.front() == '(' && ret_str.back() == ')') {
                        ret_str = ret_str.substr(1, ret_str.size() - 2);
                    }
                    ret = make_type_ptr(Type(ret_str));
                }
                m_data = FunctionType{args, ret};
                update_pool();
                return;
            }
        }
    }

    // Иначе считаем примитивным типом
    m_data = PrimitiveType{s};
    update_pool();
}


Type create_pointer_type(const Type& type) {
    if (type.is_union_type()) {
        auto comps = type.get_union_components();
        vector<TypePtr> ptr_comps;
        for (const auto& comp : comps) {
            ptr_comps.push_back(make_type_ptr(Type(PointerType{comp})));
        }
        return Type(UnionType{ptr_comps});
    } else {
        return Type(PointerType{make_type_ptr(type)});
    }
}

Type create_function_type(Type return_type, vector<Type> argument_types) {
    vector<TypePtr> args;
    for (auto& a : argument_types) {
        args.push_back(make_type_ptr(a));
    }
    return Type(FunctionType{args, make_type_ptr(return_type)});
}

Type create_function_type(vector<Type> argument_types) {
    vector<TypePtr> args;
    for (auto& a : argument_types) {
        args.push_back(make_type_ptr(a));
    }
    
    return Type(FunctionType{args, nullptr});
}

// ---------------------------------------------------------------------
// Стандартные типы
// ---------------------------------------------------------------------

namespace STANDART_TYPE {
    const Type INT = Type("Int");
    const Type BOOL = Type("Bool");
    const Type STRING = Type("String");
    const Type CHAR = Type("Char");
    const Type DOUBLE = Type("Double");
    const Type TYPE = Type("Type");
    const Type NAMESPACE = Type("Namespace");
    const Type NULL_T = Type("Null");
    const Type LAMBDA = Type("Lambda");
    const Type AUTO = Type("auto");
    const Type METHOD = Type("Method");

    const Type UNTYPED = INT | BOOL | STRING | CHAR | DOUBLE | NAMESPACE | NULL_T | LAMBDA;
}

bool IsTypeCompatible(const Type& target_type, const Type& source_type) {
    return source_type.is_sub_type(target_type);
}

struct Value {
    Type type;
    std::any data;

    Value(const Type type, std::any data) 
        : type(std::move(type)), data(std::move(data)) {}
    
    Value(const Value& other) 
        : type(other.type), 
          data(other.data) {}
  
    Value(Value&& other) = default;

    Value& operator=(const Value& other) {
        if (this != &other) {
            type = other.type;
            data = other.data;
        }
        return *this;
    }
    
    Value& operator=(Value&& other) = default;

    Value copy() const {
        return Value(*this);
    }

    friend ostream& operator<<(ostream& os, const Value& value) {
        if (value.type == STANDART_TYPE::STRING) {
            os << any_cast<string>(value.data);
        } else if (value.type == STANDART_TYPE::NAMESPACE) {
            os << "NAMESPACE";
        } else {
            os << "Value(" << value.type.pool << ")";
        }
        return os;
    }
};

struct Null {
    Null() = default;
    template<typename T> bool operator==(T) const { return false; }
    template<typename T> bool operator!=(T) const { return true; }
};

Value NewInt(int64_t value) { return Value(STANDART_TYPE::INT, value); }
Value NewDouble(NUMBER_ACCURACY value) { return Value(STANDART_TYPE::DOUBLE, value); }
Value NewBool(bool value) { return Value(STANDART_TYPE::BOOL, value); }
Value NewType(const string& name) { return Value(STANDART_TYPE::TYPE, Type(name)); }
Value NewType(const Type& type) { return Value(STANDART_TYPE::TYPE, type); }
Value NewNull() { return Value(STANDART_TYPE::NULL_T, Null()); }
Value NewString(const string& value) { return Value(STANDART_TYPE::STRING, value); }
Value NewChar(char value) { return Value(STANDART_TYPE::CHAR, value); }

Value NewPointer(int value, const Type& pointer_type, bool create_pointer = true) {
    if (create_pointer)
        return Value(create_pointer_type(pointer_type), value);
    else
        return Value(pointer_type, value);
}

Type MakePointerType(const Type& pointer_type) {
    return create_pointer_type(pointer_type);
}

Value NewPointerValue(int address, const Type& pointee_type) {
    return NewPointer(address, pointee_type);
}