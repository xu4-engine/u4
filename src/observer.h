/*
 * $Id$
 */

#ifndef OBSERVER_H
#define OBSERVER_H

template<class T>
class Observable;

/**
 * This is the interface a class must implement to watch an
 * Observable.
 */
template<class T>
class Observer {
public:
    virtual void update(Observable<T> *o, T arg) = 0;
    virtual ~Observer() {}
};

#endif /* OBSERVER_H */
