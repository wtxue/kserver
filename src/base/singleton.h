#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include <assert.h>
#include <pthread.h>

template<typename T>
class singleton
{
public:
    static void init()
    {
        _ins = new T();
        assert(_ins);
    }

    static T* ins()
    {
        pthread_once(&_once, init);
        return _ins;
    }

private:
    singleton(const singleton&);//necessary
    const singleton& operator=(const singleton&);//necessary

    static T* _ins;
    static pthread_once_t _once;
};

template<typename T>
T* singleton<T>::_ins = NULL;

template<typename T>
pthread_once_t singleton<T>::_once = PTHREAD_ONCE_INIT;

#endif
