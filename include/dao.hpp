#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/transaction.hpp>

#include <decide.hpp>

using namespace std;
using namespace eosio;

namespace daospace
{

   CONTRACT dao : public contract
   {
   public:

      dao(name self, name code, datastream<const char*> ds);
      ~dao();

      symbol         S_REWARD                        = symbol ("REWARD", 2);
      symbol         S_VOTE                          = symbol ("VOTEPOW", 2);
      asset          RAM_ALLOWANCE                   = asset {20000, symbol("TLOS", 4)};

      struct [[eosio::table, eosio::contract("dao")]] Config
      {
         // required configurations:
         // names : telos_decide_contract, reward_token_contract, last_ballot_id
         // ints  : voting_duration_sec, quorum_perc_x100, approve_threshold_perc_x100, voting_duration_sec
         map<string, name> names;
         map<string, string> strings;
         map<string, asset> assets;
         map<string, time_point> time_points;
         map<string, uint64_t> ints;
         map<string, transaction> trxs;
         map<string, float> floats;
         time_point updated_date = current_time_point();
      };

      typedef singleton<"config"_n, Config> config_table;
      typedef multi_index<"config"_n, Config> config_table_placeholder;

      struct [[eosio::table, eosio::contract("dao")]] Member
      {
         name member;
         vector<uint64_t> completed_challenges;
         uint64_t primary_key() const { return member.value; }
      };

      typedef multi_index<"members"_n, Member> member_table;

      struct [[eosio::table, eosio::contract("dao")]] Applicant
      {
         name applicant;
         string content;

         time_point created_date = current_time_point();
         time_point updated_date = current_time_point();

         uint64_t primary_key() const { return applicant.value; }
      };
      typedef multi_index<"applicants"_n, Applicant> applicant_table;

      // scope: proposal, member, applicant, challenge
      struct [[eosio::table, eosio::contract("dao")]] Object
      {
         uint64_t id;

         // core maps
         map<string, name> names;
         map<string, string> strings;
         map<string, asset> assets;
         map<string, time_point> time_points;
         map<string, uint64_t> ints;
         map<string, transaction> trxs;
         map<string, float> floats;
         uint64_t primary_key() const { return id; }

         // indexes
         uint64_t by_owner() const { return names.at("owner").value; }
         uint64_t by_type() const { return names.at("type").value; }
         uint64_t by_fk() const { return ints.at("fk"); }

         // timestamps
         time_point created_date = current_time_point();
         time_point updated_date = current_time_point();
         uint64_t by_created() const { return created_date.sec_since_epoch(); }
         uint64_t by_updated() const { return updated_date.sec_since_epoch(); }
      };

      typedef multi_index<"objects"_n, Object,
                          indexed_by<"bycreated"_n, const_mem_fun<Object, uint64_t, &Object::by_created>>, // index 2
                          indexed_by<"byupdated"_n, const_mem_fun<Object, uint64_t, &Object::by_updated>>, // 3
                          indexed_by<"byowner"_n, const_mem_fun<Object, uint64_t, &Object::by_owner>>,     // 4
                          indexed_by<"bytype"_n, const_mem_fun<Object, uint64_t, &Object::by_type>>,       // 5
                          indexed_by<"byfk"_n, const_mem_fun<Object, uint64_t, &Object::by_fk>>            // 6
                          >
          object_table;

      struct [[eosio::table, eosio::contract("dao")]] Payment
    {
        uint64_t payment_id;
        time_point payment_date = current_time_point();
        name recipient;
        asset amount;
        string memo;

        uint64_t primary_key() const { return payment_id; }
        uint64_t by_recipient() const { return recipient.value; }
        uint64_t by_amount() const { return amount.amount; }
    };

    typedef multi_index<"payments"_n, Payment,
                        indexed_by<"byrecipient"_n, const_mem_fun<Payment, uint64_t, &Payment::by_recipient>>,
                        indexed_by<"byamount"_n, const_mem_fun<Payment, uint64_t, &Payment::by_amount>>
      > payment_table;


      struct [[eosio::table, eosio::contract("dao")]] Debug
      {
         uint64_t debug_id;
         string notes;
         time_point created_date = current_time_point();
         uint64_t primary_key() const { return debug_id; }
      };

      typedef multi_index<"debugs"_n, Debug> debug_table;

      ACTION create(const name &scope,
                    const map<string, name> names,
                    const map<string, string> strings,
                    const map<string, asset> assets,
                    const map<string, time_point> time_points,
                    const map<string, uint64_t> ints,
                    const map<string, float> floats,
                    const map<string, transaction> trxs);

      ACTION apply(const name &applicant, const string &content);

      ACTION enroll(const name &enroller,
                    const name &applicant,
                    const string &content);

      // Admin
      ACTION togglepause();
      ACTION debugmsg(const string &message);
      ACTION updversion(const string &component, const string &version);

      ACTION setconfig(const map<string, name> names,
                       const map<string, string> strings,
                       const map<string, asset> assets,
                       const map<string, time_point> time_points,
                       const map<string, uint64_t> ints,
                       const map<string, float> floats,
                       const map<string, transaction> trxs);

      // convenience methods for only updating a single configuration item
      ACTION setconfigname(const string &key, const name &value);
      ACTION setconfigint(const string &key, const uint64_t &value);
      ACTION setlastballt(const name &last_ballot_id);
      ACTION clrdebugs(const uint64_t &starting_id, const uint64_t &batch_size);
      ACTION remapply(const name &applicant);

      // anyone can call closeprop, it executes the transaction if the voting passed
      ACTION closeprop(const uint64_t &proposal_id);

      // temporary hack (?) - keep a list of the members, although true membership is governed by token holdings
      ACTION removemember(const name &member_to_remove);
      ACTION addmember(const name &member);

      void validate_config_state ();
      void qualify_proposer(const name &proposer);
      name register_ballot(const name &proposer, const map<string, string> &strings);
      void debug(const string &notes);
      void checkx(const bool &condition, const string &message);
      void change_scope(const name &current_scope, const uint64_t &id, const name &new_scope, const bool &remove_old);
      string get_string(const std::map<string, string> strings, string key);
      asset adjust_asset(const asset &original_asset, const float &adjustment);
      uint64_t get_next_sender_id();
      void passprop (const uint64_t& proposal_id);

      void makepayment(const name &recipient, const asset &quantity,const string &memo);
      void issuetoken(const name &token_contract, const name &to, const asset &token_amount, const string &memo);
   };

} // namespace daospace
