
#include <stdlib.h>

#include <map>
#include <list>
#include <string>
#include <stdexcept>
#include <iostream>
#include <functional>
#include <algorithm>



//внутренние потроха для пущего полиморфизму
namespace detail {
    struct gc_ref_base_t;
    
    struct gc_manged_t {
        typedef std::list<gc_ref_base_t*> references_t;
        
        virtual ~gc_manged_t() {}
        virtual references_t refs() const = 0; 
    };
    
    struct gc_ref_base_t {
        virtual ~gc_ref_base_t() {}
        virtual gc_manged_t* managed() = 0;
    };
}

template <typename T> class gc_ref_t;

//Базовый класс для объектов подлежащих сбору(если мы хотим конечно избавиться от circular dependancy)
template <typename Derived>
struct gc_managed_provider_t : detail::gc_manged_t {
    
    virtual ~gc_managed_provider_t() {}

    //Тут нужны variadic templates но пока без них
    template <typename M1 = void, typename M2 = void>
    gc_managed_provider_t(gc_ref_t<M1> Derived::* m1 = 0, gc_ref_t<M2> Derived::* m2 = 0) {
        if (m1) {
            lazy_.push_back([this, m1] () mutable {
                return &(static_cast<Derived*>(this)->*m1);
            });
        }
        
        if (m2) {
            lazy_.push_back([this, m2] () mutable {
                return &(static_cast<Derived*>(this)->*m2);
            });
        }
    }
    
    virtual detail::gc_manged_t::references_t refs() const {
        detail::gc_manged_t::references_t ret;
        
        for(auto it = lazy_.begin(); it != lazy_.end(); ++it) {
            ret.push_back((*it)());
        }
        
        return ret;
    }
    
    
private:
    std::list<std::function<detail::gc_ref_base_t*()>> lazy_; 
};



//Собственно коллектор
class gc_t {
    typedef std::pair<int, std::string> meminfo_t;
    typedef std::map<detail::gc_manged_t*, meminfo_t> memory_t;
    
public:
    
    //примитивный синглтон
    static gc_t* instance() {
        static gc_t* p = new gc_t();
        return p;
    }
    
    template<typename T>
    T* crete(const std::string& info = "unknown") {
        T* ptr = reinterpret_cast<T*>(new T);
        
        if (!memory_.insert(std::make_pair(ptr, meminfo_t(1, info))).second) {
            throw std::runtime_error("GC corrupted: such address in use");
        }
        
        return ptr;
    }
    
    template<typename T>
    T* acquire(T* ptr) {
        if (memory_.find(ptr) == memory_.end()) {
            throw std::runtime_error("GC corrupted: no such address in pool");
        }
        memory_[ptr].first++;
        return ptr;
    }
    
    void release(detail::gc_manged_t* ptr) {
        if (memory_.find(ptr) == memory_.end()) {
            throw std::runtime_error("GC corrupted: no such address in pool");
        }
        memory_[ptr].first--;
    }
    
    void collect() {
        std::cerr << "GC[before]: " << memory_.size() << " elements in pool" << std::endl;
        
        std::list<memory_t::iterator> to_remove;
        
        for(auto it = memory_.begin(); it != memory_.end(); ++it) {
            std::cerr << "memblock [" << it->second.second << "] has " << it->second.first << " references" << std::endl;
            if (it->second.first == 0) {
                to_remove.push_back(it);
                std::cerr << "    ** removed **" << std::endl;
            }
            else if (it->second.first == 1) {
                
                //тут конечно надо запускать алгоритм обхода - но это следующий уровень
                auto refs = it->first->refs();
                for(auto refit = refs.begin(); refit != refs.end(); ++refit) {
                    
                    //пока все делаем в лоб
                    if (auto refptr = (*refit)->managed()) {
                        auto backref = refptr->refs();
                        for(auto backrefit = backref.begin(); backrefit != backref.end(); ++backrefit) {
                            if ((*backrefit)->managed() == it->first) {
                                to_remove.push_back(it);
                                std::cerr << "    ** CIRCULAR removed **" << std::endl;                                
                            }
                        }
                    }
                }
                
            }
        }
        
        for(auto rmit = to_remove.begin(); rmit != to_remove.end(); ++rmit) {
            free((*rmit)->first);
            memory_.erase(*rmit);
        }
        
        std::cerr << "GC[after]: " << memory_.size() << " elements in pool" << std::endl;
    }
    
    
private:
    gc_t(){};
    
    memory_t memory_;
};



//нечто вроде умного указателя, еще можно расширять
template <typename T>
class gc_ref_t : public detail::gc_ref_base_t {
    
public:
    
    gc_ref_t() : ptr_(0) {}
    
    gc_ref_t(const std::string& info)
        : ptr_(gc_t::instance()->crete<T>(info)) {
    } 
    
    gc_ref_t(const gc_ref_t<T>& other)
        : ptr_(0) {
        (*this) = other;
    }

    
    ~gc_ref_t() {
        if (ptr_) {
            gc_t::instance()->release(ptr_);
        }
    }
    
    T*operator->() {
        return ptr_;
    }
    
    gc_ref_t<T>& operator=(const gc_ref_t<T>& other)
    {
        if (ptr_) {
            gc_t::instance()->release(ptr_);
        }
        ptr_ = gc_t::instance()->acquire(other.ptr_);
        return *this;
    }
    
    detail::gc_manged_t* managed() {return ptr_;}
    
private:
    T* ptr_;
};




