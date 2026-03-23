#include "iostream"
#include "string"
#include "locale"
using namespace std;






int main() {
    
    string s = "привет";
    std::locale::global(std::locale("ru_RU.UTF-8"));
    

    for (int i = 0; i < s.size(); i++) {
        cout << s[i] << endl;
    }

}