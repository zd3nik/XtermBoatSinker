//-----------------------------------------------------------------------------
// Mutex.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>

namespace xbs
{

//-----------------------------------------------------------------------------
class Mutex
{
public:
  Mutex();
  virtual ~Mutex();

  class Lock {
  public:
    Lock(Mutex& mutex) : mutex(&mutex) { mutex.lock(); }
    virtual ~Lock() { release(); }
    void release() {
      if (mutex) {
        mutex->unlock();
        mutex = NULL;
      }
    }
    bool isLocked() const {
      return (mutex != NULL);
    }
  private:
    Mutex* mutex;
  };

private:
  friend class Lock;

  void lock();
  void unlock();

  pthread_mutex_t mutex;
};

} // namespace xbs

#endif // MUTEX_H
