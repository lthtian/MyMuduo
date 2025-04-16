#pragma once

class UnCopyable
{
private:
    UnCopyable(const UnCopyable &) = delete;
    UnCopyable &operator=(const UnCopyable &) = delete;

public:
    UnCopyable() = default;
    ~UnCopyable() = default;
};