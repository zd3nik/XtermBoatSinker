//-----------------------------------------------------------------------------
// Mutex.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#include "Mutex.h"
#include <iostream>

//-----------------------------------------------------------------------------
Mutex::Mutex()
{
#ifdef WIN32
  mutex = CreateMutex(NULL, FALSE, NULL);
  if (!mutex) {
    std::cerr << "Unable to create mutex: error code " << GetLastError()
              << std::endl;
  }
#else
  pthread_mutex_init(&mutex, NULL);
#endif
}

//-----------------------------------------------------------------------------
Mutex::~Mutex()
{
#ifdef WIN32
  if (mutex) {
    CloseHandle(mutex);
    mutex= NULL;
  }
#else
  pthread_mutex_destroy(&mutex);
#endif
}

//-----------------------------------------------------------------------------
void Mutex::lock()
{
#ifdef WIN32
  if (mutex) {
    WaitForSingleObject(mutex, INFINITE);
  }
#else
  pthread_mutex_lock(&mutex);
#endif
}

//-----------------------------------------------------------------------------
void Mutex::unlock()
{
#ifdef WIN32
  if (mutex) {
    ReleaseMutex(mutex);
  }
#else
  pthread_mutex_unlock(&mutex);
#endif
}
