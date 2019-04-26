#include <contract.hpp>

namespace eosio_testing {

void contract::init() {
   create_contract();

   init_serializer();
}

void contract::init_serializer() {
   const auto& accnt = ctester.control->db().get<account_object,by_name>( _contract_name);
   abi_def abi;
   BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
   abi_ser.set_abi(abi, ctester.abi_serializer_max_time);
}

void contract::create_contract() {
   ctester.create_account(_contract_name);
   ctester.produce_blocks(2);

   ctester.set_code(_contract_name, _read_wasm());
   ctester.set_abi(_contract_name, _read_abi().data());
   ctester.produce_blocks();
}

action_result contract::push_action(const account_name& signer, const action_name& name, 
                                    const variant_object& data ) {
   string action_type_name = abi_ser.get_action_type(name);

   action act;
   act.account = _contract_name;
   act.name    = name;
   act.data    = abi_ser.variant_to_binary( action_type_name, data, ctester.abi_serializer_max_time );

   return ctester.push_action( std::move(act), uint64_t(signer));
}

action_result contract::push_action(const vector<account_name> signers, const action_name& name,
                                    const variant_object& data) {
   
   transaction_trace_ptr tx_trace;
   try {
      tx_trace = ctester.push_action(_contract_name, name, signers, data);
   } catch (const fc::exception& ex) {
      edump((ex.to_detail_string()));
      return tester::error(ex.top_message());
   }
   ctester.produce_block();
   BOOST_REQUIRE_EQUAL(true, ctester.chain_has_transaction(tx_trace->id));
   return tester::success();
}

}
