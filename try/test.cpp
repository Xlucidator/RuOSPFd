#include<iostream>

using namespace std;

class A {
    int i1;
    int i2;
    int i3;

    A()=default;
    A(int i1);
    A(int i2, int* i3);
};

A::A(int i1):i1(i1) {
    i2 = 2;
}

A::A(int i2, int* i3):i2(i2) {
    this->i3 = *i3;
}