#include "twist-values.cpp"

// TODO
struct Array {
    vector<Value> values;
    Type type;
    Array(Type type, vector<Value> values) : type(type), values(values) {
        
    }
    int get_size() { return values.size(); }
};

template<typename T>
vector<T> concatenate(const vector<T>& v1, const vector<T>& v2) {
    vector<T> result;
    result.reserve(v1.size() + v2.size());  // ИСПРАВЛЕНО: удалена ошибка с * 10
    result.insert(result.end(), v1.begin(), v1.end());
    result.insert(result.end(), v2.begin(), v2.end());
    return result;
}