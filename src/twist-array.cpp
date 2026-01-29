#include "twist-values.cpp"

// TODO
struct Array {
    vector<Value> values;
    Array(vector<Value> values) : values(values) {}
    inline int get_size() const { return values.size(); }
};