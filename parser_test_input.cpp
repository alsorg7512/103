class Animal {
public:
    int age;
    virtual void speak() {}
    virtual void move() {}
};

class Dog : public Animal {
public:
    int weight;
    void speak() {}
};

class Husky : public Dog {
public:
    int coldLevel;
    void move() {}
};