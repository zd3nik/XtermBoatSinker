//-----------------------------------------------------------------------------
// Input.h
// Copyright (c) 2016 Shawn Chidester, All rights reserved
//-----------------------------------------------------------------------------
#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>
#include <ctype.h>
#include <vector>

//-----------------------------------------------------------------------------
class Input
{
public:
  Input(const unsigned bufSize)
    : bufSize(bufSize),
      buffer(new char[bufSize])
  { }

  virtual ~Input() {
    delete[] buffer;
    buffer = NULL;
  }

  int readln(FILE* stream) {
    if (!stream) {
      return -1;
    }
    if (!fgets(buffer, sizeof(buffer), stream)) {
      if (ferror(stream)) {
        return -1;
      } else {
        return 0;
      }
    }
    fields.clear();
    char* begin = buffer;
    char* end = buffer;
    for (; (*end) && ((*end) != '\r') && ((*end) != '\n'); ++end) {
      if ((*end) == '|') {
        (*end) = 0;
        fields.push_back(begin);
        begin = (end + 1);
      }
    }
    if (end > begin) {
      (*end) = 0;
      fields.push_back(begin);
    }
    return fields.size();
  }

  unsigned getFieldCount() const {
    return fields.size();
  }

  const char* getString(const unsigned index, const char* def = NULL) const {
    return (index >= fields.size()) ? def : fields.at(index);
  }

  const int getInt(const unsigned index, const int def = -1) const {
    const char* value = getString(index, NULL);
    if (value) {
      if (isdigit(*value)) {
        return atoi(value);
      } else if (((*value) == '-') && isdigit(value[1])) {
        return atoi(value);
      } else if (((*value) == '+') && isdigit(value[1])) {
        return atoi(value + 1);
      }
    }
    return def;
  }

  const int getUnsigned(const unsigned index, const unsigned def = 0) const {
    const char* value = getString(index, NULL);
    if (value) {
      const char* p = ((*value) == '+') ? (value + 1) : value;
      if (isdigit(*p)) {
        unsigned i = 0;
        for (; isdigit(*p); ++p) {
          i = ((10 * i) + (i - '0'));
        }
        return i;
      }
    }
    return def;
  }

private:
  const unsigned bufSize;
  char* buffer;
  std::vector<const char*> fields;
};

#endif // INPUT_H
