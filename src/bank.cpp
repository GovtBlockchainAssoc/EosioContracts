#include <dao.hpp>

using namespace daospace;

void dao::makepayment(const name &recipient,
                      const asset &quantity,
                      const string &memo)
{

    if (quantity.amount == 0)
    {
        return;
    }

    debug("Making payment to recipient: " + recipient.to_string() + ", quantity: " + quantity.to_string());

    config_table config_s(get_self(), get_self().value);
    Config c = config_s.get_or_create(get_self(), Config());

    if (quantity.symbol == common::S_VOTE)
    {
        action(
            permission_level{get_self(), "active"_n},
            c.names.at("telos_decide_contract"), "mint"_n,
            std::make_tuple(recipient, quantity, memo))
            .send();
    }
    else
    {
        issuetoken(c.names.at("reward_token_contract"), recipient, quantity, memo);
    }

    payment_table payment_t (get_self(), get_self().value);
    payment_t.emplace(get_self(), [&](auto &p) {
        p.payment_id = payment_t.available_primary_key();
        p.payment_date = current_block_time().to_time_point();
        p.recipient = recipient;
        p.amount = quantity;
        p.memo = memo;
    });
}

void dao::issuetoken(const name &token_contract,
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
        permission_level{get_self(), "active"_n},
        token_contract, "issue"_n,
        std::make_tuple(get_self(), token_amount, memo))
        .send();

    action(
        permission_level{get_self(), "active"_n},
        token_contract, "transfer"_n,
        std::make_tuple(get_self(), to, token_amount, memo))
        .send();

    debug(debug_str);
}
