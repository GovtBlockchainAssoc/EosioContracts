#include <dao.hpp>

using namespace daospace;

asset dao::adjust_asset(const asset &original_asset, const float &adjustment)
{
    return asset{static_cast<int64_t>(original_asset.amount * adjustment), original_asset.symbol};
}

string dao::get_string(const std::map<string, string> strings, string key)
{
    if (strings.find(key) != strings.end())
    {
        return strings.at(key);
    }
    else
    {
        return string{""};
    }
}

void dao::checkx(const bool &condition, const string &message)
{
    if (condition)
    {
        return;
    }

    transaction trx(time_point_sec(current_time_point()) + (60));
    trx.actions.emplace_back(
        permission_level{get_self(), "active"_n},
        get_self(), "debugmsg"_n,
        std::make_tuple(message));
    trx.delay_sec = 0;
    trx.send(get_next_sender_id(), get_self());

    check(false, message);
}

void dao::change_scope(const name &current_scope, const uint64_t &id, const name &new_scope, const bool &remove_old)
{

    object_table o_t_current(get_self(), current_scope.value);
    auto o_itr_current = o_t_current.find(id);
    check(o_itr_current != o_t_current.end(), "Scope: " + current_scope.to_string() + "; Object ID: " + std::to_string(id) + " does not exist.");

    object_table o_t_new(get_self(), new_scope.value);
    o_t_new.emplace(get_self(), [&](auto &o) {
        o.id = o_t_new.available_primary_key();
        o.names = o_itr_current->names;
        o.names["prior_scope"] = current_scope;
        o.assets = o_itr_current->assets;
        o.strings = o_itr_current->strings;
        o.floats = o_itr_current->floats;
        o.time_points = o_itr_current->time_points;
        o.ints = o_itr_current->ints;
        o.ints["prior_id"] = o_itr_current->id;
        o.trxs = o_itr_current->trxs;
    });

    if (remove_old)
    {
        debug("Erasing object from : " + current_scope.to_string() + "; copying to : " + new_scope.to_string());
        o_t_current.erase(o_itr_current);
    }
}

void dao::debug(const string &notes)
{
    debug_table d_t(get_self(), get_self().value);
    d_t.emplace(get_self(), [&](auto &d) {
        d.debug_id = d_t.available_primary_key();
        d.notes = notes;
    });
}

void dao::debugmsg (const string& message) {
	require_auth (get_self());
	debug (message);
}

void dao::clrdebugs (const uint64_t& starting_id, const uint64_t& batch_size) {
	check (has_auth ("gba"_n) || has_auth(get_self()), "Requires higher permission.");
	debug_table d_t (get_self(), get_self().value);
	auto d_itr = d_t.find (starting_id);

	while (d_itr->debug_id <= starting_id + batch_size) {
		d_itr = d_t.erase (d_itr);
	}

	eosio::transaction out{};
	out.actions.emplace_back(permission_level{get_self(), "active"_n}, 
							get_self(), "clrdebugs"_n, 
							std::make_tuple(d_itr->debug_id, batch_size));
	out.delay_sec = 1;
	out.send(get_next_sender_id(), get_self());    
}