#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

class Configuration {
public:
  bool read_file(const char *filename);

protected:
  virtual void configure(class JsonDocument &doc) = 0;
};

#endif
