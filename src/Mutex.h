//-----------------------------------------------------------------------------
// Mutex.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef MUTEX_H
#define MUTEX_H

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

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

#ifdef WIN32
  HANDLE mutex;
#else
  pthread_mutex_t mutex;
#endif
};

#endif // MUTEX_H
