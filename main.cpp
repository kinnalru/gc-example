
#include <iostream>
#include <string>
#include <stdexcept>

#include "gc.h"


//Так мы определяем класс подлежащий собиранию
struct A : gc_managed_provider_t<A> {
    
    //а так мы дополняем его метаинформацией о хранимых ссылках
    A() : gc_managed_provider_t<A>(&A::a) {}
    
    //Собственно хранимая ссылка
    gc_ref_t<A> a;
};

int main(int argc, char *argv[])
{
    
    {
        //Строки в конструкторе - только для детального отображения
        gc_ref_t<A> a1("a1");
        gc_ref_t<A> a2("a2");
    
        std::cerr << "\n =simple=\n" << std::endl;
        gc_t::instance()->collect();
        
        //Создаем кроссылочность
        a1->a = a2;
        a2->a = a1;
        
        std::cerr << "\n =cross-reference=\n" << std::endl;
        gc_t::instance()->collect();
        
    }
    
    std::cerr << "\n =after scope=\n" << std::endl;
    gc_t::instance()->collect();

    
    std::cerr <<"finish" << std::endl;
    return 0;
} 

