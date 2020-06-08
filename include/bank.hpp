#ifndef BANK_H
#define BANK_H

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/multi_index.hpp>

#include "eosiotoken.hpp"
#include "common.hpp"

using namespace eosio;
using std::map;
using std::string;

class Bank
{

public:
    // struct [[eosio::table, eosio::contract("dao")]] Config
    // {
    //     // required configurations:
    //     // names : telos_decide_contract, reward_token_contract, last_ballot_id
    //     // ints  : voting_duration_sec, quorum_perc_x100, approve_threshold_perc_x100, voting_duration_sec
    //     map<string, name> names;
    //     map<string, string> strings;
    //     map<string, asset> assets;
    //     map<string, time_point> time_points;
    //     map<string, uint64_t> ints;
    //     map<string, transaction> trxs;
    //     map<string, float> floats;
    // };

    // typedef singleton<"config"_n, Config> config_table;

    struct [[eosio::table, eosio::contract("dao")]] Payment
    {
        uint64_t payment_id;
        time_point payment_date = current_time_point();
        name recipient;
        asset amount;
        string memo;

        uint64_t primary_key() const { return payment_id; }
        uint64_t by_recipient() const { return recipient.value; }
        uint64_t by_assignment() const { return assignment_id; }
    };

    typedef multi_index<"payments"_n, Payment,
                        indexed_by<"byrecipient"_n, const_mem_fun<Payment, uint64_t, &Payment::by_recipient>>,
                        indexed_by<"byassignment"_n, const_mem_fun<Payment, uint64_t, &Payment::by_assignment>>>
        payment_table;

    struct [[eosio::table, eosio::contract("dao")]] Debug
    {
        uint64_t debug_id;
        string notes;
        time_point created_date = current_time_point();
        uint64_t primary_key() const { return debug_id; }
    };

    typedef multi_index<"debugs"_n, Debug> debug_table;

    void debug(const string &notes)
    {
        debug_table d_t(contract, contract.value);
        d_t.emplace(contract, [&](auto &d) {
            d.debug_id = d_t.available_primary_key();
            d.notes = notes;
        });
    }

    name contract;
    payment_table payment_t;
    config_table config_s;

    Bank(const name &contract) : contract(contract),
                                 payment_t(contract, contract.value),
                                 config_s(contract, contract.value) {}

    void makepayment(const name &recipient,
                     const asset &quantity,
                     const string &memo)
    {

        if (quantity.amount == 0)
        {
            return;
        }

        debug("Making payment to recipient: " + recipient.to_string() + ", quantity: " + quantity.to_string());

        config_table config_s(contract, contract.value);
        Config c = config_s.get_or_create(contract, Config());

        if (quantity.symbol == common::S_VOTE)
        {
            action(
                permission_level{contract, "active"_n},
                c.names.at("telos_decide_contract"), "mint"_n,
                std::make_tuple(recipient, quantity, memo))
                .send();
        }
        else
        {
            issuetoken(c.names.at("reward_token_contract"), recipient, quantity, memo);
        }

        payment_t.emplace(contract, [&](auto &p) {
            p.payment_id = payment_t.available_primary_key();
            p.payment_date = current_block_time().to_time_point();
            p.recipient = recipient;
            p.amount = quantity;
            p.memo = memo;
        });
    }

    void issuetoken(const name &token_contract,
                    const name &to,
                    const asset &token_amount,
                    const string &memo)
    {
        string debug_str = "";
        debug_str = debug_str + "Issue Token Event; ";
        debug_str = debug_str + "    Token Contract  : " + token_contract.to_string() + "; ";
        debug_str = debug_str + "    Issue To        : " + to.to_string() + "; ";
        debug_str = debug_str + "    Issue Amount    : " + token_amount.to_string() + ";";
        debug_str = debug_str + "    Memo            : " + memo + ".";

        action(
            permission_level{contract, "active"_n},
            token_contract, "issue"_n,
            std::make_tuple(contract, token_amount, memo))
            .send();

        action(
            permission_level{contract, "active"_n},
            token_contract, "transfer"_n,
            std::make_tuple(contract, to, token_amount, memo))
            .send();

        debug(debug_str);
    }

    bool holds_reward_token(const name &account)
    {
        config_table config_s(contract, contract.value);
        Config c = config_s.get_or_create(contract, Config());

        eosiotoken::accounts a_t(c.names.at("reward_token_contract"), account.value);
        auto a_itr = a_t.find(common::S_REWARD.code().raw());
        if (a_itr == a_t.end())
        {
            return false;
        }
        else if (a_itr->balance.amount > 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
};

#endif