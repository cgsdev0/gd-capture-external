#include "sway-structs.h"
#include "json.hpp"

using json = nlohmann::json;

namespace sway {
    void to_json(json& j, const Node& p) {
        j = json{{"name", p.name}, {"type", p.type}, {"nodes", p.nodes}};
    }

    void from_json(const json& j, Node& p) {
        j.at("name").get_to(p.name);
        j.at("type").get_to(p.type);
        j.at("nodes").get_to(p.nodes);
    }

    void from_json(const json& j, Change& c) {
        j.at("change").get_to(c.change);
        j.at("container").get_to(c.container);
    }

//proof
	Node parse_node(const char* s) {
		Node n;
		from_json(json::parse(s), n);
		return n;
	}
	Change parse_change(const char* s) {
		Change c;
		from_json(json::parse(s), c);
		return c;
	}
}
