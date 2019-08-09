#include "configuration.hh"

Configuration *Configuration::_instance = nullptr;


Configuration::Configuration() :
  QSettings("https://github.com/takyonxxx?tab=repositories", "SdrRtlTuner")
{
  // pass...
}

Configuration::~Configuration() {

}

Configuration &
Configuration::get() {
  if (nullptr == _instance) { _instance = new Configuration(); }
  return *_instance;
}
