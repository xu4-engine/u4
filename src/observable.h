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
 * 
 * The O class parameter should be a pointer to the class of the
 * observable itself, so it can be passed in a typesafe manner to the
 * observers update method.
 *
 * The A class can be any additional information to pass to observers.
 * Observables that don't need to pass an argument when they update
 * observers should use the default "NoArg" class for the second
 * template parameter and pass NULL to notifyObservers.
 */
template <class O, class A = NoArg*>
class Observable {
public:
    Observable() : changed(false) {}

    void addObserver(Observer<O, A> *o) {
        typename std::vector< Observer<O, A> *>::iterator i;
        i = std::find(observers.begin(), observers.end(), o);
        if (i == observers.end())
            observers.push_back(o);
    }

    int countObservers() const { 
        return observers.size();
    }

    void deleteObserver(Observer<O, A> *o) {
        typename std::vector< Observer<O, A> *>::iterator i;
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

    void notifyObservers(A arg) {
        if (!changed)
            return;

        // vector iterators are invalidated if erase is called, so a copy
        // is used to prevent problems if the observer removes itself (or
        // otherwise changes the observer list)
        typename std::vector< Observer<O, A> *> tmp = observers;
        typename std::vector< Observer<O, A> *>::iterator i;

        clearChanged();

        for (i = tmp.begin(); i != tmp.end(); i++) {
            Observer<O, A> *observer = *i;
            observer->update(static_cast<O>(this), arg);
        }
    }

protected:
    void clearChanged() { changed = false; }
    void setChanged() { changed = true; }

private:
    bool changed;
    std::vector< Observer<O, A> *> observers;
};

#endif /* OBSERVABLE_H */
