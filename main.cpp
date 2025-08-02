#include <filesystem>
#include <iostream>
#include <string>

#include <soup/FileReader.hpp>
#include <soup/main.hpp>

#include <lua.hpp>

static std::filesystem::path get_exe_path()
{
#if SOUP_WINDOWS
	std::wstring path(MAX_PATH, '\0');
	path.resize(GetModuleFileNameW(NULL, path.data(), path.size()));
	path.shrink_to_fit();
	return path;
#else
	return std::filesystem::canonical("/proc/self/exe");
#endif
}

int entry(std::vector<std::string>&& args, bool console)
{
	auto L = luaL_newstate();
	luaL_openlibs(L);

	lua_newtable(L);
	for (size_t i = 0; i != args.size(); ++i)
	{
		lua_pushinteger(L, i);
		pluto_pushstring(L, std::move(args[i]));
		lua_settable(L, -3);
	}
	lua_setglobal(L, "arg");

	soup::FileReader fr(get_exe_path());
	fr.seekEnd();
	fr.seek(fr.getPosition() - 4);
	uint32_t footer_len;
	fr.u32_le(footer_len);
	fr.seek(fr.getPosition() - (4 + footer_len));

	std::string entrycode;
	fr.str_lp<soup::u32_le_t>(entrycode);
	//std::cout << entrycode << std::endl;

	lua_newtable(L);
	for (std::string name; fr.str_lp<soup::u32_le_t>(name) && !name.empty(); )
	{
		std::string code;
		fr.str_lp<soup::u32_le_t>(code);
		//std::cout << name << " = " << code << std::endl;
		pluto_pushstring(L, std::move(name));
		pluto_pushstring(L, std::move(code));
		lua_settable(L, -3);
	}
	lua_setglobal(L, "tiefiles");

	if (luaL_loadbuffer(L, entrycode.data(), entrycode.size(), nullptr) != LUA_OK
		|| lua_pcall(L, 0, 1, 0) != LUA_OK
		)
	{
		std::cout << (lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "[error is not a string]") << std::endl;
	}

#if SOUP_WINDOWS
	system("pause");
#endif
	return 0;
}

SOUP_MAIN_CLI(entry);
