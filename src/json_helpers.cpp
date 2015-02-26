#include "common.hpp"
#include "json_helpers.hpp"

#include <SDL_rwops.h>
#include <memory>

using namespace nJsonHelpers;

bool nJsonHelpers::load_file(cstr filepath, std::function<bool(Value const&)> loader) {
	auto rw = SDL_RWFromFile(filepath, "rb");
	Sint64 size = SDL_RWsize(rw);
	if (size <= 0) {
		SDL_RWclose(rw);
		return false;
	}
	auto pdata = std::make_unique<char[]>(size + 1);
	SDL_RWread(rw, pdata.get(), size, 1);
	SDL_RWclose(rw);
	char* json = pdata.get();
	json[size] = 0;
	
	Document document;
	if (document.ParseInsitu<0>(json).HasParseError()) {
		dbg_msg("nJsonHelpers::load_file(): error parsing json file <%s>: %s\n%d", filepath, document.GetParseError());
		return false;
	}

	return loader(document);
}
