#include <dao.hpp>

using namespace daospace;
using std::map;
using std::string;

void dao::setconfig(const map<string, name> names,
                    const map<string, string> strings,
                    const map<string, asset> assets,
                    const map<string, time_point> time_points,
                    const map<string, uint64_t> ints,
                    const map<string, float> floats,
                    const map<string, transaction> trxs)
{
    require_auth(get_self());

    config_table config_s(get_self(), get_self().value);
    Config c = config_s.get_or_create(get_self(), Config());

    // retain last_ballot_id from the current configuration if it is not provided in the new one
    name last_ballot_id;
    if (names.find("last_ballot_id") != names.end())
    {
        last_ballot_id = names.at("last_ballot_id");
    }
    else if (c.names.find("last_ballot_id") != c.names.end())
    {
        last_ballot_id = c.names.at("last_ballot_id");
    }

    c.names = names;
    c.names["last_ballot_id"] = last_ballot_id;

    c.strings = strings;
    c.assets = assets;
    c.time_points = time_points;
    c.ints = ints;
    c.floats = floats;
    c.trxs = trxs;

    config_s.set(c, get_self());
}

uint64_t dao::get_next_sender_id()
{
    config_table config_s(get_self(), get_self().value);
    Config c = config_s.get_or_create(get_self(), Config());
    uint64_t return_senderid = c.ints.at("last_sender_id");
    return_senderid++;
    c.ints["last_sender_id"] = return_senderid;
    config_s.set(c, get_self());
    return return_senderid;
}

void dao::validate_config_state()
{
    config_table config_s(get_self(), get_self().value);
    Config c = config_s.get_or_create(get_self(), Config());
    checkx(c.ints.find("paused") != c.ints.end(), "Contract does not have a pause configuration. Assuming it is paused. Please contact community.");
    checkx(c.ints.at("paused") == 0, "Contract is paused for maintenance. Please try again later or contact the community.");

    // validate for required name configurations
    string required_names[]{"reward_token_contract", "telos_decide_contract", "last_ballot_id"};
    for (int i{0}; i < std::size(required_names); i++)
    {
        checkx(c.names.find(required_names[i]) != c.names.end(), "name configuration: " + required_names[i] + " is required but not provided.");
    }

    // validate for int configurations
    string required_ints[]{"quorum_perc_x100", "approve_threshold_perc_x100", "voting_duration_sec"};
    for (int i{0}; i < std::size(required_ints); i++)
    {
        checkx(c.ints.find(required_ints[i]) != c.ints.end(), "int configuration: " + required_ints[i] + " is required but not provided.");
    }
}