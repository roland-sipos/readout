cmake_minimum_required(VERSION 3.12)
project(readout VERSION 1.0.1)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(dataformats REQUIRED)
find_package(dfmessages REQUIRED)
#find_package(udaq_readout_deps REQUIRED)
find_package(folly REQUIRED)
find_package(TBB REQUIRED)

##############################################################################
# Dependency sets
#set(FLXCARD_DEPENDENCIES udaq_readout_deps::FlxCard udaq_readout_deps::cmem_rcc udaq_readout::deps::packetformat)
set(FLXUTIL_DEPENDENCIES 
  dataformats::dataformats
  dfmessages::dfmessages
  #udaq_readout_deps::packetformat 
  Folly::folly ${TBB} ${TBB_IMPORTED_TARGETS}
)

##############################################################################
# Main library
daq_add_library(#DefaultParserImpl.cpp          
  LINK_LIBRARIES appfwk::appfwk ${FLXUTIL_DEPENDENCIES}
)

##############################################################################
# Plugins
#daq_add_plugin(FelixCardReader duneDAQModule SCHEMA LINK_LIBRARIES appfwk::appfwk readout ${FLXCARD_DEPENDENCIES})
daq_add_plugin(FakeCardReader duneDAQModule SCHEMA LINK_LIBRARIES appfwk::appfwk readout)
daq_add_plugin(DataLinkHandler duneDAQModule SCHEMA LINK_LIBRARIES appfwk::appfwk readout)

##############################################################################
# Integration tests
daq_add_application(readout_test_ratelimiter test_ratelimiter_app.cxx TEST LINK_LIBRARIES readout)

##############################################################################
# Test configurations
file(COPY test/fakereadout-commands.json DESTINATION test)

##############################################################################
# Installation
daq_install()
