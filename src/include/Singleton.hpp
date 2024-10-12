#pragma once
#ifndef SINGLTON_H
#define SINGLTON_H

#include <iostream>

template <typename T>
class Singleton {
public:
    static T& instance() noexcept(std::is_nothrow_constructible<T>::value) {
        static T instance;
        return instance;
    }
    virtual ~Singleton() noexcept {
        //std::cout << "destructor called!" << std::endl;
    }
    Singleton(const Singleton&) = delete;
    Singleton& operator =(const Singleton&) = delete;
protected:
    Singleton() {
        //std::cout << "constructor called!" << std::endl;
    }
};

#endif