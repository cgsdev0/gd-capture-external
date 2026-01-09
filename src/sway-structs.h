#ifndef _SWAY_STRUCTS_H
#define _SWAY_STRUCTS_H

#include <vector>
#include <string>
#include <optional>

namespace sway {
	struct Node {
		std::optional<std::string> name;
		std::string type;
		std::vector<Node> nodes;

	};
	struct Change {
		std::string change;
		Node container;

	};
	Node parse_node(const char* s);
	Change parse_change(const char* s);
}

#endif
