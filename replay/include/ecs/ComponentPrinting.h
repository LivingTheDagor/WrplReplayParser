

#ifndef MYEXTENSION_COMPONENTPRINTING_H
#define MYEXTENSION_COMPONENTPRINTING_H
#include <iostream>
#include <type_traits>
#include <concepts>

// the declarations for this file are moved to after types such as ecs::string are declared
// AKA the bottom of ComponentTypeDefs
// types like ecs::string have not been 'declared' yet (ecs::ComponentTypeInfo<ecs::string>::type)


template<typename T>
std::string toStringImplECS(const void * p, int);



#endif // MYEXTENSION_COMPONENTPRINTING_H
