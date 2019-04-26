project(postoken)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( postoken postoken postoken.cpp )
target_include_directories( postoken PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( postoken ${CMAKE_SOURCE_DIR}/../ricardian )