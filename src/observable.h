/*
 * $Id$
 */

#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include <algorithm>
#include <vector>

#include "observer.h"

/**
 * Classes can report updates to a list of decoupled Observers by
 * extending this class.
 */
template <class T>
class Observable {
public:
    Observable() : changed(false) {}

    void addObserver(Observer<T> *o) {
        typename std::vector< Observer<T> *>::iterator i;
        i = std::find(observers.begin(), observers.end(), o);
        if (i == observers.end())
            observers.push_back(o);
    }

    int countObservers() const { 
        return observers.size();
    }

    void deleteObserver(Observer<T> *o) {
        typename std::vector< Observer<T> *>::iterator i;
        i = std::find(observers.begin(), observers.end(), o);
        if (i != observers.end())
            observers.erase(i);
    }

    void deleteObservers() { 
        observers.clear(); 
    }

    bool hasChanged() const { 
        return changed; 
    }

    void notifyObservers(T arg) {
        if (!changed)
            return;

        // vector iterators are invalidated if erase is called, so a copy
        // is used to prevent problems if the observer removes itself (or
        // otherwise changes the observer list)
        typename std::vector< Observer<T> *> tmp = observers;
        typename std::vector< Observer<T> *>::iterator i;

        clearChanged();

        for (i = tmp.begin(); i != tmp.end(); i++) {
            Observer<T> *observer = *i;
            observer->update(this, arg);
        }
    }

protected:
    void clearChanged() { changed = false; }
    void setChanged() { changed = true; }

private:
    bool changed;
    std::vector< Observer<T> *> observers;
};

#endif /* OBSERVABLE_H */
