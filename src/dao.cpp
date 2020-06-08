#include <dao.hpp>

using namespace daospace;

dao::dao(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

dao::~dao() {}

void dao::addmember (const name& member) {
	require_auth (get_self());
	member_table m_t (get_self(), get_self().value);
	auto m_itr = m_t.find (member.value);
	check (m_itr == m_t.end(), "Account is already a member: " + member.to_string());
	m_t.emplace (get_self(), [&](auto &m) {
		m.member = member;
	});
}

void dao::removemember (const name& member) {
	require_auth (get_self());
	member_table m_t (get_self(), get_self().value);
	auto m_itr = m_t.find (member.value);
	check (m_itr != m_t.end(), "Account is not a member: " + member.to_string());
	m_t.erase (m_itr);
}

void dao::togglepause () {
	require_auth (get_self());
	config_table      config_s (get_self(), get_self().value);
   	Config c = config_s.get_or_create (get_self(), Config());   
	if (c.ints.find ("paused") == c.ints.end() || c.ints.at("paused") == 0) {
		c.ints["paused"]	= 1;
	} else {
		c.ints["paused"] 	= 0;
	} 	
	config_s.set (c, get_self());
}

void dao::setconfigname (	const string& key, const name& value) {
	require_auth (get_self());
	config_table      config_s (get_self(), get_self().value);
   	Config c = config_s.get_or_create (get_self(), Config());   
  	c.names["key"] 	= value;
    c.updated_date      = current_time_point();
	config_s.set (c, get_self());
}

void dao::setconfigint (	const string& key, const uint64_t& value) {
	require_auth (get_self());
	config_table      config_s (get_self(), get_self().value);
   	Config c = config_s.get_or_create (get_self(), Config());   
  	c.ints["key"] 	= value;
    c.updated_date      = current_time_point();
	config_s.set (c, get_self());
}


void dao::updversion (const string& component, const string& version) {
	config_table      config_s (get_self(), get_self().value);
   	Config c = config_s.get_or_create (get_self(), Config());   
	c.strings[component] = version;
	config_s.set (c, get_self());
}

void dao::setlastballt ( const name& last_ballot_id) {
	require_auth (get_self());
	config_table      config_s (get_self(), get_self().value);
   	Config c = config_s.get_or_create (get_self(), Config());   
	c.names["last_ballot_id"]			=	last_ballot_id;
	config_s.set (c, get_self());
}

void dao::enroll (	const name& enroller,
					const name& applicant, 
					const string& content) {

	validate_config_state ();
	
	// this action is linked to the daomain@enrollers permission
	applicant_table a_t (get_self(), get_self().value);
	auto a_itr = a_t.find (applicant.value);
	check (a_itr != a_t.end(), "Applicant not found: " + applicant.to_string());

	config_table      config_s (get_self(), get_self().value);
   	Config c = config_s.get_or_create (get_self(), Config());  

	asset one_vote = asset { 100, common::S_VOTE };
	string memo { "Welcome to the GBA DAO!"};
	action(	
		permission_level{get_self(), "active"_n}, 
		c.names.at("telos_decide_contract"), "mint"_n, 
		make_tuple(applicant, one_vote, memo))
	.send();

	action(
		permission_level{get_self(), "active"_n},
		"eosio"_n, "buyram"_n,
		make_tuple(get_self(), applicant, common::RAM_ALLOWANCE))
	.send();

	member_table m_t (get_self(), get_self().value);
	auto m_itr = m_t.find (applicant.value);
	check (m_itr == m_t.end(), "Account is already a member: " + applicant.to_string());
	m_t.emplace (get_self(), [&](auto &m) {
		m.member = applicant;
	});

	a_t.erase (a_itr);
}	

void dao::remapply (const name& applicant) {
	require_auth (get_self());
	applicant_table a_t (get_self(), get_self().value);
	auto a_itr = a_t.find (applicant.value);
	a_t.erase (a_itr);
}

void dao::debugmsg (const string& message) {
	require_auth (get_self());
	debug (message);
}

void dao::apply (const name& applicant, 
						const string& content) {

	validate_config_state();
	require_auth (applicant);

	member_table m_t (get_self(), get_self().value);
	auto m_itr = m_t.find (applicant.value);
	check (m_itr == m_t.end(), "Applicant is already a member: " + applicant.to_string());

	applicant_table a_t (get_self(), get_self().value);
	auto a_itr = a_t.find (applicant.value);

	if (a_itr != a_t.end()) {
		a_t.modify (a_itr, get_self(), [&](auto &a) {
			a.content = content;
			a.updated_date = current_time_point();
		});
	} else {
		a_t.emplace (get_self(), [&](auto &a) {
			a.applicant = applicant;
			a.content = content;
		});
	}
}				

name dao::register_ballot (const name& proposer, 
							const map<string, string>& strings) 
{
	check (has_auth (proposer) || has_auth(get_self()), "Authentication failed. Must have authority from proposer: " +
		proposer.to_string() + "@active or " + get_self().to_string() + "@active.");
	
	qualify_proposer(proposer);

	config_table      config_s (get_self(), get_self().value);
   	Config c = config_s.get_or_create (get_self(), Config());  
	
	// increment the ballot_id
	name new_ballot_id = name (c.names.at("last_ballot_id").value + 1);
	c.names["last_ballot_id"] = new_ballot_id;
	config_s.set(c, get_self());
	
	decidespace::decide::ballots_table b_t (c.names.at("telos_decide_contract"), c.names.at("telos_decide_contract").value);
	auto b_itr = b_t.find (new_ballot_id.value);
	check (b_itr == b_t.end(), "ballot_id: " + new_ballot_id.to_string() + " has already been used.");

	vector<name> options;
   	options.push_back ("pass"_n);
   	options.push_back ("fail"_n);

	action (
      permission_level{get_self(), "active"_n},
      c.names.at("telos_decide_contract"), "newballot"_n,
      std::make_tuple(
			new_ballot_id, 
			"poll"_n, 
			get_self(), 
			common::S_VOTE, 
			"1token1vote"_n, 
			options))
   .send();

	//	  // default is to count all tokens, not just staked tokens
	//    action (
	//       permission_level{get_self(), "active"_n},
	//       c.telos_decide_contract, "togglebal"_n,
	//       std::make_tuple(new_ballot_id, "votestake"_n))
	//    .send();

   action (
	   	permission_level{get_self(), "active"_n},
		c.names.at("telos_decide_contract"), "editdetails"_n,
		std::make_tuple(
			new_ballot_id, 
			strings.at("title"), 
			strings.at("description"),
			strings.at("content")))
   .send();

   auto expiration = time_point_sec(current_time_point()) + c.ints.at("voting_duration_sec");
   
   action (
      permission_level{get_self(), "active"_n},
      c.names.at("telos_decide_contract"), "openvoting"_n,
      std::make_tuple(new_ballot_id, expiration))
   .send();

	return new_ballot_id;
}

void dao::create (const name&						scope,
					const map<string, name> 		names,
					const map<string, string>       strings,
					const map<string, asset>        assets,
					const map<string, time_point>   time_points,
					const map<string, uint64_t>     ints,
					const map<string, float>        floats,
					const map<string, transaction>  trxs)
{
	validate_config_state();
	const name owner = names.at("owner");

	check (has_auth (owner) || has_auth(get_self()), "Authentication failed. Must have authority from owner: " +
		owner.to_string() + "@active or " + get_self().to_string() + "@active.");
	
	qualify_proposer (owner);

	object_table o_t (get_self(), scope.value);
	o_t.emplace (get_self(), [&](auto &o) {
		o.id                       	= o_t.available_primary_key();
		o.names                    	= names;
		o.strings                  	= strings;
		o.assets                  	= assets;
		o.time_points              	= time_points;
		o.ints                     	= ints;
		o.floats                   	= floats;
		o.trxs                     	= trxs;

		config_table      config_s (get_self(), get_self().value);
   		Config c = config_s.get_or_create (get_self(), Config()); 
		o.strings["client_version"] = get_string(c.strings, "client_version");
		o.strings["contract_version"] = get_string(c.strings, "contract_version");

		if (scope == "proposal"_n) {
			if (assets.find("reward_token_amount") != assets.end() || assets.find("vote_token_amount") != assets.end()) {
				checkx (names.find("recipient") != names.end(), "recipient parameter is required if using reward_token_amount or vote_token_amount");
			}
			o.names["ballot_id"]		= register_ballot (owner, strings);		
		}
	});      
}

void dao::closeprop(const uint64_t& proposal_id) {

	validate_config_state();

	// find the proposal object being closed
	object_table o_t (get_self(), "proposal"_n.value);
	auto o_itr = o_t.find(proposal_id);
	check (o_itr != o_t.end(), "Scope: " + "proposal"_n.to_string() + "; Object ID: " + std::to_string(proposal_id) + " does not exist.");
	auto prop = *o_itr;

	config_table      config_s (get_self(), get_self().value);
   	Config c = config_s.get_or_create (get_self(), Config());  

	// Find the ballot and treasury from the Telos Decide tables
	decidespace::decide::ballots_table b_t (c.names.at("telos_decide_contract"), c.names.at("telos_decide_contract").value);
	auto b_itr = b_t.find (prop.names.at("ballot_id").value);
	check (b_itr != b_t.end(), "ballot_id: " + prop.names.at("ballot_id").to_string() + " not found.");

	decidespace::decide::treasuries_table t_t (c.names.at("telos_decide_contract"), c.names.at("telos_decide_contract").value);
	auto t_itr = t_t.find (common::S_VOTE.code().raw());
	check (t_itr != t_t.end(), "Treasury: " + common::S_VOTE.code().to_string() + " not found.");

	// calculate the quorum and approval numbers
	float quorum_perc = (float) c.ints.at("quorum_perc_x100") / (float) 100;
	asset quorum_threshold = adjust_asset(t_itr->supply, quorum_perc);  
	
	// calculate the number of votes required for the proposal to pass
	float approval_perc = (float) c.ints.at("approval_threshold_perc_x100") / (float) 100;
	asset approval_threshold = adjust_asset(b_itr->total_raw_weight, approval_perc);

	// retrieve the amounts of passing and failing votes
	map<name, asset> votes = b_itr->options;
	asset votes_pass = votes.at("pass"_n);
	asset votes_fail = votes.at("fail"_n);

	// log the results to the debug table
	string debug_str = " Total Vote Weight: " + b_itr->total_raw_weight.to_string() + "\n";
	debug_str = debug_str + " Total Supply: " + t_itr->supply.to_string() + "\n"; 
	debug_str = debug_str + " Quorum Threshold: " + quorum_threshold.to_string() + "\n";	
	debug_str = debug_str + " Approval Threshold: " + approval_threshold.to_string() + "\n";	
	debug_str = debug_str + " Votes Passing: " + votes_pass.to_string() + "\n";
	debug_str = debug_str + " Votes Failing: " + votes_fail.to_string() + "\n";

	bool passed = false;
	if (b_itr->total_raw_weight >= quorum_threshold && 	// must meet quorum
				votes_pass > approval_threshold) {  // must have 50% of the vote power
		debug_str = debug_str + "Proposal passed.";
		passprop (proposal_id);
	} else {
		debug_str = debug_str + "Proposal failed.";
		change_scope("proposal"_n, proposal_id, name("failedprops"), true);
	}

	debug_str = debug_str + string ("Ballot ID read from prop for closing ballot: " + prop.names.at("ballot_id").to_string() + "\n");
	action (
		permission_level{get_self(), "active"_n},
		c.names.at("telos_decide_contract"), "closevoting"_n,
		std::make_tuple(prop.names.at("ballot_id"), true))
	.send();

	debug (debug_str);
}

void dao::passprop (const uint64_t& proposal_id) {

	object_table o_t(get_self(), "proposal"_n.value);
	auto o_itr = o_t.find(proposal_id);
	check (o_itr != o_t.end(), "Proposal ID does not exist: " + std::to_string(proposal_id));

	string memo{"Payments for proposal ID: " + std::to_string(proposal_id)};
	if (o_itr->strings.find("title") != o_itr->strings.end()) {
		memo = memo + "; proposal title: " + o_itr->strings.at("title");
	}

	if (o_itr->assets.find("reward_token_amount") != o_itr->assets.end() && 
			o_itr->names.find("recipient") != o_itr->names.end()) {
		makepayment(o_itr->names.at("recipient"), o_itr->assets.at("reward_token_amount"), memo);
	} 

	if (o_itr->assets.find("vote_token_amount") != o_itr->assets.end() && 
			o_itr->names.find("recipient") != o_itr->names.end()) {
		makepayment(o_itr->names.at("recipient"), o_itr->assets.at("vote_token_amount"), memo);
	}

	change_scope("proposal"_n, proposal_id, name("passedprops"), true);
}

void dao::qualify_proposer (const name& proposer) {
	// ownership of at least one voting token is required to post a proposal
	// if there should be other criteria, code that here
}
