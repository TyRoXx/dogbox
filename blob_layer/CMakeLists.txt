file(GLOB sources *.h *.cpp)
add_library(blob_layer ${sources})
target_link_libraries(blob_layer SQLite::SQLite3 OpenSSL::Crypto dl)
