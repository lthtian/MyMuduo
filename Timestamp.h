#pragma once
#include <iostream>
class Timestamp
{
public:
    // 可以防止隐式转换
    explicit inline Timestamp(int64_t microSecondsSinceEpoch = 0) : _microSecondsSinceEpoch(microSecondsSinceEpoch) {}
    static Timestamp now();
    std::string toString() const;

private:
    int64_t _microSecondsSinceEpoch;
};