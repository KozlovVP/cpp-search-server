// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь: 271

// Закомитьте изменения и отправьте их в свой репозиторий.
//КОД:

/*#include <iostream>
#include <algorithm>

using namespace std;

int main()
{
    int c = 0;
    for (int i = 1; i <= 1000; i++) {
        string s = to_string(i);
        if (count(s.begin(), s.end(), '3') > 0) {c+=1;}
    }
    cout << c;
}*/
