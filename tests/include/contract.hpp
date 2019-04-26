#pragma once

#include <eosio_testing.hpp>

namespace eosio_testing {

class contract {
public:
   typedef std::function<std::vector<uint8_t>()> read_wasm_f;
   typedef std::function<fc::ecc::range_proof_type()> read_abi_f;

   contract(tester& tester, read_wasm_f read_wasm, read_abi_f read_abi, 
            account_name contract_name) 
      : ctester(tester), _read_wasm(read_wasm), _read_abi(read_abi), 
        _contract_name(contract_name) {}

   virtual ~contract() {}

   virtual void init();

   void init_serializer();

   virtual void create_contract();

   virtual action_result push_action(const account_name& signer, const action_name &name, 
                                     const variant_object &data );
   virtual action_result push_action(const vector<account_name> signers, const action_name& name,
                                     const variant_object& data);

   fc::variant get_entry(account_name table_name, const string& row_type_name, uint64_t id) {
      vector<char> data = ctester.get_row_by_account(_contract_name, _contract_name, table_name, id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant(row_type_name, data, 
                                                                        ctester.abi_serializer_max_time );
   }

   fc::variant get_entry(uint64_t scope, account_name table_name, const string& row_type_name, uint64_t id) {
      vector<char> data = ctester.get_row_by_account(_contract_name, scope, table_name, id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant(row_type_name, data, 
                                                                        ctester.abi_serializer_max_time );
   }

   size_t get_entry_count(account_name table_name) {
      const table_id_object* table = ctester.find_table(_contract_name, _contract_name, table_name);
      return (table == nullptr) ? 0 : table->count;
   }

   size_t get_entry_count(uint64_t scope, account_name table_name) {
      const table_id_object* table = ctester.find_table(_contract_name, scope, table_name);
      return (table == nullptr) ? 0 : table->count;
   }

   account_name get_contract_name() {
      return _contract_name;
   }

   tester& ctester;
   abi_serializer abi_ser;

protected:
   read_wasm_f _read_wasm;
   read_abi_f _read_abi;
   account_name _contract_name;
};

}
